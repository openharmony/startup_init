/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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
#include "init_service.h"

#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "init.h"
#include "init_log.h"
#include "init_service_manager.h"
#include "securec.h"

#ifdef ENABLE_PROCESS_PRIORITY
#include <sys/resource.h>
#define MIN_IMPORTANT_LEVEL (-20)
#define MAX_IMPORTANT_LEVEL (19)
#endif

void NotifyServiceChange(Service *service, int status)
{
    UNUSED(service);
    UNUSED(status);
}

int IsForbidden(const char *fieldStr)
{
    size_t fieldLen = strlen(fieldStr);
    size_t forbidStrLen = strlen(BIN_SH_NOT_ALLOWED);
    if (fieldLen == forbidStrLen) {
        if (strncmp(fieldStr, BIN_SH_NOT_ALLOWED, fieldLen) == 0) {
            return 1;
        }
        return 0;
    } else if (fieldLen > forbidStrLen) {
        // "/bin/shxxxx" is valid but "/bin/sh xxxx" is invalid
        if (strncmp(fieldStr, BIN_SH_NOT_ALLOWED, forbidStrLen) == 0) {
            if (fieldStr[forbidStrLen] == ' ') {
                return 1;
            }
        }
        return 0;
    } else {
        return 0;
    }
}

int SetImportantValue(Service *service, const char *attrName, int value, int flag)
{
    UNUSED(attrName);
    UNUSED(flag);
    INIT_ERROR_CHECK(service != NULL, return SERVICE_FAILURE, "Set service attr failed! null ptr.");
#ifdef ENABLE_PROCESS_PRIORITY
    if (value >= MIN_IMPORTANT_LEVEL && value <= MAX_IMPORTANT_LEVEL) { // -20~19
        service->attribute |= SERVICE_ATTR_IMPORTANT;
        service->importance = value;
    } else {
        INIT_LOGE("Importance level = %d, is not between -20 and 19, error", value);
        return SERVICE_FAILURE;
    }
#else
    if (value != 0) {
        service->attribute |= SERVICE_ATTR_IMPORTANT;
    }
#endif
    return SERVICE_SUCCESS;
}

int ServiceExec(Service *service, const ServiceArgs *pathArgs)
{
    INIT_ERROR_CHECK(service != NULL, return SERVICE_FAILURE, "Exec service failed! null ptr.");
    INIT_LOGI("ServiceExec %s", service->name);
    INIT_ERROR_CHECK(pathArgs != NULL && pathArgs->count > 0,
        return SERVICE_FAILURE, "Exec service failed! null ptr.");

#ifdef ENABLE_PROCESS_PRIORITY
    if (service->importance != 0) {
        INIT_ERROR_CHECK(setpriority(PRIO_PROCESS, 0, service->importance) == 0,
            service->lastErrno = INIT_EPRIORITY;
            return SERVICE_FAILURE,
            "Service error %d %s, failed to set priority %d.", errno, service->name, service->importance);
    }
#endif
    int isCritical = (service->attribute & SERVICE_ATTR_CRITICAL);
    INIT_ERROR_CHECK(execv(pathArgs->argv[0], pathArgs->argv) == 0,
        service->lastErrno = INIT_EEXEC;
        return errno, "[startup_failed]failed to execv %d %d %s", isCritical, errno, service->name);
    return SERVICE_SUCCESS;
}

int SetAccessToken(const Service *service)
{
    return SERVICE_SUCCESS;
}

void GetAccessToken(void)
{
    return;
}

void IsEnableSandbox(void)
{
    return;
}

int SetServiceEnterSandbox(const Service *service, const char *path)
{
    UNUSED(path);
    UNUSED(service);
    return 0;
}
