// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <components/log.h>

#include "media_core.h"
#include "media_evt.h"
#include "lcd_act.h"
#include "frame_buffer.h"

#include <driver/int.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>

#include <driver/dma.h>

#include <driver/jpeg_enc.h>
#include <driver/jpeg_enc_types.h>
#include <driver/dvp_camera.h>
#include <driver/dvp_camera_types.h>

#include <soc/mapping.h>

#include <driver/lcd.h>
#include <driver/dma.h>
#include <driver/gpio.h>
#include <driver/jpeg_dec.h>
#include <driver/dma2d.h>
//#include <lcd_dma2d_config.h>
#include <driver/jpeg_dec_types.h>
#include "modules/image_scale.h"
//#include <BK7256_RegList.h>
#include <driver/uvc_camera_types.h>
#include <driver/uvc_camera.h>

#include <driver/pwm.h>
#include "modules/image_scale.h"

#include "cache.h"


#define TAG "rotate"
#include "cache.h"

#if CONFIG_SLAVE_CORE
#define MINOOR_DTCM __attribute__((section(".dtcm_sec_data ")))
#else
#define MINOOR_DTCM
#endif

#if CONFIG_SLAVE_CORE
#define MINOOR_ITCM __attribute__((section(".itcm_sec_code ")))
#else
#define MINOOR_ITCM
#endif

#if CONFIG_SLAVE_CORE
#define MINOOR_ITCM_BSS __attribute__((section(".itcm_sec_bss ")))
#else
#define MINOOR_ITCM_BSS
#endif

#include "mailbox_channel.h"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

frame_buffer_t *rotate_frame = NULL;
frame_buffer_t *decoder_frame = NULL;

uint32_t lcd_width = 0;
uint32_t lcd_height = 0;

lcd_info_t *lcd_info_ptr = NULL;


#define BLOCK_WIDTH     (160)
#define BLOCK_HEIGHT    (40)
#define BLOCK_SIZE      (BLOCK_WIDTH * BLOCK_HEIGHT * 2)

MINOOR_DTCM uint8_t rx_block[BLOCK_SIZE];
MINOOR_ITCM_BSS uint8_t tx_block[BLOCK_SIZE];



void rotate_complete(frame_buffer_t *frame)
{
	media_msg_t msg;
	msg.event = EVENT_LCD_ROTATE_RIGHT_COMP_EVT;
	msg.param = (uint32_t)frame;
	media_send_msg(&msg);
}


MINOOR_ITCM void memcpy_word(uint32_t *dst, uint32_t *src, uint32_t size)
{
	uint32_t i = 0;

	for (i = 0; i < size; i++)
	{
		dst[i] = src[i];
	}
}

#if 0
MINOOR_ITCM void lcd_act_rotate_degree90(uint32_t param)
{

	lcd_info_ptr = (lcd_info_t *)param;
	uint32_t i, j, k;
	uint8_t *cp_ptr = NULL;
	uint16_t src_width, src_height;

	decoder_frame = lcd_info_ptr->decoder_frame;
	rotate_frame = lcd_info_ptr->rotate_frame;

	src_width = lcd_info_ptr->src_pixel_x;
	src_height = lcd_info_ptr->src_pixel_y;

	lcd_width = lcd_info_ptr->lcd_pixel_x;
	lcd_height = lcd_info_ptr->lcd_pixel_y;

#if(1) // cpu1 cache enable 
	uint8_t *dst_frame_temp = rotate_frame->frame + 0x4000000;
	uint8_t *src_frame_temp = decoder_frame->frame + 0x4000000;

	flush_dcache(src_frame_temp, JPEG_DEC_FRAME_SIZE);
	flush_dcache(dst_frame_temp, JPEG_DEC_FRAME_SIZE);
#endif
	LOGD("camera %d:%d, lcd %d:%d\n", src_width, src_height, lcd_width, lcd_height);

	if (lcd_width == src_width
	    && lcd_width == src_width)
	{
		LOGD("do not rotate\n");
		memcpy_word((uint32_t *)(rotate_frame->frame), (uint32_t *)decoder_frame->frame, decoder_frame->length / 4);
		rotate_complete(rotate_frame);
		return;
	}

	for (j = 0; j < (src_height / BLOCK_HEIGHT); j++)
	{

		for (i = 0; i < (src_width / BLOCK_WIDTH); i++)
		{
			for (k = 0; k < BLOCK_HEIGHT; k++)
			{
#if(1) // cpu1 cache enable 
				cp_ptr = src_frame_temp + i * BLOCK_WIDTH * 2 + j * BLOCK_HEIGHT * src_width * 2 + k * src_width * 2;
#else
				cp_ptr =  decoder_frame->frame  + i * BLOCK_WIDTH * 2 + j * BLOCK_HEIGHT * src_width * 2 + k * src_width * 2;
#endif
				memcpy_word((uint32_t *)(rx_block + BLOCK_WIDTH * 2 * k), (uint32_t *)cp_ptr, BLOCK_WIDTH * 2 / 4);
			}

			vuyy_rotate_degree90(rx_block, tx_block, BLOCK_WIDTH, BLOCK_HEIGHT);

			for (k = 0; k < BLOCK_WIDTH; k++)
			{
#if(1) // cpu1 cache enable 
				cp_ptr = dst_frame_temp + (src_height / BLOCK_HEIGHT - j - 1) * BLOCK_HEIGHT * 2 + (i) * BLOCK_WIDTH * src_height * 2 + k * src_height * 2;
#else
				cp_ptr = rotate_frame->frame + (src_height / BLOCK_HEIGHT - j - 1) * BLOCK_HEIGHT * 2 + (i) * BLOCK_WIDTH * src_height * 2 + k * src_height * 2;
#endif
				memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + BLOCK_HEIGHT * 2 * k), BLOCK_HEIGHT * 2 / 4);
			}
		}
	}

