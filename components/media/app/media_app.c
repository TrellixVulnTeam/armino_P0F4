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
#include <os/str.h>
#include <os/mem.h>
#include <components/log.h>
#include <common/bk_include.h>
#include <driver/int.h>
#include <driver/dvp_camera_types.h>

#include <components/video_transfer.h>


#include "media_core.h"
#include "media_evt.h"
#include "media_app.h"
#include "transfer_act.h"
#include "camera_act.h"
#include "lcd_act.h"


#define TAG "media_app"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static beken_thread_t media_app_th_hd = NULL;
static beken_queue_t media_app_msg_queue = NULL;
static app_camera_type_t app_camera_type = APP_CAMERA_INVALIED;

extern void rwnxl_set_video_transfer_flag(uint32_t video_transfer_flag);

bk_err_t media_send_msg_sync(uint32_t event, uint32_t param)
{
	int ret = kGeneralErr;
	param_pak_t *param_pak = (param_pak_t *) os_malloc(sizeof(param_pak_t));
	media_msg_t msg;

	if (param_pak == NULL)
	{
		LOGE("%s malloc param_pak_t failed\n", __func__);
		goto out;
	}

	ret = rtos_init_semaphore_ex(&param_pak->sem, 1, 0);

	if (ret != kNoErr)
	{
		LOGE("%s init semaphore failed\n", __func__);
		goto out;
	}

	param_pak->param = param;

	msg.event = event;
	msg.param = (uint32_t)param_pak;

	media_send_msg(&msg);

	ret = rtos_get_semaphore(&param_pak->sem, BEKEN_WAIT_FOREVER);

	if (ret != kNoErr)
	{
		LOGE("%s wait semaphore failed\n", __func__);
		goto out;
	}

	ret = param_pak->ret;

	rtos_deinit_semaphore(&param_pak->sem);

out:

	if (param_pak)
	{
		os_free(param_pak);
	}

	return ret;
}


