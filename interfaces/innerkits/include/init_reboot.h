/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INIT_REBOOT_API_H
#define INIT_REBOOT_API_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define STARTUP_DEVICE_CTL "startup.device.ctl"
#define DEVICE_CMD_STOP "stop"
#define DEVICE_CMD_SUSPEND "suspend"
#define DEVICE_CMD_RESUME "resume"
#define DEVICE_CMD_FREEZE "freeze"
#define DEVICE_CMD_RESTORE "restore"

/**
 * @brief reboot system
 *
 * @param option format is: mode[:info]
 *  mode : command sub key, example: updater, shutdown ...
 *  info : extend info
 * @return int
 */
int DoReboot(const char *cmdContent);

/**
 * @brief reboot system
 *
 * @param mode command sub key, example: updater, shutdown ...
 * @param option extend info
 *
 * @return int
 */
int DoRebootExt(const char *mode, const char *option);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