#if CONFIG_SLAVE_CORE
	rotate_complete(rotate_frame);
#endif
}
#else
MINOOR_ITCM void lcd_act_rotate_degree90(uint32_t param)
{
	register uint32_t i, j, k;
	register uint8_t *cp_ptr = NULL;
	register uint16_t src_width, src_height;
	mb_chnl_cmd_t mb_cmd;

	int (*func)(unsigned char *vuyy, unsigned char *rotatedVuyy, int width, int height);
	src_width = decoder_frame->width;
	src_height = decoder_frame->height;

	register uint8_t *dst_frame_temp = rotate_frame->frame + 0x4000000;
	register uint8_t *src_frame_temp = decoder_frame->frame + 0x4000000;

	switch (rotate_frame->fmt)
	{
		case PIXEL_FMT_VUYY:
			func = vuyy_rotate_degree90;
			break;
		case PIXEL_FMT_YUYV:
		default:
			func = yuyv_rotate_degree90;
			break;
	}

	flush_dcache(src_frame_temp, JPEG_DEC_FRAME_SIZE);
	flush_dcache(dst_frame_temp, JPEG_DEC_FRAME_SIZE);

	for (j = 0; j < (src_height / BLOCK_HEIGHT); j++)
	{

		for (i = 0; i < (src_width / BLOCK_WIDTH); i++)
		{
			for (k = 0; k < BLOCK_HEIGHT; k++)
			{
				cp_ptr = src_frame_temp + i * BLOCK_WIDTH * 2 + j * BLOCK_HEIGHT * src_width * 2 + k * src_width * 2;
				memcpy_word((uint32_t *)(rx_block + BLOCK_WIDTH * 2 * k), (uint32_t *)cp_ptr, BLOCK_WIDTH * 2 / 4);
			}

			func(rx_block, tx_block, BLOCK_WIDTH, BLOCK_HEIGHT);

			for (k = 0; k < BLOCK_WIDTH; k++)
			{
				cp_ptr = dst_frame_temp + (src_height / BLOCK_HEIGHT - j - 1) * BLOCK_HEIGHT * 2 + (i) * BLOCK_WIDTH * src_height * 2 + k * src_height * 2;
				memcpy_word((uint32_t *)cp_ptr, (uint32_t *)(tx_block + BLOCK_HEIGHT * 2 * k), BLOCK_HEIGHT * 2 / 4);
			}
		}
	}

	mb_cmd.hdr.cmd = 0x19;
	mb_cmd.param1 = (uint32_t)decoder_frame;
	mb_cmd.param2 = (uint32_t)rotate_frame;
	mb_cmd.param3 = 1;

	mb_chnl_write(MB_CHNL_VID, &mb_cmd);


}

#endif



static void lcd_calc_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
	//LOGI("%s, %08X\n", __func__, cmd_buf->hdr.cmd);

	if (cmd_buf->hdr.cmd == 0x18)
	{
		decoder_frame = (frame_buffer_t *)cmd_buf->param1;
		rotate_frame = (frame_buffer_t *)cmd_buf->param2;

		media_msg_t msg;
		msg.event = EVENT_LCD_ROTATE_RIGHT_CMD;
		media_send_msg(&msg);
	}
}


static void lcd_calc_mailbox_tx_isr(void *param)
{
	LOGD("%s\n", __func__);

}

static void lcd_calc_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
	LOGD("%s\n", __func__);

}

void lcd_calc_init(void)
{
	mb_chnl_open(MB_CHNL_VID, NULL);
	mb_chnl_ctrl(MB_CHNL_VID, MB_CHNL_SET_RX_ISR, lcd_calc_mailbox_rx_isr);
	mb_chnl_ctrl(MB_CHNL_VID, MB_CHNL_SET_TX_ISR, lcd_calc_mailbox_tx_isr);
	mb_chnl_ctrl(MB_CHNL_VID, MB_CHNL_SET_TX_CMPL_ISR, lcd_calc_mailbox_tx_cmpl_isr);
}
