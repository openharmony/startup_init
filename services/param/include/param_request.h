
/*
 * Copyright (c) 2020 Huawei Device Co., Ltd.
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

#ifndef BASE_STARTUP_PARAM_REQUEST_H
#define BASE_STARTUP_PARAM_REQUEST_H
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include "param_manager.h"
#include "sys_param.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct {
    ParamWorkSpace paramSpace;
    int clientFd;
    pthread_mutex_t mutex;
} ClientWorkSpace;

int WatchParamCheck(const char *keyprefix);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
