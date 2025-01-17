#include <common/bk_include.h>
#include "bk_arm_arch.h"

#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "video_transfer_common.h"

#include <components/video_transfer.h>

#include "bk_drv_model.h"
#include <os/mem.h>

#include <components/spidma.h>
#include <components/dvp_camera.h>
#include <driver/timer.h>
#include <driver/dma.h>

#if CONFIG_GENERAL_DMA
#include "bk_general_dma.h"
#endif

#define JPEG_EOF_CHECK              1
#define TVIDEO_DEBUG                1
#include "bk_uart.h"
#if TVIDEO_DEBUG
#define TVIDEO_PRT                  os_printf
#define TVIDEO_WPRT                 warning_prf
#define TVIDEO_FATAL                fatal_prf
#else
#define TVIDEO_PRT                  null_prf
#define TVIDEO_WPRT                 null_prf
#define TVIDEO_FATAL                null_prf
#endif

#define TVIDEO_TIMER_CHANNEL        TIMER_ID1
#define TVIDEO_TIMER_VALUE          2000// 1 second

video_config_t tvideo_st;
video_pool_t tvideo_pool;

#define TV_QITEM_COUNT      (120)
beken_thread_t  tvideo_thread_hdl = NULL;
beken_queue_t tvideo_msg_que = NULL;
static uint8_t  g_dma_id = 0;
static uint32_t g_frame_total_num = 0;
static uint32_t g_lost_flag = 0;
static uint32_t g_lost_frame_id = 0;
static uint8_t g_packet_count = 0;
static bool  tvideo_open = false;

