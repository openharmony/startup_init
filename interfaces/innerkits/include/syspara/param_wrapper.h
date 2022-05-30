/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef STARTUP_SYSVERSION_API_H
#define STARTUP_SYSVERSION_API_H

#include <string>

namespace OHOS {
namespace system {
int GetStringParameter(const std::string key, std::string &value, const std::string def = "");
int GetIntParameter(const std::string key, int def);
}
}

#endif // STARTUP_SYSVERSION_API_H