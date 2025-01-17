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
#include <os/mem.h>
#include <os/str.h>
#include "stdio.h"
#include "sys_driver.h"
#if CONFIG_AUD_TRAS_MODE_CPU1
#include "audio_mailbox.h"
#include "media_common.h"
#endif
#include "aud_intf_private.h"
#if CONFIG_AUD_TRAS_MODE_CPU0
#include "aud_tras_drv_api.h"
#endif


#define MAILBOX_CHECK_CONTROL    0

#define AUD_INTF_TAG "aud_intf"

#define LOGI(...) BK_LOGI(AUD_INTF_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_INTF_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_INTF_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_INTF_TAG, ##__VA_ARGS__)

/* check aud_intf busy status */
#define CHECK_AUD_INTF_BUSY_STA() do {\
		if (aud_intf_info.api_info.busy_status) {\
			return BK_ERR_AUD_INTF_BUSY;\
		}\
		aud_intf_info.api_info.busy_status = true;\
	} while(0)

//aud_intf_all_setup_t aud_all_setup;
aud_intf_info_t aud_intf_info;

#if CONFIG_AUD_TRAS_MODE_CPU1
#define TU_QITEM_COUNT      (60)
beken_thread_t aud_tras_demo_thread_hdl = NULL;
beken_queue_t aud_tras_demo_msg_que = NULL;

#if MAILBOX_CHECK_CONTROL
static uint32_t audio_tras_mb_busy_status = false;
#endif
#endif //CONFIG_AUD_TRAS_MODE_CPU1


/* extern api */
//static void audio_tras_demo_deconfig(void);
#if CONFIG_AUD_TRAS_MODE_CPU1
static bk_err_t audio_tras_send_msg(audio_tras_opcode_t op, uint32_t param1, uint32_t param2, uint32_t param3);
#endif

#if CONFIG_AUD_TRAS_DAC_DEBUG
static bool dac_debug_enable = false ;
#endif

static bk_err_t aud_intf_voc_write_spk_data(uint8_t *dac_buff, uint32_t size);

#if CONFIG_AUD_TRAS_DAC_DEBUG
bk_err_t bk_aud_intf_set_voc_dac_debug(bool enable)
{
	dac_debug_enable = enable;
	return aud_tras_drv_send_msg(AUD_TRAS_VOC_DAC_BEBUG, (void *)&dac_debug_enable);
}
#endif

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
static void audio_tras_aec_dump_config(void)
{
	sprintf(aud_intf_info.sin_file_name, "1:/%s", "aec_dump_sin.pcm");
	sprintf(aud_intf_info.ref_file_name, "1:/%s", "aec_dump_ref.pcm");
	sprintf(aud_intf_info.out_file_name, "1:/%s", "aec_dump_out.pcm");
}

static bk_err_t audio_tras_aec_dump_file_open(void)
{
	FRESULT fr;

	/*open file to save sin data of AEC */
	fr = f_open(&(aud_intf_info.sin_file), aud_intf_info.sin_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		LOGE("open %s fail.\r\n", aud_intf_info.sin_file_name);
		return BK_FAIL;
	}

	/*open file to save ref data of AEC */
	fr = f_open(&(aud_intf_info.ref_file), aud_intf_info.ref_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		LOGE("open %s fail.\r\n", aud_intf_info.ref_file_name);
		return BK_FAIL;
	}

	/*open file to save out data of AEC */
	fr = f_open(&(aud_intf_info.out_file), aud_intf_info.out_file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK) {
		LOGE("open %s fail.\r\n", aud_intf_info.out_file_name);
		return BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t audio_tras_aec_dump_file_close(void)
{
	FRESULT fr;

	/* close sin file */
	fr = f_close(&(aud_intf_info.sin_file));
	if (fr != FR_OK) {
		LOGE("close %s fail!\r\n", aud_intf_info.sin_file_name);
		return BK_FAIL;
	}
	LOGI("close sin file complete \r\n");

	/* close ref file */
	fr = f_close(&(aud_intf_info.ref_file));
	if (fr != FR_OK) {
		LOGE("close %s fail!\r\n", aud_intf_info.ref_file_name);
		return BK_FAIL;
	}
	LOGI("close ref file complete \r\n");

	/* close unused file */
	fr = f_close(&(aud_intf_info.out_file));
	if (fr != FR_OK) {
		LOGE("close %s fail!\r\n", aud_intf_info.out_file_name);
		return BK_FAIL;
	}
	LOGI("close out file complete \r\n");

	return BK_OK;
}

static bk_err_t audio_tras_aec_dump_handle(void)
{
	FRESULT fr;
	uint32 uiTemp = 0;

	/* write sin data to file */
	fr = f_write(&(aud_intf_info.sin_file), (void *)aud_intf_info.voc_info.aec_dump.mic_dump_addr, aud_intf_info.voc_info.aec_dump.len, &uiTemp);
	if (fr != FR_OK) {
		LOGE("write %s fail.\r\n", aud_intf_info.sin_file_name);
		return BK_FAIL;
	}

	/* write ref data to file */
	fr = f_write(&(aud_intf_info.ref_file), (void *)aud_intf_info.voc_info.aec_dump.ref_dump_addr, aud_intf_info.voc_info.aec_dump.len, &uiTemp);
	if (fr != FR_OK) {
		LOGE("write %s fail.\r\n", aud_intf_info.ref_file_name);
		return BK_FAIL;
	}

	/* write unused data to file */
	fr = f_write(&(aud_intf_info.out_file), (void *)aud_intf_info.voc_info.aec_dump.out_dump_addr, aud_intf_info.voc_info.aec_dump.len, &uiTemp);
	if (fr != FR_OK) {
		LOGE("write %s fail.\r\n", aud_intf_info.out_file_name);
		return BK_FAIL;
	}

	return BK_OK;
}
#endif //CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
#endif //CONFIG_AUD_TRAS_AEC_DUMP_DEBUG

static bk_err_t aud_intf_send_msg_with_sem(aud_tras_drv_op_t op, void *param, uint32_t ms)
{
	bk_err_t ret = BK_OK;

	ret = aud_tras_drv_send_msg(op, param);
	if (ret != BK_OK) {
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_TX_MSG;
	}

	if (rtos_get_semaphore(&aud_intf_info.api_info.sem, ms) != BK_OK) {
		LOGE("%s wait semaphore failed\n", __func__);
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_TIMEOUT;
	}

	aud_intf_info.api_info.busy_status = false;
	return aud_intf_info.api_info.result;
}

/* common api */
static void aud_tras_drv_com_event_cb_handle(aud_tras_drv_com_event_t event, bk_err_t result)
{
	switch (event) {
		case EVENT_AUD_TRAS_COM_INIT:
			LOGD("init aud_intf driver complete \r\n");
			aud_intf_info.drv_status = AUD_INTF_DRV_STA_IDLE;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_COM_DEINIT:
			LOGD("deinit aud_intf driver complete \r\n");
			aud_intf_info.drv_status = AUD_INTF_DRV_STA_NULL;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_COM_SET_MODE:
			LOGD("set aud_intf mode complete \r\n");
			if (result == BK_ERR_AUD_INTF_OK)
				aud_intf_info.drv_status = AUD_INTF_DRV_STA_WORK;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_COM_SET_MIC_GAIN:
			LOGD("set mic gain complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_COM_SET_SPK_GAIN:
			LOGD("set spk gain complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		default:
			break;
	}
}

bk_err_t bk_aud_intf_set_mode(aud_intf_work_mode_t work_mode)
{
	if (aud_intf_info.drv_info.setup.work_mode == work_mode)
		return BK_ERR_AUD_INTF_OK;

	CHECK_AUD_INTF_BUSY_STA();
	aud_intf_info.drv_info.setup.work_mode = work_mode;
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SET_MODE, &aud_intf_info.drv_info.setup.work_mode, 2000);
}

bk_err_t bk_aud_intf_set_mic_gain(uint8_t value)
{
	/* check value range */
	if (value > 0x3F)
		return BK_ERR_AUD_INTF_PARAM;

	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL || aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
		if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL) {
			aud_intf_info.mic_info.mic_gain = value;
			return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_SET_GAIN, &aud_intf_info.mic_info.mic_gain, 1000);
		}

		if (aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
			aud_intf_info.voc_info.aud_setup.adc_gain = value;
			return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_SET_MIC_GAIN, &aud_intf_info.voc_info.aud_setup.adc_gain, 1000);
		}
	}

	aud_intf_info.api_info.busy_status = false;
	return BK_ERR_AUD_INTF_STA;
}

