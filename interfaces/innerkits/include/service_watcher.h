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

#ifndef SERVICE_WATCH_API_H
#define SERVICE_WATCH_API_H

#include <unistd.h>
#include "init_param.h"
#include "service_control.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define INVALID_PID 0

typedef struct {
    ServiceStatus status;
    pid_t pid;
} ServiceInfo;

typedef void (*ServiceStatusChangePtr)(const char *key, const ServiceInfo *serviceInfo);
int ServiceWatchForStatus(const char *serviceName, ServiceStatusChangePtr changeCallback);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
