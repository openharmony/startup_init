/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "addwatcher_fuzzer.h"
#include <string>
#include <memory>
#include "watcher_manager.h"
#include "securec.h"

using namespace OHOS::init_param;
namespace OHOS {
     bool FuzzAddWatcher(const uint8_t* data, size_t size)
    {
        if (size < sizeof(uint32_t)) {
            return false;
        }
        std::unique_ptr<WatcherManager> watcherManager = std::make_unique<WatcherManager>(0, true);
        std::string str(reinterpret_cast<const char*>(data), size);
        uint32_t randomValue = 0;
        errno_t ret = memcpy_s(&randomValue, sizeof(randomValue), data, sizeof(uint32_t));
        if (ret != 0) {
            return false;
        }
        if (!watcherManager->AddWatcher(str, randomValue)) {
            return true;
        };
        return false;
    }
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::FuzzAddWatcher(data, size);
    return 0;
}