bk_err_t bk_aud_intf_set_mic_samp_rate(aud_adc_samp_rate_t samp_rate)
{
	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL) {
		aud_intf_info.mic_info.samp_rate = samp_rate;
		return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_SET_SAMP_RATE, &aud_intf_info.mic_info.samp_rate, 1000);
	}

	aud_intf_info.api_info.busy_status = false;
	return BK_ERR_AUD_INTF_STA;
}

bk_err_t bk_aud_intf_set_mic_chl(aud_intf_mic_chl_t mic_chl)
{
	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL) {
		aud_intf_info.mic_info.mic_chl = mic_chl;
		return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_SET_CHL, &aud_intf_info.mic_info.mic_chl, 1000);
	}

	aud_intf_info.api_info.busy_status = false;
	return BK_ERR_AUD_INTF_STA;
}

bk_err_t bk_aud_intf_set_spk_gain(uint8_t value)
{
	/* check value range */
	if (value > 0x3F)
		return BK_ERR_AUD_INTF_PARAM;

	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL || aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
		if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL) {
			aud_intf_info.spk_info.spk_gain = value;
			//return aud_tras_drv_send_msg(AUD_TRAS_DRV_SPK_SET_GAIN, &aud_intf_info.spk_info.spk_gain);
			return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_SET_GAIN, &aud_intf_info.spk_info.spk_gain, 1000);
		}

		if (aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
			aud_intf_info.voc_info.aud_setup.dac_gain = value;
			//return aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_SET_SPK_GAIN, &aud_intf_info.voc_info.aud_setup.dac_gain);
			return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_SET_SPK_GAIN, &aud_intf_info.voc_info.aud_setup.dac_gain, 1000);
		}
	}

	aud_intf_info.api_info.busy_status = false;
	return BK_ERR_AUD_INTF_STA;
}

bk_err_t bk_aud_intf_set_spk_samp_rate(aud_dac_samp_rate_t samp_rate)
{
	CHECK_AUD_INTF_BUSY_STA();

	/*check spk status */
	if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL) {
		aud_intf_info.spk_info.samp_rate = samp_rate;
		return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_SET_SAMP_RATE, &aud_intf_info.spk_info.samp_rate, 1000);
	}

	aud_intf_info.api_info.busy_status = false;
	return BK_ERR_AUD_INTF_STA;
}

bk_err_t bk_aud_intf_set_spk_chl(aud_intf_spk_chl_t spk_chl)
{
	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL) {
		aud_intf_info.spk_info.spk_chl = spk_chl;
		return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_SET_CHL, &aud_intf_info.spk_info.spk_chl, 1000);
	}

	aud_intf_info.api_info.busy_status = false;
	return BK_ERR_AUD_INTF_STA;
}

bk_err_t bk_aud_intf_set_aec_para(aud_intf_voc_aec_para_t aec_para, uint32_t value)
{
	bk_err_t ret = BK_OK;
//	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	aud_intf_voc_aec_ctl_t *aec_ctl = NULL;

	/*check aec status */
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	aec_ctl = os_malloc(sizeof(aud_intf_voc_aec_ctl_t));
	if (aec_ctl == NULL) {
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_MEMY;
	}
	aec_ctl->op = aec_para;
	aec_ctl->value = value;

	switch (aec_para) {
		case AUD_INTF_VOC_AEC_MIC_DELAY:
			aud_intf_info.voc_info.aec_setup->mic_delay = value;
			break;

		case AUD_INTF_VOC_AEC_EC_DEPTH:
			aud_intf_info.voc_info.aec_setup->ec_depth = value;
			break;

		case AUD_INTF_VOC_AEC_REF_SCALE:
			aud_intf_info.voc_info.aec_setup->ref_scale = value;
			break;

		case AUD_INTF_VOC_AEC_VOICE_VOL:
			aud_intf_info.voc_info.aec_setup->voice_vol = value;
			break;

		case AUD_INTF_VOC_AEC_TXRX_THR:
			aud_intf_info.voc_info.aec_setup->TxRxThr = value;
			break;

		case AUD_INTF_VOC_AEC_TXRX_FLR:
			aud_intf_info.voc_info.aec_setup->TxRxFlr = value;
			break;

		case AUD_INTF_VOC_AEC_NS_LEVEL:
			aud_intf_info.voc_info.aec_setup->ns_level = value;
			break;

		case AUD_INTF_VOC_AEC_NS_PARA:
			aud_intf_info.voc_info.aec_setup->ns_para = value;
			break;

		case AUD_INTF_VOC_AEC_DRC:
			aud_intf_info.voc_info.aec_setup->drc = value;
			break;

		case AUD_INTF_VOC_AEC_INIT_FLAG:
			aud_intf_info.voc_info.aec_setup->init_flags = value;
			break;

		default:
			break;
	}

	ret = aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_SET_AEC_PARA, aec_ctl, 20000);
	if (ret != BK_ERR_AUD_INTF_OK)
		os_free(aec_ctl);

	return ret;
}

bk_err_t bk_aud_intf_get_aec_para(void)
{
	/*check aec status */
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_GET_AEC_PARA, NULL, 20000);
}

static void aud_tras_drv_mic_event_cb_handle(aud_tras_drv_mic_event_t event, bk_err_t result)
{
	switch (event) {
		case EVENT_AUD_TRAS_MIC_INIT:
			LOGD("init aud mic complete \r\n");
			aud_intf_info.mic_status = AUD_INTF_MIC_STA_IDLE;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_MIC_START:
			LOGD("start aud mic complete \r\n");
			aud_intf_info.mic_status = AUD_INTF_MIC_STA_START;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_MIC_PAUSE:
			LOGD("pause aud mic complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_MIC_STOP:
			LOGD("stop aud mic complete \r\n");
			aud_intf_info.mic_status = AUD_INTF_MIC_STA_IDLE;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_MIC_DEINIT:
			LOGD("deinit aud mic complete \r\n");
			aud_intf_info.mic_status = AUD_INTF_MIC_STA_NULL;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_MIC_SET_SAMP_RATE:
			LOGD("set aud mic samp rate complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_MIC_SET_CHL:
			LOGD("set aud mic chl complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		default:
			break;
	}
}

bk_err_t bk_aud_intf_mic_init(aud_intf_mic_setup_t *setup)
{
	CHECK_AUD_INTF_BUSY_STA();

	aud_intf_info.mic_info.mic_chl = setup->mic_chl;
	aud_intf_info.mic_info.samp_rate = setup->samp_rate;
	aud_intf_info.mic_info.frame_size = setup->frame_size;
	aud_intf_info.mic_info.mic_gain = setup->mic_gain;
	aud_intf_info.mic_info.aud_tras_drv_mic_event_cb = aud_tras_drv_mic_event_cb_handle;

	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_INIT, &aud_intf_info.mic_info, 2000);
}

bk_err_t bk_aud_intf_mic_deinit(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_DEINIT, NULL, 1000);
}

bk_err_t bk_aud_intf_mic_start(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_START, NULL, 1000);
}

bk_err_t bk_aud_intf_mic_pause(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_PAUSE, NULL, 1000);
}

bk_err_t bk_aud_intf_mic_stop(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_MIC_STOP, NULL, 1000);
}

static void aud_intf_spk_deconfig(void)
{
	aud_intf_info.spk_info.spk_chl = AUD_INTF_SPK_CHL_LEFT;
	aud_intf_info.spk_info.samp_rate = AUD_DAC_SAMP_RATE_8K;
	aud_intf_info.spk_info.frame_size = 0;
	aud_intf_info.spk_info.spk_gain = 0;
	ring_buffer_clear(aud_intf_info.spk_info.spk_rx_rb);
	os_free(aud_intf_info.spk_info.spk_rx_ring_buff);
	aud_intf_info.spk_info.spk_rx_ring_buff = NULL;
	os_free(aud_intf_info.spk_info.spk_rx_rb);
	aud_intf_info.spk_info.spk_rx_rb = NULL;
}

static void aud_tras_drv_spk_event_cb_handle(aud_tras_drv_spk_event_t event, bk_err_t result)
{
	switch (event) {
		case EVENT_AUD_TRAS_SPK_INIT:
			LOGD("init aud spk complete \r\n");
			aud_intf_info.spk_status = AUD_INTF_SPK_STA_IDLE;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_SPK_START:
			LOGD("start aud spk complete \r\n");
			aud_intf_info.spk_status = AUD_INTF_SPK_STA_START;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_SPK_PAUSE:
			LOGD("pause aud spk complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_SPK_STOP:
			LOGD("stop aud spk complete \r\n");
			aud_intf_info.spk_status = AUD_INTF_SPK_STA_IDLE;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_SPK_DEINIT:
			LOGD("deinit aud spk complete \r\n");
			aud_intf_spk_deconfig();
			aud_intf_info.spk_status = AUD_INTF_SPK_STA_NULL;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_SPK_SET_SAMP_RATE:
			LOGD("set aud spk samp rate complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_SPK_SET_CHL:
			LOGD("set aud spk chl complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		default:
			break;
	}
}

bk_err_t bk_aud_intf_spk_init(aud_intf_spk_setup_t *setup)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	//os_printf("bk_aud_intf_spk_init \r\n");
	CHECK_AUD_INTF_BUSY_STA();

	aud_intf_info.spk_info.spk_chl = setup->spk_chl;
	aud_intf_info.spk_info.samp_rate = setup->samp_rate;
	aud_intf_info.spk_info.frame_size = setup->frame_size;
	aud_intf_info.spk_info.spk_gain = setup->spk_gain;
	aud_intf_info.spk_info.work_mode = setup->work_mode;
	aud_intf_info.spk_info.fifo_frame_num = 4;			//default: 4
	aud_intf_info.spk_info.aud_tras_drv_spk_event_cb = aud_tras_drv_spk_event_cb_handle;

	aud_intf_info.spk_info.spk_rx_ring_buff = os_malloc(aud_intf_info.spk_info.frame_size * aud_intf_info.spk_info.fifo_frame_num);
	if (aud_intf_info.spk_info.spk_rx_ring_buff == NULL) {
		LOGE("malloc spk_rx_ring_buff fail \r\n");
		err = BK_ERR_AUD_INTF_MEMY;
		return BK_FAIL;
	}

	aud_intf_info.spk_info.spk_rx_rb = os_malloc(sizeof(RingBufferContext));
	if (aud_intf_info.spk_info.spk_rx_rb == NULL) {
		LOGE("malloc spk_rx_rb fail \r\n");
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_spk_init_exit;
	}

	ring_buffer_init(aud_intf_info.spk_info.spk_rx_rb, (uint8_t *)aud_intf_info.spk_info.spk_rx_ring_buff, aud_intf_info.spk_info.frame_size * aud_intf_info.spk_info.fifo_frame_num, DMA_ID_MAX, RB_DMA_TYPE_NULL);
	ret = aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_INIT, &aud_intf_info.spk_info, 2000);
	if (ret != BK_ERR_AUD_INTF_OK) {
		err = ret;
		goto aud_intf_spk_init_exit;
	}

	return BK_ERR_AUD_INTF_OK;

aud_intf_spk_init_exit:
	if (aud_intf_info.spk_info.spk_rx_ring_buff != NULL)
		os_free(aud_intf_info.spk_info.spk_rx_ring_buff);
	if (aud_intf_info.spk_info.spk_rx_rb != NULL)
		os_free(aud_intf_info.spk_info.spk_rx_rb);
	aud_intf_info.spk_info.aud_tras_drv_spk_event_cb = NULL;
	return err;
}

bk_err_t bk_aud_intf_spk_deinit(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_DEINIT, NULL, 1000);
}

bk_err_t bk_aud_intf_spk_start(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_START, NULL, 1000);
}

bk_err_t bk_aud_intf_spk_pause(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_PAUSE, NULL, 1000);
}

bk_err_t bk_aud_intf_spk_stop(void)
{
	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_STOP, NULL, 1000);
}

