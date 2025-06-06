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
#include "beget_ext.h"
#ifdef PARAM_FEATURE_DEVICEINFO
#include "device_info_kits.h"
#endif
#include "param_comm.h"
#include "securec.h"
#include "sysparam_errno.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

int AclGetDevUdid(char *udid, int size)
{
    if (udid == nullptr || size < UDID_LEN) {
        return SYSPARAM_INVALID_INPUT;
    }
    (void)memset_s(udid, size, 0, size);
#ifdef PARAM_FEATURE_DEVICEINFO
    std::string result = {};
    OHOS::device_info::DeviceInfoKits &instance = OHOS::device_info::DeviceInfoKits::GetInstance();
    int ret = instance.GetUdid(result);
    if (ret == 0) {
        ret = strcpy_s(udid, size, result.c_str());
    }
#else
    int ret = GetDevUdid_(udid, size);
#endif
    return ret;
}

const char *AclGetSerial(void)
{
    static char serialNumber[MAX_SERIAL_LEN] = {0};
#ifdef PARAM_FEATURE_DEVICEINFO
    std::string result = {};
    OHOS::device_info::DeviceInfoKits &instance = OHOS::device_info::DeviceInfoKits::GetInstance();
    int ret = instance.GetSerialID(result);
    if (ret == 0) {
        ret = strcpy_s(serialNumber, sizeof(serialNumber), result.c_str());
        BEGET_ERROR_CHECK(ret == 0, return nullptr, "Failed to copy");
    }
#else
    const char *tmpSerial = GetSerial_();
    if (tmpSerial != nullptr) {
        int ret = strcpy_s(serialNumber, sizeof(serialNumber), tmpSerial);
        BEGET_ERROR_CHECK(ret == 0, return nullptr, "Failed to copy");
    }
#endif
    return serialNumber;
}

int AclGetDiskSN(char *diskSN, int size)
{
    if (diskSN == nullptr || size < DISK_SN_LEN) {
        return SYSPARAM_INVALID_INPUT;
    }
    (void)memset_s(diskSN, size, 0, size);
    int ret = 0;
#ifdef PARAM_FEATURE_GET_DEVICE_SN
    std::string result = {};
    OHOS::device_info::DeviceInfoKits &instance = OHOS::device_info::DeviceInfoKits::GetInstance();
    ret = instance.GetDiskSN(result);
    if (ret == 0) {
        ret = strcpy_s(diskSN, size, result.c_str());
    }
#endif
    return ret;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */
