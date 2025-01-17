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
#include <stdio.h>
#include <stdlib.h>
#include "sys_driver.h"
#include "aud_driver.h"
#include <components/aud_intf.h>
#include <components/aud_intf_types.h>
#include "ff.h"
#include "diskio.h"


static FIL spk_file;
static FIL mic_file;
static FIL voc_red_file;
static FIL voc_ply_file;
static char mic_file_name[50];
static char spk_file_name[50];
static char voc_record_file_name[50];
static char voc_play_file_name[50];
static aud_intf_drv_setup_t aud_intf_drv_setup;
static aud_intf_mic_setup_t aud_intf_mic_setup;
static aud_intf_spk_setup_t aud_intf_spk_setup;
static aud_intf_voc_setup_t aud_intf_voc_setup;
static aud_intf_work_mode_t aud_work_mode = AUD_INTF_WORK_MODE_NULL;
static int32_t *temp_spk_addr = NULL;     //存放从SDcard中取的pcm信号
static bool spk_file_empty = false;


extern void delay(int num);

static void cli_aud_intf_help(void)
{
	os_printf("aud_intf_record_test {init|start|pause|stop|set_chl|deinit xxx.pcm|mic1|dual} \r\n");
	os_printf("aud_intf_play_test {init|start|pause|stop|set_chl|deinit xxx.pcm|left|dual} \r\n");
	os_printf("aud_intf_sd_voc_test {start|stop xx.pcm, xx.pcm} \r\n");
	os_printf("aud_intf_set_voc_param_test {param value} \r\n");
	os_printf("aud_intf_set_aec_param_test {param value} \r\n");
	os_printf("aud_intf_get_aec_param_test \r\n");
	os_printf("aud_intf_set_samp_rate_test {param value} \r\n");
	os_printf("aud_intf_doorbell_test {start|stop} \r\n");
}

static int send_mic_data_to_sd(uint8_t *data, unsigned int len)
{
	//bk_err_t ret = BK_OK;
	FRESULT fr;
	uint32 uiTemp = 0;

	//os_printf("[send_mic_data_to_sd] \r\n");

	/* write data to file */
	fr = f_write(&mic_file, (void *)data, len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", mic_file_name);
	}

	return uiTemp;
}

void cli_aud_intf_record_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	FRESULT fr;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		os_printf("init record test \r\n");

		/*open file to save pcm data */
		sprintf(mic_file_name, "1:/%s", argv[2]);
		fr = f_open(&mic_file, mic_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", mic_file_name);
			return;
		}

		aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		aud_intf_drv_setup.task_config.priority = 3;
		aud_intf_drv_setup.aud_intf_rx_spk_data = NULL;
		aud_intf_drv_setup.aud_intf_tx_mic_data = send_mic_data_to_sd;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_init complete \r\n");
		}

		aud_work_mode = AUD_INTF_WORK_MODE_GENERAL;
		ret = bk_aud_intf_set_mode(aud_work_mode);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		aud_intf_mic_setup.mic_chl = AUD_INTF_MIC_CHL_MIC1;
		aud_intf_mic_setup.samp_rate = AUD_ADC_SAMP_RATE_8K;
		aud_intf_mic_setup.frame_size = 320;
		aud_intf_mic_setup.mic_gain = 0x2d;
		ret = bk_aud_intf_mic_init(&aud_intf_mic_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_init complete \r\n");
		}

		os_printf("init mic record complete \r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_aud_intf_mic_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_start complete \r\n");
		}
		os_printf("start mic record \r\n");
	} else if (os_strcmp(argv[1], "pause") == 0) {
		ret = bk_aud_intf_mic_pause();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_start complete \r\n");
		}
		os_printf("pause mic record \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_mic_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_stop complete \r\n");
		}

		os_printf("stop mic record \r\n");
	} else if (os_strcmp(argv[1], "set_chl") == 0) {
		if (os_strcmp(argv[2], "mic1") == 0) {
			ret = bk_aud_intf_set_mic_chl(AUD_INTF_MIC_CHL_MIC1);
		} else if (os_strcmp(argv[2], "dual") == 0) {
			ret = bk_aud_intf_set_mic_chl(AUD_INTF_MIC_CHL_DUAL);
		} else {
			os_printf("mic channel is error \r\n");
			return;
		}

		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mic_chl fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mic_chl complete \r\n");
		}

		os_printf("change mic channel \r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_aud_intf_mic_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_mic_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_mic_deinit complete \r\n");
		}

		spk_file_empty = false;

		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_deinit complete \r\n");
		}

		/* close mic file */
		fr = f_close(&mic_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", mic_file_name);
			return;
		}
		os_printf("stop mic record \r\n");

		os_printf("stop record test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}

static bk_err_t read_spk_data_from_sd(unsigned int size)
{
	bk_err_t ret = BK_OK;
	FRESULT fr;
	uint32 uiTemp = 0;

	if (spk_file_empty)
		return BK_FAIL;

	temp_spk_addr = os_malloc(size);

	/* read data from file */
	fr = f_read(&spk_file, (void *)temp_spk_addr, size, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", spk_file_name);
	}

	if (uiTemp != size) {
		spk_file_empty = true;
		os_printf("the %s is empty \r\n", spk_file_name);
	}

	/* write a fram speaker data to speaker_ring_buff */
	ret = bk_aud_intf_write_spk_data((uint8_t*)temp_spk_addr, size);
	if (ret != BK_OK) {
		os_printf("write spk data fail \r\n");
		return ret;
	}

	os_free(temp_spk_addr);

	return ret;
}

void cli_aud_intf_play_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;
	FRESULT fr;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "init") == 0) {
		os_printf("init play test \r\n");

		/*open file to read pcm data */
		sprintf(spk_file_name, "1:/%s", argv[2]);
		fr = f_open(&spk_file, spk_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", spk_file_name);
			return;
		}

		aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		aud_intf_drv_setup.task_config.priority = 3;
		aud_intf_drv_setup.aud_intf_rx_spk_data = read_spk_data_from_sd;
		aud_intf_drv_setup.aud_intf_tx_mic_data = NULL;
		ret = bk_aud_intf_drv_init(&aud_intf_drv_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_init complete \r\n");
		}

		aud_work_mode = AUD_INTF_WORK_MODE_GENERAL;
		ret = bk_aud_intf_set_mode(aud_work_mode);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_mode fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_mode complete \r\n");
		}

		aud_intf_spk_setup.spk_chl = AUD_INTF_SPK_CHL_LEFT;
		aud_intf_spk_setup.samp_rate = AUD_DAC_SAMP_RATE_8K;
		aud_intf_spk_setup.frame_size = 640;
		aud_intf_spk_setup.spk_gain = 0x2d;
		aud_intf_spk_setup.work_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		ret = bk_aud_intf_spk_init(&aud_intf_spk_setup);
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_init fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_init complete \r\n");
		}

		os_printf("init play \r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_aud_intf_spk_start();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_start fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_start complete \r\n");
		}

		os_printf("start play \r\n");
	} else if (os_strcmp(argv[1], "pause") == 0) {
		ret = bk_aud_intf_spk_pause();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_pause fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_pause complete \r\n");
		}

		os_printf("pause play \r\n");
	} else if (os_strcmp(argv[1], "set_chl") == 0) {
		if (os_strcmp(argv[2], "left") == 0) {
			ret = bk_aud_intf_set_spk_chl(AUD_INTF_SPK_CHL_LEFT);
		} else if (os_strcmp(argv[2], "dual") == 0) {
			ret = bk_aud_intf_set_spk_chl(AUD_INTF_SPK_CHL_DUAL);
		} else {
			os_printf("spk channel is error \r\n");
			return;
		}

		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_set_spk_chl fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_set_spk_chl complete \r\n");
		}

		os_printf("change spk channel \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_spk_stop();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_stop fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_stop complete \r\n");
		}

		spk_file_empty = false;

		/* close spk file */
		fr = f_close(&spk_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", spk_file_name);
			return;
		}

		fr = f_open(&spk_file, spk_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", spk_file_name);
			return;
		}

		os_printf("stop play \r\n");
	} else if (os_strcmp(argv[1], "deinit") == 0) {
		ret = bk_aud_intf_spk_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_spk_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_spk_deinit complete \r\n");
		}

		ret = bk_aud_intf_drv_deinit();
		if (ret != BK_ERR_AUD_INTF_OK) {
			os_printf("bk_aud_intf_drv_deinit fail, ret:%d \r\n", ret);
		} else {
			os_printf("bk_aud_intf_drv_deinit complete \r\n");
		}

		spk_file_empty = false;

		/* close spk file */
		fr = f_close(&spk_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", spk_file_name);
			return;
		}

		os_printf("deinit play \r\n");
		os_printf("deinit play test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}


static bk_err_t aud_write_sd_data_to_spk(unsigned int size)
{
	bk_err_t ret = BK_OK;
	FRESULT fr;
	uint32 uiTemp = 0;

	//os_printf("enter %s \r\n", __func__);

	if (spk_file_empty)
		return BK_FAIL;

	temp_spk_addr = os_malloc(size);

	/* read data from file */
	fr = f_read(&voc_ply_file, (void *)temp_spk_addr, size, &uiTemp);
	if (fr != FR_OK) {
		os_printf("read %s fail.\r\n", voc_play_file_name);
	}

	if (uiTemp != size) {
		spk_file_empty = true;
		os_printf("the %s is empty \r\n", voc_play_file_name);
		//TODO
	}

	/* write a fram speaker data to speaker_ring_buff */
	ret = bk_aud_intf_write_spk_data((uint8_t*)temp_spk_addr, size);
	if (ret != BK_OK) {
		os_printf("write voc spk data fail \r\n");
		return ret;
	}

	os_free(temp_spk_addr);

	return ret;
}

static int aud_voc_write_mic_to_sd(UINT8 *data, UINT32 len)
{
	FRESULT fr;
	uint32 uiTemp = 0;

	//os_printf("[send_mic_data_to_sd] \r\n");

	/* write data to file */
	fr = f_write(&voc_red_file, (void *)data, len, &uiTemp);
	if (fr != FR_OK) {
		os_printf("write %s fail.\r\n", voc_record_file_name);
	}

	return uiTemp;
}

/* audio voice transfer pcm data by sdcard */
void cli_aud_intf_sd_voc_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	FRESULT fr;
	bk_err_t ret = BK_OK;

	if (argc != 4) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "start") == 0) {
		os_printf("start voice transfer test \r\n");

		/*open file to save voice mic data */
		sprintf(voc_record_file_name, "1:/%s", argv[2]);
		fr = f_open(&voc_red_file, voc_record_file_name, FA_CREATE_ALWAYS | FA_WRITE);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", voc_record_file_name);
			return;
		}

		/*open file to read and write voice spk data */
		sprintf(voc_play_file_name, "1:/%s", argv[3]);
		fr = f_open(&voc_ply_file, voc_play_file_name, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK) {
			os_printf("open %s fail.\r\n", voc_play_file_name);
			return;
		}

		aud_intf_drv_setup.work_mode = AUD_INTF_WORK_MODE_NULL;
		aud_intf_drv_setup.task_config.priority = 3;
		aud_intf_drv_setup.aud_intf_rx_spk_data = aud_write_sd_data_to_spk;
		aud_intf_drv_setup.aud_intf_tx_mic_data = aud_voc_write_mic_to_sd;
		bk_aud_intf_drv_init(&aud_intf_drv_setup);
		aud_work_mode = AUD_INTF_WORK_MODE_VOICE;
		bk_aud_intf_set_mode(aud_work_mode);
		aud_intf_voc_setup.aec_enable = true;
		aud_intf_voc_setup.samp_rate = AUD_INTF_VOC_SAMP_RATE_8K;
		//aud_intf_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_G711A;
		aud_intf_voc_setup.data_type = AUD_INTF_VOC_DATA_TYPE_PCM;
		aud_intf_voc_setup.spk_mode = AUD_DAC_WORK_MODE_SIGNAL_END;
		aud_intf_voc_setup.mic_gain = 0x2d;
		aud_intf_voc_setup.spk_gain = 0x2d;
		aud_intf_voc_setup.aec_cfg.ec_depth = 20;
		aud_intf_voc_setup.aec_cfg.TxRxThr = 30;
		aud_intf_voc_setup.aec_cfg.TxRxFlr = 6;
		aud_intf_voc_setup.aec_cfg.ns_level = 2;
		aud_intf_voc_setup.aec_cfg.ns_para = 1;
		bk_aud_intf_voc_init(aud_intf_voc_setup);

		temp_spk_addr = os_malloc(640);
		os_memset(temp_spk_addr, 0, 640);
		rtos_delay_milliseconds(2000);
		os_printf("start write spk data \r\n");
		for (int i = 0; i < 10; i++) {
			/* write a fram speaker data to speaker_ring_buff */
			ret = bk_aud_intf_write_spk_data((uint8_t*)temp_spk_addr, 640);
			if (ret != BK_OK) {
				os_printf("write voc spk data fail \r\n");
				return;
			}
		}
		os_free(temp_spk_addr);

		os_printf("start voice \r\n");
	} else if (os_strcmp(argv[1], "stop") == 0) {
		bk_aud_intf_voc_deinit();
		aud_work_mode = AUD_INTF_WORK_MODE_NULL;
		bk_aud_intf_set_mode(aud_work_mode);
		bk_aud_intf_drv_deinit();

		/* close voice record file */
		fr = f_close(&voc_red_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", voc_record_file_name);
			return;
		}

		/* close voice play file */
		fr = f_close(&voc_ply_file);
		if (fr != FR_OK) {
			os_printf("close %s fail!\r\n", voc_play_file_name);
			return;
		}

		os_printf("stop voice \r\n");
		os_printf("stop voice test \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}

void cli_aud_intf_set_voc_param_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	uint8_t value = 0;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	value = strtoul(argv[2], NULL, 0);
	if (os_strcmp(argv[1], "mic_gain") == 0) {
		bk_aud_intf_set_mic_gain(value);
		os_printf("set voc mic gain:%d \r\n", value);
	} else if (os_strcmp(argv[1], "spk_gain") == 0) {
		bk_aud_intf_set_spk_gain(value);
		os_printf("set voc spk gain:%d \r\n", value);
	} else if (os_strcmp(argv[1], "gain") == 0) {
		os_printf("stop play \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

}

void cli_aud_intf_set_aec_param_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	uint8_t value = 0;
	aud_intf_voc_aec_para_t op;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	value = strtoul(argv[2], NULL, 0);
	if (os_strcmp(argv[1], "init_flags") == 0) {
		op = AUD_INTF_VOC_AEC_INIT_FLAG;
		os_printf("set aec init_flags:%d \r\n", value);
	} else if (os_strcmp(argv[1], "mic_delay") == 0) {
		op = AUD_INTF_VOC_AEC_MIC_DELAY;
		os_printf("set aec mic_delay:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ec_depth") == 0) {
		op = AUD_INTF_VOC_AEC_EC_DEPTH;
		os_printf("set aec ec_depth:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ref_scale") == 0) {
		op = AUD_INTF_VOC_AEC_REF_SCALE;
		os_printf("set aec ref_scale:%d \r\n", value);
	} else if (os_strcmp(argv[1], "voice_vol") == 0) {
		op = AUD_INTF_VOC_AEC_VOICE_VOL;
		os_printf("set aec voice_vol:%d \r\n", value);
	} else if (os_strcmp(argv[1], "TxRxThr") == 0) {
		op = AUD_INTF_VOC_AEC_TXRX_THR;
		os_printf("set aec TxRxThr:%d \r\n", value);
	} else if (os_strcmp(argv[1], "TxRxFlr") == 0) {
		op = AUD_INTF_VOC_AEC_TXRX_FLR;
		os_printf("set aec TxRxFlr:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ns_level") == 0) {
		op = AUD_INTF_VOC_AEC_NS_LEVEL;
		os_printf("set aec ns_level:%d \r\n", value);
	} else if (os_strcmp(argv[1], "ns_para") == 0) {
		op = AUD_INTF_VOC_AEC_NS_PARA;
		os_printf("set aec ns_para:%d \r\n", value);
	} else if (os_strcmp(argv[1], "drc") == 0) {
		op = AUD_INTF_VOC_AEC_DRC;
		os_printf("set aec drc:%d \r\n", value);
	} else {
		cli_aud_intf_help();
		return;
	}
	ret = bk_aud_intf_set_aec_para(op, value);
	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("set aec parameters complete \r\n");
}

void cli_aud_intf_get_aec_param_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc != 1) {
		cli_aud_intf_help();
		return;
	}

	ret = bk_aud_intf_get_aec_para();
	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("get aec parameters complete \r\n");
}