static void aud_intf_voc_deconfig(void)
{
	/* audio deconfig */
	aud_intf_info.voc_info.samp_rate = AUD_INTF_VOC_SAMP_RATE_8K;	//default value
	aud_intf_info.voc_info.aud_setup.adc_gain = 0;
	aud_intf_info.voc_info.aud_setup.dac_gain = 0;
	aud_intf_info.voc_info.aud_setup.mic_frame_number = 0;
	aud_intf_info.voc_info.aud_setup.mic_samp_rate_points = 0;
	aud_intf_info.voc_info.aud_setup.speaker_frame_number = 0;
	aud_intf_info.voc_info.aud_setup.speaker_samp_rate_points = 0;

	/* aec deconfig */
	if (aud_intf_info.voc_info.aec_enable) {
		os_free(aud_intf_info.voc_info.aec_setup);
	}
	aud_intf_info.voc_info.aec_setup = NULL;
	aud_intf_info.voc_info.aec_enable = false;

	/* tx deconfig */
	aud_intf_info.voc_info.tx_info.buff_length = 0;
	aud_intf_info.voc_info.tx_info.tx_buff_status = false;
	aud_intf_info.voc_info.tx_info.ping.busy_status = false;
	os_free(aud_intf_info.voc_info.tx_info.ping.buff_addr);
	aud_intf_info.voc_info.tx_info.ping.buff_addr = NULL;
	aud_intf_info.voc_info.tx_info.pang.busy_status = false;
	os_free(aud_intf_info.voc_info.tx_info.pang.buff_addr);
	aud_intf_info.voc_info.tx_info.pang.buff_addr = NULL;

	/* rx deconfig */
	aud_intf_info.voc_info.rx_info.rx_buff_status = false;
	os_free(aud_intf_info.voc_info.rx_info.decoder_ring_buff);
	aud_intf_info.voc_info.rx_info.decoder_ring_buff = NULL;
	os_free(aud_intf_info.voc_info.rx_info.decoder_rb);
	aud_intf_info.voc_info.rx_info.decoder_rb = NULL;
	aud_intf_info.voc_info.rx_info.frame_num = 0;
	aud_intf_info.voc_info.rx_info.frame_size = 0;
	aud_intf_info.voc_info.rx_info.fifo_frame_num = 0;
	aud_intf_info.voc_info.rx_info.rx_buff_seq_tail = 0;
	aud_intf_info.voc_info.rx_info.aud_trs_read_seq = 0;

	/* callback deconfig */
	aud_intf_info.voc_info.aud_tras_drv_voc_event_cb = NULL;

	/* delate audio transfer task */
#if CONFIG_AUD_TRAS_MODE_CPU1
//	audio_tras_send_msg(AUD_TRAS_EXIT, 0, 0, 0);
#endif

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
	os_free(aud_intf_info.voc_info.aec_dump.mic_dump_addr);
	aud_intf_info.voc_info.aec_dump.mic_dump_addr = NULL;
	os_free(aud_intf_info.voc_info.aec_dump.ref_dump_addr);
	aud_intf_info.voc_info.aec_dump.ref_dump_addr = NULL;
	os_free(aud_intf_info.voc_info.aec_dump.out_dump_addr);
	aud_intf_info.voc_info.aec_dump.out_dump_addr = NULL;
	aud_intf_info.voc_info.aec_dump.len = 0;

	bk_err_t ret = BK_OK;
	ret = audio_tras_aec_dump_file_close();
	if (ret != BK_OK) {
		LOGE("close aec dump file fail \r\n");
	}
#endif //CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
#endif //CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
}

