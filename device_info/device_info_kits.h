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

#ifndef OHOS_SYSTEM_DEVICE_ID_KITS_S
#define OHOS_SYSTEM_DEVICE_ID_KITS_S
#include <mutex>
#include "idevice_info.h"
#include "singleton.h"
#include "beget_ext.h"

namespace OHOS {
namespace device_info {
class INIT_LOCAL_API DeviceInfoKits final : public DelayedRefSingleton<DeviceInfoKits> {
    DECLARE_DELAYED_REF_SINGLETON(DeviceInfoKits);
public:
    DISALLOW_COPY_AND_MOVE(DeviceInfoKits);

    static DeviceInfoKits &GetInstance();
    int32_t GetUdid(std::string& result);
    int32_t GetSerialID(std::string& result);
    int32_t GetDiskSN(std::string& result);

    void FinishStartSASuccess(const sptr<IRemoteObject> &remoteObject);
    void FinishStartSAFailed();
    void ResetService(const wptr<IRemoteObject> &remote);
private:
    // For death event procession
    class DeathRecipient final : public IRemoteObject::DeathRecipient {
    public:
        DeathRecipient(void) = default;
        ~DeathRecipient(void) final = default;
        DISALLOW_COPY_AND_MOVE(DeathRecipient);
        void OnRemoteDied(const wptr<IRemoteObject> &remote) final;
    };
    sptr<IRemoteObject::DeathRecipient> GetDeathRecipient(void)
    {
        return deathRecipient_;
    }

    void LoadDeviceInfoSa(std::unique_lock<std::mutex> &lock);
    sptr<IDeviceInfo> GetService(std::unique_lock<std::mutex> &lock);
    std::mutex lock_;
    std::condition_variable deviceInfoLoadCon_;
    sptr<IRemoteObject::DeathRecipient> deathRecipient_ {};
    sptr<IDeviceInfo> deviceInfoService_ {};
    sptr<IDeviceInfo> RetryGetService(std::unique_lock<std::mutex> &lock);
};
} // namespace device_info
} // namespace OHOS

#ifndef DINFO_DOMAIN
#define DINFO_DOMAIN (BASE_DOMAIN + 8)
#endif

#ifndef DINFO_TAG
#define DINFO_TAG "DeviceInfoKits"
#endif

#define DINFO_LOGI(fmt, ...) STARTUP_LOGI(DINFO_DOMAIN, DINFO_TAG, fmt, ##__VA_ARGS__)
#define DINFO_LOGE(fmt, ...) STARTUP_LOGE(DINFO_DOMAIN, DINFO_TAG, fmt, ##__VA_ARGS__)
#define DINFO_LOGV(fmt, ...) STARTUP_LOGV(DINFO_DOMAIN, DINFO_TAG, fmt, ##__VA_ARGS__)

#define DINFO_CHECK(ret, exper, ...) \
    if (!(ret)) { \
        DINFO_LOGE(__VA_ARGS__); \
        exper; \
    }
#define DINFO_ONLY_CHECK(ret, exper, ...) \
    if (!(ret)) { \
        exper; \
    }

#endif // OHOS_SYSTEM_DEVICE_ID_KITS_S