bk_err_t tvideo_send_msg(uint8_t type, uint32_t data)
{
	bk_err_t ret;
	video_msg_t msg;

	if (tvideo_msg_que) {
		msg.type = type;
		msg.data = data;

		ret = rtos_push_to_queue(&tvideo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			TVIDEO_FATAL("tvideo_intfer_send_msg failed\r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t tvideo_pool_init(void *data)
{
	uint32_t i = 0;
	bk_err_t ret = kNoErr;
	video_setup_t *setup = (video_setup_t *)((int)data);

	if (tvideo_pool.pool == NULL) {
		tvideo_pool.pool = os_malloc(sizeof(uint8_t) * TVIDEO_POOL_LEN);
		if (tvideo_pool.pool == NULL) {
			TVIDEO_FATAL("tvideo_pool alloc failed\r\n");
			ret = kNoMemoryErr;
			return ret;
		}
	}

	os_memset(&tvideo_pool.pool[0], 0, sizeof(uint8_t)*TVIDEO_POOL_LEN);

	co_list_init(&tvideo_pool.free);
	co_list_init(&tvideo_pool.ready);
#if TVIDEO_DROP_DATA_NONODE
	co_list_init(&tvideo_pool.receiving);
	tvideo_pool.drop_pkt_flag = 0;
#endif

	for (i = 0; i < (TVIDEO_POOL_LEN / TVIDEO_RXNODE_SIZE); i++) {
		tvideo_pool.elem[i].buf_start =
			(void *)&tvideo_pool.pool[i * TVIDEO_RXNODE_SIZE];
		tvideo_pool.elem[i].buf_len = 0;

		co_list_push_back(&tvideo_pool.free,
						  (struct co_list_hdr *)&tvideo_pool.elem[i].hdr);
	}

	TVIDEO_PRT("video transfer send type:%d, open type:%d\r\n",
			   setup->send_type, setup->open_type);

	tvideo_pool.open_type = setup->open_type;
	tvideo_pool.send_type = setup->send_type;
	tvideo_pool.send_func = setup->send_func;
	tvideo_pool.start_cb = setup->start_cb;
	tvideo_pool.end_cb = setup->end_cb;

#if(TVIDEO_USE_HDR && CONFIG_CAMERA)
	// sccb with camera interface on chip, or default
	if ((tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
		&& ((setup->pkt_header_size % 4) != 0)) {
		TVIDEO_WPRT("pkt header-size should 4byte-aligned, but:%d\r\n",
					setup->pkt_header_size);
	}

	tvideo_pool.frame_id = 0;
	tvideo_pool.add_pkt_header = setup->add_pkt_header;
	tvideo_pool.pkt_header_size = setup->pkt_header_size;
#endif

	g_dma_id = bk_dma_alloc(DMA_DEV_DTCM);
	if ((g_dma_id < DMA_ID_0) || (g_dma_id >= DMA_ID_MAX))
	{
		TVIDEO_WPRT("dma alloc failed!\r\n");
		g_dma_id = DMA_ID_MAX;
	}
	TVIDEO_PRT("video_transfer dma:%d\r\n", g_dma_id);

	return ret;
}

static void tvideo_timer_check_callback(timer_id_t timer_id)
{
	tvideo_send_msg(VIDEO_CPU0_EOF_CHECK, 0);
}

static void tvideo_jpeg_eof_check_handler(void)
{
#if JPEG_EOF_CHECK
	if (g_frame_total_num < 10) {
		// jpeg eof error, reboot jpeg
		// step 1: stop timer
		bk_timer_stop(TVIDEO_TIMER_CHANNEL);

		// step 2: deinit
		bk_camera_deinit();

		// step 3: init
		bk_camera_init(&tvideo_st);
		g_frame_total_num = 0;

		// step 4: restart timer
		bk_timer_start(TVIDEO_TIMER_CHANNEL, TVIDEO_TIMER_VALUE, tvideo_timer_check_callback);
	} else {
		//TVIDEO_PRT("jpg:%d\r\n", g_frame_total_num);
		g_frame_total_num = 0;
	}
#else
	//TVIDEO_PRT("jpg:%d\r\n", g_frame_total_num);
#endif
}


static void tvideo_rx_handler(void *curptr, uint32_t newlen, uint32_t is_eof, uint32_t frame_len)
{
	video_elem_t *elem = NULL;
	if (is_eof) {
		g_frame_total_num++;
	}

	do {
		if (!newlen)
			break;

		if (tvideo_pool.frame_id != g_lost_frame_id) {
//			os_printf("frame_id_new = %d\r\n", tvideo_pool.frame_id);
			g_lost_flag = 0;
		}

		if (g_lost_flag && is_eof == 0) {
			return;
		}

#if TVIDEO_DROP_DATA_NONODE
		// drop pkt has happened, so drop it, until spidma timeout handler.
		if (tvideo_pool.drop_pkt_flag & TVIDEO_DROP_DATA_FLAG)
			break;
#endif

		elem = (video_elem_t *)co_list_pick(&tvideo_pool.free);
		if (elem) {
			if (newlen > tvideo_st.node_len)
				newlen = tvideo_st.node_len;

#if(TVIDEO_USE_HDR && CONFIG_CAMERA)
			// sccb with camera interface on chip, or default
			if (tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA) {
				uint32_t pkt_cnt = 0;
				video_packet_t param;

				if (is_eof) {
					pkt_cnt = frame_len / tvideo_st.node_len;
					if (frame_len % tvideo_st.node_len)
						pkt_cnt += 1;
				}

				param.ptk_ptr = (uint8_t *)elem->buf_start;
				param.ptklen = newlen;
				param.frame_id = tvideo_pool.frame_id;
				param.is_eof = is_eof;
				param.frame_len = pkt_cnt;

				if (tvideo_pool.add_pkt_header)
					tvideo_pool.add_pkt_header(&param);
				if (g_dma_id != DMA_ID_MAX) {
					dma_memcpy_by_chnl(param.ptk_ptr + tvideo_pool.pkt_header_size, curptr, newlen, g_dma_id);
				} else{
					os_memcpy(param.ptk_ptr + tvideo_pool.pkt_header_size, curptr, newlen);
				}
				
				if (tvideo_st.node_len > newlen) {
					//uint32_t left = tvideo_st.node_len - newlen;
					//os_memset((elem_tvhdr + 1 + newlen), 0, left);
				}
				//elem->buf_len = tvideo_st.node_len + sizeof(TV_HDR_ST);
				elem->buf_len = newlen + tvideo_pool.pkt_header_size;
				elem->frame_id = tvideo_pool.frame_id;
			} else
#endif //#if (TVIDEO_USE_HDR && CONFIG_CAMERA)
			{
				// only copy data
				
				if (g_dma_id != DMA_ID_MAX) {
					dma_memcpy_by_chnl(elem->buf_start, curptr, newlen, g_dma_id);
				} else{
					os_memcpy(elem->buf_start, curptr, newlen);
				}

				if (tvideo_st.node_len > newlen) {
					//uint32_t left = tvideo_st.node_len - newlen;
					//os_memset(((uint8_t*)elem->buf_start + newlen), 0, left);
				}
				//elem->buf_len = tvideo_st.node_len;
				elem->buf_len = newlen;
			}

			co_list_pop_front(&tvideo_pool.free);
#if TVIDEO_DROP_DATA_NONODE
			co_list_push_back(&tvideo_pool.receiving, (struct co_list_hdr *)&elem->hdr);
#else
			co_list_push_back(&tvideo_pool.ready, (struct co_list_hdr *)&elem->hdr);
#endif
			g_packet_count++;

		} 
		else
		{
#if TVIDEO_DROP_DATA_NONODE
			// not node for receive pkt, drop aready received, and also drop
			// the new come.
			uint32_t cnt_rdy = co_list_cnt(&tvideo_pool.receiving);

			tvideo_pool.drop_pkt_flag |= TVIDEO_DROP_DATA_FLAG;
			if (cnt_rdy)
				co_list_concat(&tvideo_pool.free, &tvideo_pool.receiving);
#else
			TVIDEO_WPRT("lost\r\n");
			g_lost_flag = 1;
			g_lost_frame_id = tvideo_pool.frame_id;
#endif
		}
	} while (0);

	if((g_packet_count >= 0 && g_packet_count < 4) && is_eof == 1) {
		tvideo_send_msg(VIDEO_CPU0_SEND, 0);
		g_packet_count = 0;
		return;
	}

	if (g_packet_count == 4) {
		tvideo_send_msg(VIDEO_CPU0_SEND, 0);
		g_packet_count = 0;
	}
}

static void tvideo_end_frame_handler(void)
{
#if TVIDEO_DROP_DATA_NONODE
	// reset drop flag, new pkt can receive
	tvideo_pool.drop_pkt_flag &= (~TVIDEO_DROP_DATA_FLAG);
	if (!co_list_is_empty(&tvideo_pool.receiving))
		co_list_concat(&tvideo_pool.ready, &tvideo_pool.receiving);
#endif

#if(TVIDEO_USE_HDR && CONFIG_CAMERA)
	if (tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
		tvideo_pool.frame_id++;
#endif

	tvideo_send_msg(VIDEO_CPU0_SEND, 0);
}

static bk_err_t tvideo_config_desc(void)
{
	bk_err_t ret = kNoErr;
	uint32_t node_len = TVIDEO_RXNODE_SIZE_TCP;

	if (tvideo_pool.send_type == TVIDEO_SND_UDP) {
#if(TVIDEO_USE_HDR && CONFIG_CAMERA)
		if (tvideo_pool.open_type != TVIDEO_OPEN_SPIDMA)
			node_len = TVIDEO_RXNODE_SIZE_UDP - tvideo_pool.pkt_header_size;
		else
#endif
			node_len = TVIDEO_RXNODE_SIZE_UDP;

	} else if (tvideo_pool.send_type == TVIDEO_SND_TCP)
		node_len = TVIDEO_RXNODE_SIZE_TCP;
	else if (tvideo_pool.send_type == TVIDEO_SND_INTF) {
#if(TVIDEO_USE_HDR && CONFIG_CAMERA)
		node_len = TVIDEO_RXNODE_SIZE_UDP - tvideo_pool.pkt_header_size;
#else
		node_len = TVIDEO_RXNODE_SIZE_UDP;
#endif
	} else if (tvideo_pool.send_type == TVIDEO_SND_BUFFER)
		node_len = TVIDEO_RXNODE_SIZE_TCP;
	else {
		TVIDEO_WPRT("Err snd tpye in spidma\r\n");
		ret = kParamErr;
		return ret;
	}

	if (tvideo_st.rxbuf == NULL)
	{
		tvideo_st.rxbuf = os_malloc(sizeof(uint8_t) * TVIDEO_RXBUF_LEN);
		if (tvideo_st.rxbuf == NULL) {
			TVIDEO_WPRT("malloc rxbuf failed!\r\n");
			ret = kNoMemoryErr;
			return ret;
		}
	}

	tvideo_st.rxbuf_len = node_len * 4;
	tvideo_st.node_len = node_len;
	tvideo_st.rx_read_len = 0;

	tvideo_st.sener_cfg = 0;
	// set for gc0328c
	CMPARAM_SET_PPI(tvideo_st.sener_cfg, VGA_640_480);
	CMPARAM_SET_FPS(tvideo_st.sener_cfg, TYPE_20FPS);
	// set for hm_1055
	//CMPARAM_SET_PPI(tvideo_st.sener_cfg, VGA_1280_720);
	//CMPARAM_SET_FPS(tvideo_st.sener_cfg, TYPE_15FPS);

	tvideo_st.node_full_handler = tvideo_rx_handler;
	tvideo_st.data_end_handler = tvideo_end_frame_handler;

	return ret;
}

static void tvideo_poll_handler(void)
{
	uint32_t send_len;
	video_elem_t *elem = NULL;

	do {
		elem = (video_elem_t *)co_list_pick(&tvideo_pool.ready);
		if (elem) {
			if (tvideo_pool.send_func) {
				if (elem->frame_id == g_lost_frame_id) {

				} else {
					send_len = tvideo_pool.send_func(elem->buf_start, elem->buf_len);
					if (send_len != elem->buf_len)
						break;
				}
			}

			co_list_pop_front(&tvideo_pool.ready);
			co_list_push_back(&tvideo_pool.free, (struct co_list_hdr *)&elem->hdr);
		}
	} while (elem);
}

/*---------------------------------------------------------------------------*/
static void video_transfer_main(beken_thread_arg_t data)
{
	bk_err_t err;
	TVIDEO_PRT("video_transfer_main entry\r\n");

	err = tvideo_pool_init(data);
	if (err != kNoErr) {
		goto tvideo_exit;
	}

	err = tvideo_config_desc();
	if (err != kNoErr) {
		goto tvideo_exit;
	}


	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA) {

#if CONFIG_SPIDMA
		spidma_intfer_init(&tvideo_st);
#endif
		} else {//if(tvideo_pool.open_type == TVIDEO_OPEN_SCCB)
			err = bk_camera_init(&tvideo_st);
			if (err != kNoErr) {
				goto tvideo_exit;
			}

			tvideo_open = true;
	}

	if (tvideo_pool.start_cb != NULL)
		tvideo_pool.start_cb();

	bk_timer_start(TVIDEO_TIMER_CHANNEL, TVIDEO_TIMER_VALUE, tvideo_timer_check_callback);

	while (1) {
		video_msg_t msg;
		err = rtos_pop_from_queue(&tvideo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == err) {
			switch (msg.type) {
			case VIDEO_CPU0_SEND:
				tvideo_poll_handler();
				break;

			case VIDEO_CPU0_EOF_CHECK:
				tvideo_jpeg_eof_check_handler();
				break;

			case VIDEO_CPU0_EXIT:
				goto tvideo_exit;
				break;

			default:
				break;
			}
		}
	}

	if (tvideo_pool.end_cb != NULL)
		tvideo_pool.end_cb();

tvideo_exit:
	TVIDEO_PRT("video_transfer_main exit\r\n");

	bk_timer_stop(TVIDEO_TIMER_CHANNEL);
	g_frame_total_num = 0;

	if (g_dma_id != DMA_ID_MAX)
	{
		bk_dma_stop(g_dma_id);

		bk_dma_deinit(g_dma_id);

		bk_dma_free(DMA_DEV_DTCM, g_dma_id);
	}

	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA) {
#if CONFIG_SPIDMA
		spidma_intfer_deinit();
#endif
	} else {
		if (tvideo_open)
			bk_camera_deinit();
	}

	if (tvideo_pool.pool) {
		os_free(tvideo_pool.pool);
		tvideo_pool.pool = NULL;
	}

	if (tvideo_st.rxbuf) {
		os_free(tvideo_st.rxbuf);
		tvideo_st.rxbuf = NULL;
	}

	os_memset(&tvideo_st, 0, sizeof(video_config_t));

	rtos_deinit_queue(&tvideo_msg_que);
	tvideo_msg_que = NULL;

	tvideo_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}

video_setup_t video_transfer_setup_bak = {0};
bk_err_t bk_video_transfer_init(video_setup_t *setup_cfg)
{
	int ret;

	TVIDEO_PRT("video_transfer_init %d,%d\r\n", setup_cfg->send_type, setup_cfg->open_type);

	if ((!tvideo_thread_hdl) && (!tvideo_msg_que)) {
		// bakup setup_cfg, because of that 'setup_cfg' may not static value.
		os_memcpy(&video_transfer_setup_bak, setup_cfg, sizeof(video_setup_t));

		ret = rtos_init_queue(&tvideo_msg_que,
							  "tvideo_queue",
							  sizeof(video_msg_t),
							  TV_QITEM_COUNT);
		if (kNoErr != ret) {
			TVIDEO_FATAL("spidma_intfer ceate queue failed\r\n");
			return kGeneralErr;
		}

		ret = rtos_create_thread(&tvideo_thread_hdl,
								 4,
								 "video_intf",
								 (beken_thread_function_t)video_transfer_main,
								 4 * 1024,
								 (beken_thread_arg_t)&video_transfer_setup_bak);
		if (ret != kNoErr) {
			rtos_deinit_queue(&tvideo_msg_que);
			tvideo_msg_que = NULL;
			tvideo_thread_hdl = NULL;
			TVIDEO_FATAL("Error: Failed to create spidma_intfer: %d\r\n", ret);
			return kGeneralErr;
		}

		return kNoErr;
	} else
		return kInProgressErr;
}

bk_err_t bk_video_transfer_deinit(void)
{
	TVIDEO_PRT("video_transfer_deinit\r\n");

	tvideo_send_msg(VIDEO_CPU0_EXIT, 0);

	while (tvideo_thread_hdl)
		rtos_delay_milliseconds(10);

	return kNoErr;
}


bk_err_t bk_video_transfer_stop(void)
{
	int ret = kNoErr;

	// stop jpeg
	if (!tvideo_open)
	{
		TVIDEO_PRT("video_transfer_stop already!\r\n");
		return ret;
	}

	bk_timer_stop(TVIDEO_TIMER_CHANNEL);

	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA) {
#if CONFIG_SPIDMA
		spidma_intfer_deinit();
#endif
	} else {
		ret = bk_camera_deinit();
	}

	tvideo_open = false;

	return ret;
}

bk_err_t bk_video_transfer_start(void)
{
	int ret = kNoErr;

	if (tvideo_open)
	{
		TVIDEO_PRT("video_transfer_start already!\r\n");
		return ret;
	}

	// start jpeg
	if (tvideo_pool.open_type == TVIDEO_OPEN_SPIDMA)
	{

#if CONFIG_SPIDMA
		spidma_intfer_init(&tvideo_st);
#endif
	}
	else
	{
		ret = bk_camera_init(&tvideo_st);
	}

	if (ret != kNoErr)
	{
		TVIDEO_PRT("video_transfer_start failed!\r\n");
		return ret;
	}

	bk_timer_start(TVIDEO_TIMER_CHANNEL, TVIDEO_TIMER_VALUE, tvideo_timer_check_callback);

	tvideo_open = true;

	return ret;
}