static void aud_tras_drv_voc_event_cb_handle(aud_tras_drv_voc_event_t event, bk_err_t result)
{
	switch (event) {
		case EVENT_AUD_TRAS_VOC_INIT:
			LOGD("init voc complete \r\n");
			aud_intf_info.voc_status = AUD_INTF_VOC_STA_IDLE;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_VOC_START:
			LOGD("start voc transfer complete \r\n");
			aud_intf_info.voc_status = AUD_INTF_VOC_STA_START;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_VOC_STOP:
			LOGD("stop voc transfer complete \r\n");
			aud_intf_info.voc_status = AUD_INTF_VOC_STA_STOP;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_VOC_DEINIT:
			LOGD("deinit voc transfer complete \r\n");
			aud_intf_voc_deconfig();
			aud_intf_info.voc_status = AUD_INTF_VOC_STA_NULL;
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_VOC_SET_AEC_PARA:
			LOGD("set voc aec param complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

		case EVENT_AUD_TRAS_VOC_GET_AEC_PARA:
			LOGD("get voc aec param complete \r\n");
			aud_intf_info.api_info.result = result;
			rtos_set_semaphore(&aud_intf_info.api_info.sem);
			break;

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
		case EVENT_AUD_TRAS_VOC_AEC_DUMP:
			audio_tras_aec_dump_handle();
			//os_printf("stop audio transfer complete \r\n");
			break;
#endif
#endif

		default:
			break;
	}
}

bk_err_t bk_aud_intf_voc_init(aud_intf_voc_setup_t setup)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	CHECK_AUD_INTF_BUSY_STA();

	aud_intf_info.api_info.busy_status = true;

	//aud_tras_drv_setup.aud_trs_mode = demo_setup.mode;
	aud_intf_info.voc_info.samp_rate = setup.samp_rate;
	aud_intf_info.voc_info.aec_enable = setup.aec_enable;
	aud_intf_info.voc_info.data_type = setup.data_type;
	/* audio config */
	aud_intf_info.voc_info.aud_setup.adc_gain = setup.mic_gain;	//default: 0x2d
	aud_intf_info.voc_info.aud_setup.dac_gain = setup.spk_gain;	//default: 0x2d
	aud_intf_info.voc_info.aud_setup.mic_frame_number = 15;
	aud_intf_info.voc_info.aud_setup.mic_samp_rate_points = 160;	//if AEC enable , the value is equal to aec_samp_rate_points, and the value not need to set
	aud_intf_info.voc_info.aud_setup.speaker_frame_number = 2;
	aud_intf_info.voc_info.aud_setup.speaker_samp_rate_points = 160;	//if AEC enable , the value is equal to aec_samp_rate_points, and the value not need to set
	aud_intf_info.voc_info.aud_setup.spk_mode = setup.spk_mode;

	if (aud_intf_info.voc_info.aec_enable) {
		/* aec config */
		aud_intf_info.voc_info.aec_setup = os_malloc(sizeof(aec_config_t));
		if (aud_intf_info.voc_info.aec_setup == NULL) {
			LOGE("malloc aec_setup fail \r\n");
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_intf_voc_init_exit;
		}
		aud_intf_info.voc_info.aec_setup->init_flags = 0x1f;
		aud_intf_info.voc_info.aec_setup->mic_delay = 0;
		aud_intf_info.voc_info.aec_setup->ec_depth = setup.aec_cfg.ec_depth;
		aud_intf_info.voc_info.aec_setup->ref_scale = 0;
		aud_intf_info.voc_info.aec_setup->TxRxThr = setup.aec_cfg.TxRxThr;
		aud_intf_info.voc_info.aec_setup->TxRxFlr = setup.aec_cfg.TxRxFlr;
		aud_intf_info.voc_info.aec_setup->voice_vol = 14;
		aud_intf_info.voc_info.aec_setup->ns_level = setup.aec_cfg.ns_level;
		aud_intf_info.voc_info.aec_setup->ns_para = setup.aec_cfg.ns_para;
		aud_intf_info.voc_info.aec_setup->drc = 15;
	} else {
		aud_intf_info.voc_info.aec_setup = NULL;
	}

	/* tx config */
	switch (aud_intf_info.voc_info.data_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
			aud_intf_info.voc_info.tx_info.buff_length = aud_intf_info.voc_info.aud_setup.mic_samp_rate_points;
			break;

		case AUD_INTF_VOC_DATA_TYPE_PCM:
			aud_intf_info.voc_info.tx_info.buff_length = aud_intf_info.voc_info.aud_setup.mic_samp_rate_points * 2;
			break;

		default:
			break;
	}
	aud_intf_info.voc_info.tx_info.ping.busy_status = false;
	aud_intf_info.voc_info.tx_info.ping.buff_addr = os_malloc(aud_intf_info.voc_info.tx_info.buff_length);
	if (aud_intf_info.voc_info.tx_info.ping.buff_addr == NULL) {
		LOGE("malloc pingpang buffer of tx fail \r\n");
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	aud_intf_info.voc_info.tx_info.pang.busy_status = false;
	aud_intf_info.voc_info.tx_info.pang.buff_addr = os_malloc(aud_intf_info.voc_info.tx_info.buff_length);
	if (aud_intf_info.voc_info.tx_info.pang.buff_addr == NULL) {
		LOGE("malloc pang buffer of tx fail \r\n");
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	aud_intf_info.voc_info.tx_info.tx_buff_status = true;

	/* rx config */
	aud_intf_info.voc_info.rx_info.aud_trs_read_seq = 0;
	switch (aud_intf_info.voc_info.data_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
			aud_intf_info.voc_info.rx_info.frame_size = 320;
			break;

		case AUD_INTF_VOC_DATA_TYPE_PCM:
			aud_intf_info.voc_info.rx_info.frame_size = 320 * 2;
			break;

		default:
			break;
	}
	aud_intf_info.voc_info.rx_info.frame_num = 15;
	aud_intf_info.voc_info.rx_info.rx_buff_seq_tail = 0;
	aud_intf_info.voc_info.rx_info.fifo_frame_num = 10;
	aud_intf_info.voc_info.rx_info.decoder_ring_buff = os_malloc(aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.frame_num);
	if (aud_intf_info.voc_info.rx_info.decoder_ring_buff == NULL) {
		LOGE("malloc decoder ring buffer of rx fail \r\n");
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	LOGI("malloc decoder_ring_buff:%p, size:%d \r\n", aud_intf_info.voc_info.rx_info.decoder_ring_buff, aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.frame_num);
	aud_intf_info.voc_info.rx_info.decoder_rb = os_malloc(sizeof(RingBufferContext));
	if (aud_intf_info.voc_info.rx_info.decoder_rb == NULL) {
		LOGE("malloc decoder_rb fail \r\n");
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	ring_buffer_init(aud_intf_info.voc_info.rx_info.decoder_rb, (uint8_t *)aud_intf_info.voc_info.rx_info.decoder_ring_buff, (aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.frame_num), DMA_ID_MAX, RB_DMA_TYPE_NULL);
	aud_intf_info.voc_info.rx_info.rx_buff_status = true;

	//os_printf("decoder_rb:%p \r\n", aud_tras_drv_setup.rx_context.decoder_rb);

	/* callback config */
	aud_intf_info.voc_info.aud_tras_drv_voc_event_cb = aud_tras_drv_voc_event_cb_handle;

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
	if (aud_intf_info.voc_info.samp_rate == AUD_INTF_VOC_SAMP_RATE_8K)
		aud_intf_info.voc_info.aec_dump.len = 320;
	else if (aud_intf_info.voc_info.samp_rate == AUD_INTF_VOC_SAMP_RATE_16K)
		aud_intf_info.voc_info.aec_dump.len = 640;
	else
		aud_intf_info.voc_info.aec_dump.len = 0;

	if (aud_intf_info.voc_info.aec_dump.len) {
		aud_intf_info.voc_info.aec_dump.mic_dump_addr = (int16_t *)os_malloc(aud_intf_info.voc_info.aec_dump.len);
		if (aud_intf_info.voc_info.aec_dump.mic_dump_addr == NULL) {
			LOGE("malloc mic_dump_addr fail \r\n");
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_intf_voc_init_exit;
		}
		aud_intf_info.voc_info.aec_dump.ref_dump_addr = (int16_t *)os_malloc(aud_intf_info.voc_info.aec_dump.len);
		if (aud_intf_info.voc_info.aec_dump.ref_dump_addr == NULL) {
			LOGE("malloc ref_dump_addr fail \r\n");
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_intf_voc_init_exit;
		}
		aud_intf_info.voc_info.aec_dump.out_dump_addr = (int16_t *)os_malloc(aud_intf_info.voc_info.aec_dump.len);
		if (aud_intf_info.voc_info.aec_dump.out_dump_addr == NULL) {
			LOGE("malloc ref_dump_addr fail \r\n");
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_intf_voc_init_exit;
		}
	}

	audio_tras_aec_dump_config();
	ret = audio_tras_aec_dump_file_open();
	if (ret != BK_OK) {
		LOGE("open dump file fail \r\n");
		err = BK_ERR_AUD_INTF_FILE;
		goto aud_intf_voc_init_exit;
	}
#endif //CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
#endif //CONFIG_AUD_TRAS_AEC_DUMP_DEBUG

	ret = aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_INIT, (void *)&aud_intf_info.voc_info);
	if (ret != BK_OK) {
		err = BK_ERR_AUD_INTF_TX_MSG;
		goto aud_intf_voc_init_exit;
	}

	if (rtos_get_semaphore(&aud_intf_info.api_info.sem, 3000) != BK_OK) {
		LOGE("%s wait semaphore failed\n", __func__);
		err = BK_ERR_AUD_INTF_TIMEOUT;
		goto aud_intf_voc_init_exit;
	}

	if (aud_intf_info.api_info.result != BK_ERR_AUD_INTF_OK) {
		err = aud_intf_info.api_info.result;
		os_printf("err:%d \r\n", err);
		goto aud_intf_voc_init_exit;
	} else {
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_OK;
	}

aud_intf_voc_init_exit:
	if (aud_intf_info.voc_info.aec_setup != NULL)
		os_free(aud_intf_info.voc_info.aec_setup);
	if (aud_intf_info.voc_info.tx_info.ping.buff_addr != NULL)
		os_free(aud_intf_info.voc_info.tx_info.ping.buff_addr);
	if (aud_intf_info.voc_info.tx_info.pang.buff_addr != NULL)
		os_free(aud_intf_info.voc_info.tx_info.pang.buff_addr);
	if (aud_intf_info.voc_info.rx_info.decoder_ring_buff != NULL)
		os_free(aud_intf_info.voc_info.rx_info.decoder_ring_buff);
	if (aud_intf_info.voc_info.rx_info.decoder_rb != NULL)
		os_free(aud_intf_info.voc_info.rx_info.decoder_rb);
	aud_intf_info.voc_info.aud_tras_drv_voc_event_cb = NULL;

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
#if CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
	if (aud_intf_info.voc_info.aec_dump.mic_dump_addr != NULL)
		os_free(aud_intf_info.voc_info.aec_dump.mic_dump_addr);
	if (aud_intf_info.voc_info.aec_dump.ref_dump_addr != NULL)
		os_free(aud_intf_info.voc_info.aec_dump.ref_dump_addr);
	if (aud_intf_info.voc_info.aec_dump.out_dump_addr != NULL)
		os_free(aud_intf_info.voc_info.aec_dump.out_dump_addr);
#endif //CONFIG_AUD_TRAS_AEC_DUMP_MODE_TF
#endif //CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
	aud_intf_info.api_info.busy_status = false;

	return err;
}

bk_err_t bk_aud_intf_voc_deinit(void)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL) {
		LOGI("voice is alreay deinit \r\n");
		return BK_ERR_AUD_INTF_OK;
	}

	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_DEINIT, NULL, 1000);
}