bk_err_t media_app_camera_open(app_camera_type_t type, media_ppi_t ppi)
{
	int ret = kGeneralErr;

	LOGI("%s\n", __func__);

	app_camera_type = type;

	if (type == APP_CAMERA_DVP)
	{
		if (CAMERA_STATE_DISABLED != get_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_DVP_JPEG_OPEN_IND, ppi);
	}
	else if (type == APP_CAMERA_YUV)
	{
		if (CAMERA_STATE_DISABLED != get_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_DVP_YUV_OPEN_IND, ppi);
	}
	else if (type == APP_CAMERA_MIX)
	{
		if (CAMERA_STATE_DISABLED != get_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_DVP_MIX_OPEN_IND, ppi);
	}
	else if (type == APP_CAMERA_UVC)
	{

#if (CONFIG_USB_UVC)
		if (CAMERA_STATE_DISABLED != get_camera_state())
		{
			LOGI("%s already opened\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_UVC_OPEN_IND, ppi);
#endif
	}

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_camera_close(app_camera_type_t type)
{
	int ret = kGeneralErr;

	LOGI("%s\n", __func__);

	if (app_camera_type != type)
	{
		LOGE("The camera type of open and close not match!\n");
		return ret;
	}

	if (type == APP_CAMERA_DVP)
	{
		if (CAMERA_STATE_ENABLED != get_camera_state())
		{
			LOGI("%s already closed\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_DVP_CLOSE_IND, DVP_MODE_JPG);
	}
	else if (type == APP_CAMERA_YUV)
	{
		if (CAMERA_STATE_ENABLED != get_camera_state())
		{
			LOGI("%s already closed\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_DVP_CLOSE_IND, DVP_MODE_YUV);
	}
	else if (type == APP_CAMERA_UVC)
	{


#if (CONFIG_USB_UVC)

		if (CAMERA_STATE_DISABLED == get_camera_state())
		{
			LOGI("%s already closed\n", __func__);
			return kNoErr;
		}

		ret = media_send_msg_sync(EVENT_CAM_UVC_CLOSE_IND, 0);
#endif
	}

	app_camera_type = APP_CAMERA_INVALIED;

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_mailbox_test(void)
{
	bk_err_t ret = BK_FAIL;
	uint32_t param = 0x88888888;

	LOGI("%s +++\n", __func__);

	ret = media_send_msg_sync(EVENT_LCD_DEFAULT_CMD, param);

	LOGI("%s ---\n", __func__);

	return ret;
}

bk_err_t media_app_transfer_open(void *setup_cfg)
{
	int ret = kNoErr;
	video_setup_t *ptr = NULL;

	LOGI("%s, %p\n", __func__, ((video_setup_t *)setup_cfg)->send_func);

	rwnxl_set_video_transfer_flag(true);

	if (TRS_STATE_DISABLED != get_transfer_state())
	{
		LOGI("%s already opened\n", __func__);
		return ret;
	}

	ptr = (video_setup_t *)os_malloc(sizeof(video_setup_t));
	os_memcpy(ptr, (video_setup_t *)setup_cfg, sizeof(video_setup_t));

	ret = media_send_msg_sync(EVENT_TRANSFER_OPEN_IND, (uint32_t)ptr);

	os_free(ptr);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_transfer_pause(bool pause)
{
	return media_send_msg_sync(EVENT_TRANSFER_PAUSE_IND, pause);
}

bk_err_t media_app_transfer_close(void)
{
	bk_err_t ret;

	LOGI("%s\n", __func__);

	if (TRS_STATE_ENABLED != get_transfer_state())
	{
		LOGI("%s already closed\n", __func__);
		return kNoErr;
	}

	ret = media_send_msg_sync(EVENT_TRANSFER_CLOSE_IND, 0);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_lcd_rotate(bool enable)
{
	bk_err_t ret;

	LOGI("%s\n", __func__);

	ret = media_send_msg_sync(EVENT_LCD_ROTATE_ENABLE_IND, enable);

	LOGI("%s complete\n", __func__);

	return ret;
}


static bk_err_t _media_app_lcd_open(void *lcd_open, app_lcd_open_type_t is_open_with_gui)
{
	int ret = kNoErr;
	lcd_open_t *ptr = NULL;

//	if (LCD_STATE_ENABLED == get_lcd_state())
//	{
//		LOGI("%s already opened\n", __func__);
//		return ret;
//	}
	LOGI("%s \n", __func__);

	ptr = (lcd_open_t *)os_malloc(sizeof(lcd_open_t));
	os_memcpy(ptr, (lcd_open_t *)lcd_open, sizeof(lcd_open_t));

	if(!is_open_with_gui)
		ret = media_send_msg_sync(EVENT_LCD_OPEN_IND, (uint32_t)ptr);
	else
		ret = media_send_msg_sync(EVENT_LCD_OPEN_WITH_GUI, (uint32_t)ptr);

	os_free(ptr);

	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_lcd_display_beken(void)
{
	bk_err_t ret;
	
	ret = media_send_msg_sync(EVENT_LCD_BEKEN_LOGO_DISPLAY, 0);
	LOGI("%s complete\n", __func__);

	return ret;
}

bk_err_t media_app_lcd_open(void *lcd_open)
{
	return _media_app_lcd_open(lcd_open, APP_LCD_NO_USE_GUI);
}

bk_err_t media_app_lcd_open_withgui(void *lcd_open)
{
	return _media_app_lcd_open(lcd_open, APP_LCD_USE_GUI);
}


bk_err_t media_app_lcd_display(char *name, uint32_t lcd_ppi)
{
	bk_err_t ret;
	char *capture_name = NULL;

	LOGI("%s\n", __func__);
	if (name != NULL)
	{
		uint32_t len = os_strlen(name) + 1;

		if (len > 31)
		{
			len = 31;
		}

		capture_name = (char *)os_malloc(len);
		os_memset(capture_name, 0, len);
		os_memcpy(capture_name, name, len);
		capture_name[len - 1] = '\0';
	}

	//ret = media_send_msg_sync(EVENT_LCD_OPEN_IND, (uint32_t)lcd_ppi);

	ret = media_send_msg_sync(EVENT_LCD_DISPLAY_IND, (uint32_t)capture_name);

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_lcd_close(void)
{
	bk_err_t ret;

	LOGI("%s\n", __func__);

	if (LCD_STATE_ENABLED == get_lcd_state() || LCD_STATE_DISPLAY == get_lcd_state())
	{
		ret = media_send_msg_sync(EVENT_LCD_CLOSE_IND, 0);
	}
	else
	{
		LOGI("%s already closed\n", __func__);
		return BK_OK;
	}

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_lcd_set_backlight(uint8_t level)
{
	bk_err_t ret;

	LOGI("%s\n", __func__);

	if (LCD_STATE_ENABLED != get_lcd_state())
	{
		LOGI("%s not open\n", __func__);
		return kNoErr;
	}

	ret = media_send_msg_sync(EVENT_LCD_SET_BACKLIGHT_IND, level);

	LOGI("%s complete\n", __func__);

	return ret;
}


bk_err_t media_app_send_msg(media_msg_t *msg)
{
	bk_err_t ret;

	if (media_app_msg_queue)
	{
		ret = rtos_push_to_queue(&media_app_msg_queue, msg, BEKEN_NO_WAIT);

		if (kNoErr != ret)
		{
			LOGE("%s failed\n", __func__);
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

bk_err_t media_app_capture(char *name)
{
	int ret;
	char *capture_name = NULL;

	LOGI("%s, %s\n", __func__, name);

	if (name != NULL)
	{
		uint32_t len = os_strlen(name) + 1;

		if (len > 31)
		{
			len = 31;
		}

		capture_name = (char *)os_malloc(len);
		os_memset(capture_name, 0, len);
		os_memcpy(capture_name, name, len);
		capture_name[len - 1] = '\0';
	}

	ret = media_send_msg_sync(EVENT_STORAGE_CAPTURE_IND, (uint32_t)capture_name);
	os_free(capture_name);

	LOGI("%s complete\n", __func__);

	return ret;
}


static void media_app_message_handle(void)
{
	bk_err_t ret = BK_OK;
	media_msg_t msg;

	while (1)
	{
		ret = rtos_pop_from_queue(&media_app_msg_queue, &msg, BEKEN_WAIT_FOREVER);

		if (kNoErr == ret)
		{
			switch (msg.event)
			{
				case EVENT_APP_DVP_OPEN:
					break;

				case EVENT_APP_EXIT:
					goto exit;
					break;

				default:
					break;
			}
		}
	}

exit:

	/* delate msg queue */
	ret = rtos_deinit_queue(&media_app_msg_queue);

	if (ret != kNoErr)
	{
		LOGE("delate message queue fail\n");
	}

	media_app_msg_queue = NULL;

	LOGE("delate message queue complete\n");

	/* delate task */
	media_app_th_hd = NULL;
	rtos_delete_thread(NULL);

	LOGE("delate task complete\n");
}


bk_err_t media_app_init(void)
{
	bk_err_t ret = BK_OK;

	if (media_app_msg_queue != NULL)
	{
		ret = kNoErr;
		LOGE("%s, media_app_msg_queue allready init, exit!\n");
		goto error;
	}

	if (media_app_th_hd != NULL)
	{
		ret = kNoErr;
		LOGE("%s, media_app_th_hd allready init, exit!\n");
		goto error;
	}

	ret = rtos_init_queue(&media_app_msg_queue,
	                      "media_app_msg_queue",
	                      sizeof(media_msg_t),
	                      MEDIA_MINOR_MSG_QUEUE_SIZE);

	if (ret != kNoErr)
	{
		LOGE("%s, ceate media minor message queue failed\n");
		goto error;
	}

	ret = rtos_create_thread(&media_app_th_hd,
	                         BEKEN_DEFAULT_WORKER_PRIORITY,
	                         "media_app_thread",
	                         (beken_thread_function_t)media_app_message_handle,
	                         4096,
	                         NULL);

	if (ret != kNoErr)
	{
		LOGE("create media app thread fail\n");
		goto error;
	}


	LOGI("media app thread startup complete\n");

	return kNoErr;
error:

	if (media_app_msg_queue)
	{
		rtos_deinit_queue(&media_app_msg_queue);
		media_app_msg_queue = NULL;
	}

	return ret;
}

bk_err_t media_app_dump_display_frame(void)
{
	return media_send_msg_sync(EVENT_LCD_DUMP_DISPLAY_IND, 0);
}

bk_err_t media_app_dump_decoder_frame(void)
{
	return media_send_msg_sync(EVENT_LCD_DUMP_DECODER_IND, 0);
}

bk_err_t media_app_dump_jpeg_frame(void)
{
	return media_send_msg_sync(EVENT_LCD_DUMP_JPEG_IND, 0);
}

bk_err_t media_app_lcd_step_mode(bool enable)
{
	return media_send_msg_sync(EVENT_LCD_STEP_MODE_IND, enable);
}

bk_err_t media_app_lcd_step_trigger(void)
{
	return media_send_msg_sync(EVENT_LCD_STEP_TRIGGER_IND, 0);
}