/* set mic and speaker sample rate */
void cli_aud_intf_set_samp_rate_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;
	aud_adc_samp_rate_t adc_samp_rate;
	aud_dac_samp_rate_t dac_samp_rate;

	if (argc != 3) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "mic") == 0) {
		if (os_strcmp(argv[2], "8K") == 0) {
			adc_samp_rate = AUD_ADC_SAMP_RATE_8K;
		} else if (os_strcmp(argv[2], "16K") == 0) {
			adc_samp_rate = AUD_ADC_SAMP_RATE_16K;
		} else if (os_strcmp(argv[2], "44K") == 0) {
			adc_samp_rate = AUD_ADC_SAMP_RATE_44_1K;
		} else if (os_strcmp(argv[2], "48K") == 0) {
			adc_samp_rate = AUD_ADC_SAMP_RATE_48K;
		} else {
			os_printf("the param is error \r\n");
			return;
		}
		ret = bk_aud_intf_set_mic_samp_rate(adc_samp_rate);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("set mic samp rate:%d fail \r\n", adc_samp_rate);
		os_printf("set mic samp rate:%d complete \r\n", adc_samp_rate);
	} else if (os_strcmp(argv[1], "spk") == 0) {
		if (os_strcmp(argv[2], "8K") == 0) {
			dac_samp_rate = AUD_ADC_SAMP_RATE_8K;
		} else if (os_strcmp(argv[2], "16K") == 0) {
			dac_samp_rate = AUD_ADC_SAMP_RATE_16K;
		} else if (os_strcmp(argv[2], "44K") == 0) {
			dac_samp_rate = AUD_ADC_SAMP_RATE_44_1K;
		} else if (os_strcmp(argv[2], "48K") == 0) {
			dac_samp_rate = AUD_ADC_SAMP_RATE_48K;
		} else {
			os_printf("the param is error \r\n");
			return;
		}
		ret = bk_aud_intf_set_spk_samp_rate(dac_samp_rate);
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("set spk samp rate:%d fail, ret:%d \r\n", dac_samp_rate, ret);
		os_printf("set spk samp rate:%d complete \r\n", dac_samp_rate);

	} else {
		cli_aud_intf_help();
		return;
	}

	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("set samp rate complete \r\n");
}