bk_err_t bk_aud_intf_voc_start(void)
{
	switch (aud_intf_info.voc_status) {
		case AUD_INTF_VOC_STA_NULL:
		case AUD_INTF_VOC_STA_START:
			return BK_ERR_AUD_INTF_STA;

		case AUD_INTF_VOC_STA_IDLE:
		case AUD_INTF_VOC_STA_STOP:
			if (ring_buffer_get_fill_size(aud_intf_info.voc_info.rx_info.decoder_rb)/aud_intf_info.voc_info.rx_info.frame_size < aud_intf_info.voc_info.rx_info.fifo_frame_num) {
				uint8_t *temp_buff = NULL;
				uint32_t temp_size = aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.fifo_frame_num - ring_buffer_get_fill_size(aud_intf_info.voc_info.rx_info.decoder_rb);
				temp_buff = os_malloc(temp_size);
				if (temp_buff == NULL) {
					return BK_ERR_AUD_INTF_MEMY;
				} else {
					os_memset(temp_buff, 0xD5, temp_size);
					aud_intf_voc_write_spk_data(temp_buff, temp_size);
					os_free(temp_buff);
				}
			}
			break;

		default:
			return BK_ERR_AUD_INTF_FAIL;
	}

	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_START, NULL, 2000);
}

bk_err_t bk_aud_intf_voc_stop(void)
{
	switch (aud_intf_info.voc_status) {
		case AUD_INTF_VOC_STA_NULL:
		case AUD_INTF_VOC_STA_IDLE:
		case AUD_INTF_VOC_STA_STOP:
			return BK_ERR_AUD_INTF_STA;
			break;

		case AUD_INTF_VOC_STA_START:
			break;

		default:
			return BK_ERR_AUD_INTF_FAIL;
	}

	CHECK_AUD_INTF_BUSY_STA();
	return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_STOP, NULL, 1000);
}


#if CONFIG_AUD_TRAS_MODE_CPU1
/* media common mailbox api */
static bk_err_t media_send_common_mb_msg(common_maibox_cmd_t cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
	mb_chnl_cmd_t mb_cmd;

	//send mailbox msg to cpu0
	mb_cmd.hdr.cmd = cmd;
	mb_cmd.param1 = param1;
	mb_cmd.param2 = param2;
	mb_cmd.param3 = param3;
	return mb_chnl_write(MB_CHNL_COM, &mb_cmd);
}

static void media_mb_rx_isr(common_mailbox_msg_t *com_mb, mb_chnl_ack_t *cmd_buf)
{
	bk_err_t ret = BK_OK;
	common_maibox_cmd_t op;

	/* check mailbox msg and send msg to task */
	switch(cmd_buf->hdr.cmd) {
		case CMD_COM_MB_AUD_TRAS_INIT_CMPL:
			audio_transfer_event_process(EVENT_AUD_TRAS_INIT_CMP);
			break;

		default:
			break;
	}

	if (ret != kNoErr) {
		LOGE("send msg: %d fail \r\n", op);
	}

}

static void media_mb_tx_isr(common_mailbox_msg_t *com_mb, mb_chnl_ack_t *cmd_buf)
{
	//TODO
}

static void media_mb_tx_cmpl_isr(common_mailbox_msg_t *com_mb, mb_chnl_ack_t *cmd_buf)
{
	//TODO
}

static void media_com_mailbox_init(void)
{
	//init maibox
	//mb_chnl_init();
	mb_chnl_open(MB_CHNL_COM, NULL);
	if (media_mb_rx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_COM, MB_CHNL_SET_RX_ISR, media_mb_rx_isr);
	if (media_mb_tx_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_COM, MB_CHNL_SET_TX_ISR, media_mb_tx_isr);
	if (media_mb_tx_cmpl_isr != NULL)
		mb_chnl_ctrl(MB_CHNL_COM, MB_CHNL_SET_TX_CMPL_ISR, media_mb_tx_cmpl_isr);
}


