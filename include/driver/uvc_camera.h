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

#pragma once

#include <driver/uvc_camera_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief     Init the uvc
 *
 * This API init psram, dma, uvc param, and open uvc
 *
 * @param data configure for camera
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_driver_init(const uvc_camera_config_t * config);

/**
 * @brief     Deinit the uvc
 *
 * This API deinit dma and close uvc
 *
 * @attation: called after bk_uvc_camera_driver_init
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_driver_deinit(void);

/**
 * @brief     uvc power on
 *
 * This API called by user, before calling bk_uvc_camera_driver_init, you should power on uvc
 *
 * @attation 1. This api only work when CONFIG_DOORBALL_DEMO1=y
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_power_on(void);

/**
 * @brief     Get uvc config
 *
 * This API called by user, get uvc support fps and resolutions
 *
 * @param param the memory user malloc for save uvc support config
 *
 * @param count the param1 malloc size, the count config of uvc support
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_get_config(uvc_camera_device_t *param, uint16_t count);

/**
 * @brief     Set uvc config
 *
 * This API called by user, Set uvc support fps and resolutions
 *
 * @param config the uvc need support param the user set
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_set_config(uvc_camera_device_t * config);

/**
 * @brief     Set uvc start
 *
 * This API called by user, Set uvc start transfer
 *
 * @attation 1. this api called must after uvc have been init and have stopped
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_driver_start(void);

/**
 * @brief     Stop uvc start
 *
 * This API called by user, Set uvc stop transfer
 *
 * @attation 1. this api called must after uvc have been init and have start
 *
 * @return
 *    - kNoErr: succeed
 *    - others: other errors.
 */
bk_err_t bk_uvc_camera_driver_stop(void);

/**
 * @brief     Get current uvc device
 *
 * This API called by user, get currnet uvc device in working
 *
 * @attation 1. if no uvc work, the return will be NULL
 *
 * @return
 *    - uvc_camera_device_t *: current working uvc device ptr
 *    - NULL: no uvc device working
 */
uvc_camera_device_t *bk_uvc_camera_get_device(void);

#ifdef __cplusplus
}
#endif