/* set mic and speaker sample rate */
void cli_aud_intf_doorbell_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_err_t ret = BK_OK;

	if (argc != 2) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "stop") == 0) {
		ret = bk_aud_intf_voc_stop();
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("stop doorbell fail \r\n");
		os_printf("stop doorbell complete \r\n");
	} else if (os_strcmp(argv[1], "start") == 0) {
		ret = bk_aud_intf_voc_start();
		if (ret != BK_ERR_AUD_INTF_OK)
			os_printf("start doorbell fail \r\n");
		os_printf("start doorbell complete \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}

	if (ret != BK_OK)
		os_printf("test fail \r\n");

	os_printf("set samp rate complete \r\n");
}

#if CONFIG_AUD_TRAS_DAC_DEBUG
void cli_aud_intf_debug_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc != 2) {
		cli_aud_intf_help();
		return;
	}

	if (os_strcmp(argv[1], "on") == 0) {
		os_printf("turn on voc dac debug \r\n");
		bk_aud_intf_set_voc_dac_debug(true);
	} else if (os_strcmp(argv[1], "off") == 0) {
		bk_aud_intf_set_voc_dac_debug(false);
		os_printf("turn off voc dac debug \r\n");
	} else {
		cli_aud_intf_help();
		return;
	}
}
#endif