static bk_err_t audio_tras_send_msg(audio_tras_opcode_t op, uint32_t param1, uint32_t param2, uint32_t param3)
{
	bk_err_t ret;
	audio_tras_msg_t msg;

	msg.op = op;
	msg.param1 = param1;
	msg.param2 = param2;
	msg.param3 = param3;
	if (aud_tras_demo_msg_que) {
		ret = rtos_push_to_queue(&aud_tras_demo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("send audio_tras_msg fail \r\n");
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t audio_tras_send_aud_mb_msg(audio_tras_mb_cmd_t cmd, uint32_t param1, uint32_t param2, uint32_t param3)
{
	bk_err_t ret = BK_OK;

	audio_tras_mb_msg_t mb_msg;

	mb_msg.mb_cmd = cmd;
	mb_msg.param1 = param1;
	mb_msg.param2 = param2;
	mb_msg.param3 = param3;

#if MAILBOX_CHECK_CONTROL

	/* wait send audio_mailbox_msg complete */
	while(audio_cp0_mailbox_busy_status)
		;

	audio_cp0_mailbox_busy_status = true;

	ret = aud_tras_mb_send_msg(&mb_msg);
	if (ret != BK_OK)
		audio_cp0_mailbox_busy_status = false;

#else
	ret = aud_tras_mb_send_msg(&mb_msg);
#endif

	return ret;
}

static void audio_tras_mb_tx_isr(aud_mb_t *aud_mb)
{
	//os_printf("enter cp0_mailbox_tx_isr \r\n");
}

static void audio_tras_mb_tx_cmpl_isr(aud_mb_t *aud_mb, mb_chnl_ack_t *cmd_buf)
{
	//os_printf("enter cp0_mailbox_tx_cmpl \r\n");

#if MAILBOX_CHECK_CONTROL
	audio_cp0_mailbox_busy_status = false;
#endif

}

static void audio_tras_mb_rx_isr(aud_mb_t *aud_mb, mb_chnl_cmd_t *cmd_buf)
{
	bk_err_t ret = BK_OK;
	//audio_tras_mb_msg_t msg;
	//audio_tras_mb_cmd_t op = AUD_TRAS_IDLE;

	/* check mailbox msg and send msg to task */
	switch (cmd_buf->hdr.cmd) {
		case AUD_TRAS_MB_CMD_TX_REQ:
			ret = audio_tras_send_msg(AUD_TRAS_TX_MIC_DATA, cmd_buf->param1, cmd_buf->param2, cmd_buf->param3);
			if (ret != kNoErr) {
				LOGE("send audio_tras_msg: AUD_TRAS_TX_MIC_DATA fail \r\n");
			}
			break;

		case AUD_TRAS_MB_CMD_STOP_TRANSFER_DONE:
			audio_transfer_event_process(EVENT_AUD_TRAS_STOP_CMP);
			break;

#if 0
		case AUD_MB_WRITE_DAC_DATA_DONE_CMD:
			msg.op = AUDIO_CP0_WRITE_DONE;
			break;

		case AUD_MB_READ_ADC_DATA_REQ_CMD:
			msg.op = AUDIO_CP0_READ_REQ;
			break;

		case AUD_MB_WRITE_DAC_DATA_REQ_CMD:
			msg.op = AUDIO_CP0_WRITE_REQ;
			break;

		case AUD_MB_STOP_TRANSFER_CMD:
			msg.op = AUDIO_CP0_EXIT;
			ret = audio_send_stop_msg(msg);
			if (ret != kNoErr) {
				os_printf("cp0: send stop msg: %d fail \r\n", msg.op);
			}
			os_printf("cp0: send stop msg: %d complete \r\n", msg.op);
			return;

		case AUD_MB_START_TRANSFER_DONE_CMD:
			msg.op = AUDIO_CP0_READY;
			break;
#endif
		default:
			break;
	}

}

#endif	//CONFIG_AUD_TRAS_MODE_CPU1



#if 0
static void aud_intf_drv_deconfig(void)
{

	/* delate audio transfer task */
#if CONFIG_AUD_TRAS_MODE_CPU1
//	audio_tras_send_msg(AUD_TRAS_EXIT, 0, 0, 0);
#endif

#if CONFIG_AUD_TRAS_AEC_DUMP_DEBUG
	os_free(aud_tras_drv_setup.aec_dump.mic_dump_addr);
	aud_tras_drv_setup.aec_dump.mic_dump_addr = NULL;
	os_free(aud_tras_drv_setup.aec_dump.ref_dump_addr);
	aud_tras_drv_setup.aec_dump.ref_dump_addr = NULL;
	os_free(aud_tras_drv_setup.aec_dump.out_dump_addr);
	aud_tras_drv_setup.aec_dump.out_dump_addr = NULL;
	aud_tras_drv_setup.aec_dump.len = 0;

	bk_err_t ret = BK_OK;
	ret = audio_tras_aec_dump_file_close();
	if (ret != BK_OK) {
		os_printf("close aec dump file fail \r\n");
	}
#endif
}
#endif

#if CONFIG_AUD_TRAS_MODE_CPU1
/* audio transfer demo task in cpu1 mode */
static void audio_tras_demo_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;
	uint32_t size = 0;

	/* init media common mailbox channel */
	media_com_mailbox_init();
	/* send audio transfer init mailbox command to CPU1 */
	media_send_common_mb_msg(CMD_COM_MB_AUD_TRAS_INIT, (uint32_t)(&aud_tras_drv_setup), 0, 0);

	/* init maibox */
	aud_tras_mb_init(audio_tras_mb_rx_isr, audio_tras_mb_tx_isr, audio_tras_mb_tx_cmpl_isr);
	LOGI("config mailbox complete \r\n");

	while(1) {
		audio_tras_msg_t msg;
		ret = rtos_pop_from_queue(&aud_tras_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {

				case AUD_TRAS_IDLE:
					break;

				case AUD_TRAS_TX_MIC_DATA:
					if (aud_tras_drv_setup.aud_cb.audio_send_mic_data) {
						size = aud_tras_drv_setup.aud_cb.audio_send_mic_data((unsigned char *)msg.param1, msg.param2);
						if (size != msg.param2) {
							//os_printf("send audio data by wifi fail \r\n");
							//os_printf("audio lost \r\n");
						}
						//response Tx Ack
						audio_tras_send_aud_mb_msg(AUD_TRAS_MB_CMD_TX_DONE, msg.param1, msg.param2, 0);
					}
					break;
#if 0
				case AUDIO_CP0_GET_DECODER_REMAIN_SIZE_DONE:
					if (audio_get_decoder_remain_size_callback)
						audio_get_decoder_remain_size_callback(decoder_remain_size);
					break;

				case AUDIO_CP0_READ_REQ:
					if (audio_encoder_read_req_handler)
						audio_encoder_read_req_handler();
					break;

				case AUDIO_CP0_READ_DONE:
					if (audio_read_done_callback)
						audio_read_done_callback((uint8_t *)read_buffer, read_buffer_size);
					break;

				case AUDIO_CP0_WRITE_REQ:
					if (audio_decoder_write_req_handler)
						audio_decoder_write_req_handler();
					break;

				case AUDIO_CP0_WRITE_DONE:
					if (audio_write_done_callback)
						audio_write_done_callback((uint8_t *)write_buffer, write_buffer_size);
					break;

				case AUDIO_CP0_READY:
					if (audio_transfer_ready_callback)
						audio_transfer_ready_callback();
					break;
#endif

				case AUD_TRAS_EXIT:
					/* receive mailbox msg of stop test from cpu1, and stop transfer */
					/* deinit audio transfer demo */
					audio_tras_demo_deconfig();
					aud_intf_info.status = AUD_TRAS_STA_IDLE;
					goto audio_transfer_exit;
					//os_printf("cp0: goto audio_transfer_exit \r\n");
					break;

				default:
					break;
			}
		}
	}

audio_transfer_exit:
	/* deinit audio transfer mailbox channel */
	aud_tras_mb_deinit();

	/* delate msg queue */
	ret = rtos_deinit_queue(&aud_tras_demo_msg_que);
	if (ret != kNoErr) {
		LOGE("delate message queue fail \r\n");
		//return BK_FAIL;
	}
	aud_tras_demo_msg_que = NULL;
	LOGI("delate message queue complete \r\n");

	/* delate task */
	aud_tras_demo_thread_hdl = NULL;
	rtos_delete_thread(NULL);
}
#endif	//CONFIG_AUD_TRAS_MODE_CPU1

bk_err_t bk_aud_intf_drv_init(aud_intf_drv_setup_t *setup)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	if (aud_intf_info.api_info.busy_status) {
		return BK_ERR_AUD_INTF_BUSY;
	}

	if (aud_intf_info.drv_status != AUD_INTF_DRV_STA_NULL) {
		LOGI("aud_intf driver already init \r\n");
		return BK_ERR_AUD_INTF_OK;
	}

	aud_intf_info.api_info.busy_status = true;
	ret = rtos_init_semaphore_ex(&aud_intf_info.api_info.sem, 1, 0);
	if (ret != BK_OK) {
		LOGE("init semaphore fail \r\n");
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_FAIL;
	}

	/* save drv_info */
	aud_intf_info.drv_info.setup = *setup;
	aud_intf_info.drv_info.aud_tras_drv_com_event_cb = aud_tras_drv_com_event_cb_handle;

	/* init audio interface driver */
#if CONFIG_AUD_TRAS_MODE_CPU0
	LOGI("init aud_intf driver in CPU0 mode \r\n");
	ret = aud_tras_drv_init(&aud_intf_info.drv_info);
	if (ret != BK_OK) {
		LOGE("init aud_intf driver fail \r\n");
		goto aud_intf_drv_init_exit;
	}

	if (rtos_get_semaphore(&aud_intf_info.api_info.sem, 3000) != BK_OK) {
		LOGE("%s wait semaphore failed\n", __func__);
		err = BK_ERR_AUD_INTF_TIMEOUT;
		goto aud_intf_drv_init_exit;
	}

	if (aud_intf_info.api_info.result != BK_ERR_AUD_INTF_OK) {
		err = aud_intf_info.api_info.result;
		os_printf("err:%d \r\n", err);
		goto aud_intf_drv_init_exit;
	} else {
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_OK;
	}
#endif

#if CONFIG_AUD_TRAS_MODE_CPU1
	LOGI("init audio transfer in CPU1 mode \r\n");
	/* AUD_TRAS_STA_IDLE -> AUD_TRAS_STA_INITING */
	aud_intf_info.status = AUD_TRAS_STA_INITING;

	/* create audio transfer app task in cpu0 */
	if ((!aud_tras_demo_thread_hdl) && (!aud_tras_demo_msg_que)) {
		ret = rtos_init_queue(&aud_tras_demo_msg_que,
							  "audio_transfer_demo_queue",
							  sizeof(audio_tras_msg_t),
							  TU_QITEM_COUNT);
		if (ret != kNoErr) {
			LOGE("ceate audio internal message queue fail \r\n");
			goto audio_transfer_exit;
		}

		//creat audio transfer task
		ret = rtos_create_thread(&aud_tras_demo_thread_hdl,
							 3,
							 "audio_intf",
							 (beken_thread_function_t)audio_tras_demo_main,
							 4096,
							 0);
		if (ret != kNoErr) {
			LOGE("create audio transfer task fail \r\n");
			rtos_deinit_queue(&aud_tras_demo_msg_que);
			aud_tras_demo_msg_que = NULL;
			aud_tras_demo_thread_hdl = NULL;
			goto aud_intf_drv_init_exit;
		}
	} else {
		LOGE("task and queue is exist \r\n");
	}
#endif

aud_intf_drv_init_exit:
	rtos_deinit_semaphore(&aud_intf_info.api_info.sem);
	LOGE("init aud_intf driver fail \r\n");
	aud_intf_info.api_info.busy_status = false;

	return err;
}

bk_err_t bk_aud_intf_drv_deinit(void)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	CHECK_AUD_INTF_BUSY_STA();


/*
	if (aud_intf_info.status != AUD_INTF_STA_WORKING) {
		os_printf("audio transfer not working \r\n");
		return BK_OK;
	}
*/


#if CONFIG_AUD_TRAS_MODE_CPU0
	ret = aud_tras_drv_deinit();
	if (ret != BK_OK) {
		LOGE("deinit audio transfer fail \r\n");
		aud_intf_info.api_info.busy_status = false;
		err = BK_ERR_AUD_INTF_TX_MSG;
		goto aud_intf_drv_deinit_exit;
	}

	if (rtos_get_semaphore(&aud_intf_info.api_info.sem, 3000) != BK_OK) {
		LOGE("%s wait semaphore failed\n", __func__);
		err = BK_ERR_AUD_INTF_TIMEOUT;
		goto aud_intf_drv_deinit_exit;
	}

	if (aud_intf_info.api_info.result != BK_ERR_AUD_INTF_OK) {
		err = aud_intf_info.api_info.result;
		os_printf("err:%d \r\n", err);
		goto aud_intf_drv_deinit_exit;
	} else {
		rtos_deinit_semaphore(&aud_intf_info.api_info.sem);
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_OK;
	}
#endif

#if CONFIG_AUD_TRAS_MODE_CPU1
	/* send mailbox msg to CPU1 to stop audio transfer */
	ret = audio_tras_send_aud_mb_msg(AUD_TRAS_MB_CMD_STOP_TRANSFER, 0, 0, 0);
	if (ret != BK_OK) {
		LOGE("send msg to stop audio transfer fail \r\n");
		return BK_FAIL;
	}
#endif

aud_intf_drv_deinit_exit:
	aud_intf_info.api_info.busy_status = false;
	return err;
}

/* write speaker data in voice work mode */
static bk_err_t aud_intf_voc_write_spk_data(uint8_t *dac_buff, uint32_t size)
{
	uint32_t write_size = 0;

	//os_printf("enter: %s \r\n", __func__);

	/* check aud_intf status */
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	if (ring_buffer_get_free_size(aud_intf_info.voc_info.rx_info.decoder_rb) >= size) {
		write_size = ring_buffer_write(aud_intf_info.voc_info.rx_info.decoder_rb, dac_buff, size);
		if (write_size != size) {
			LOGE("write decoder_ring_buff fail, size:%d \r\n", size);
			return BK_FAIL;
		}
		aud_intf_info.voc_info.rx_info.rx_buff_seq_tail += size/(aud_intf_info.voc_info.rx_info.frame_size);
	}

	return BK_OK;
}

/* write speaker data in general work mode */
static bk_err_t aud_intf_genl_write_spk_data(uint8_t *dac_buff, uint32_t size)
{
	uint32_t write_size = 0;

	//os_printf("enter: %s \r\n", __func__);

	if (aud_intf_info.spk_status == AUD_INTF_SPK_STA_START) {
		if (ring_buffer_get_free_size(aud_intf_info.spk_info.spk_rx_rb) >= size) {
			write_size = ring_buffer_write(aud_intf_info.spk_info.spk_rx_rb, dac_buff, size);
			if (write_size != size) {
				LOGE("write spk_rx_ring_buff fail, write_size:%d \r\n", write_size);
				return BK_FAIL;
			}
		}
	}

	return BK_OK;
}

bk_err_t bk_aud_intf_write_spk_data(uint8_t *dac_buff, uint32_t size)
{
	bk_err_t ret = BK_OK;

	//os_printf("enter: %s \r\n", __func__);

	switch (aud_intf_info.drv_info.setup.work_mode) {
		case AUD_INTF_WORK_MODE_GENERAL:
			ret = aud_intf_genl_write_spk_data(dac_buff, size);
			break;

		case AUD_INTF_WORK_MODE_VOICE:
			ret = aud_intf_voc_write_spk_data(dac_buff, size);
			break;

		default:
			ret = BK_FAIL;
			break;
	}

	return ret;
}

