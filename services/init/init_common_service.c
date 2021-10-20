/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __MUSL__
#include <stropts.h>
#endif
#include <sys/capability.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "init.h"
#include "init_adapter.h"
#include "init_cmds.h"
#include "init_log.h"
#include "init_service_socket.h"
#include "init_utils.h"
#include "securec.h"

#ifndef TIOCSCTTY
#define TIOCSCTTY 0x540E
#endif
// 240 seconds, 4 minutes
static const int CRASH_TIME_LIMIT = 240;
// maximum number of crashes within time CRASH_TIME_LIMIT for one service
static const int CRASH_COUNT_LIMIT = 4;

// 240 seconds, 4 minutes
static const int CRITICAL_CRASH_TIME_LIMIT = 240;
// maximum number of crashes within time CRITICAL_CRASH_TIME_LIMIT for one service
static const int CRITICAL_CRASH_COUNT_LIMIT = 4;

static int SetAllAmbientCapability(void)
{
    for (int i = 0; i <= CAP_LAST_CAP; ++i) {
        if (SetAmbientCapability(i) != 0) {
            return SERVICE_FAILURE;
        }
    }
    return SERVICE_SUCCESS;
}

static int SetPerms(const Service *service)
{
    INIT_CHECK_RETURN_VALUE(KeepCapability() == 0, SERVICE_FAILURE);
    if (service->servPerm.gIDCnt > 0) {
        INIT_ERROR_CHECK(setgid(service->servPerm.gIDArray[0]) == 0, return SERVICE_FAILURE,
            "SetPerms, setgid for %s failed. %d", service->name, errno);
    }
    if (service->servPerm.gIDCnt > 1) {
        INIT_ERROR_CHECK(setgroups(service->servPerm.gIDCnt - 1, &service->servPerm.gIDArray[1]) == 0,
             return SERVICE_FAILURE,
            "SetPerms, setgroups failed. errno = %d, gIDCnt=%d", errno, service->servPerm.gIDCnt);
    }
    if (service->servPerm.uID != 0) {
        if (setuid(service->servPerm.uID) != 0) {
            INIT_LOGE("setuid of service: %s failed, uid = %d", service->name, service->servPerm.uID);
            return SERVICE_FAILURE;
        }
    }

    // umask call always succeeds and return the previous mask value which is not needed here
    (void)umask(DEFAULT_UMASK_INIT);

    struct __user_cap_header_struct capHeader;
    capHeader.version = _LINUX_CAPABILITY_VERSION_3;
    capHeader.pid = 0;
    struct __user_cap_data_struct capData[CAP_NUM] = {};
    for (unsigned int i = 0; i < service->servPerm.capsCnt; ++i) {
        if (service->servPerm.caps[i] == FULL_CAP) {
            for (int j = 0; j < CAP_NUM; ++j) {
                capData[j].effective = FULL_CAP;
                capData[j].permitted = FULL_CAP;
                capData[j].inheritable = FULL_CAP;
            }
            break;
        }
        capData[CAP_TO_INDEX(service->servPerm.caps[i])].effective |= CAP_TO_MASK(service->servPerm.caps[i]);
        capData[CAP_TO_INDEX(service->servPerm.caps[i])].permitted |= CAP_TO_MASK(service->servPerm.caps[i]);
        capData[CAP_TO_INDEX(service->servPerm.caps[i])].inheritable |= CAP_TO_MASK(service->servPerm.caps[i]);
    }

    if (capset(&capHeader, capData) != 0) {
        INIT_LOGE("capset faild for service: %s, error: %d", service->name, errno);
        return SERVICE_FAILURE;
    }
    for (unsigned int i = 0; i < service->servPerm.capsCnt; ++i) {
        if (service->servPerm.caps[i] == FULL_CAP) {
            return SetAllAmbientCapability();
        }
        if (SetAmbientCapability(service->servPerm.caps[i]) != 0) {
            INIT_LOGE("SetAmbientCapability faild for service: %s", service->name);
            return SERVICE_FAILURE;
        }
    }
    return SERVICE_SUCCESS;
}

static void OpenConsole(void)
{
    const int stdError = 2;
    setsid();
    WaitForFile("/dev/console", WAIT_MAX_COUNT);
    int fd = open("/dev/console", O_RDWR);
    if (fd >= 0) {
        ioctl(fd, TIOCSCTTY, 0);
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, stdError); // Redirect fd to 0, 1, 2
        close(fd);
    } else {
        INIT_LOGE("Open /dev/console failed. err = %d", errno);
    }
    return;
}

static int WritePid(const Service *service)
{
    const int maxPidStrLen = 50;
    char pidString[maxPidStrLen];
    pid_t childPid = getpid();
    int len = snprintf_s(pidString, maxPidStrLen, maxPidStrLen - 1, "%d", childPid);
    if (len <= 0) {
        INIT_LOGE("Failed to format pid for service %s", service->name);
        return SERVICE_FAILURE;
    }
    for (int i = 0; i < service->writePidArgs.count; i++) {
        if (service->writePidArgs.argv[i] == NULL) {
            continue;
        }
        FILE *fd = NULL;
        char *realPath = GetRealPath(service->writePidArgs.argv[i]);
        if (realPath != NULL) {
            fd = fopen(realPath, "wb");
        } else {
            fd = fopen(service->writePidArgs.argv[i], "wb");
        }
        if (fd != NULL) {
            if ((int)fwrite(pidString, 1, len, fd) != len) {
                INIT_LOGE("Failed to write %s pid:%s", service->writePidArgs.argv[i], pidString);
            }
            (void)fclose(fd);
        } else {
            INIT_LOGE("Failed to open %s.", service->writePidArgs.argv[i]);
        }
        if (realPath != NULL) {
            free(realPath);
        }
        INIT_LOGD("ServiceStart writepid filename=%s, childPid=%s, ok", service->writePidArgs.argv[i], pidString);
    }
    return SERVICE_SUCCESS;
}

int ServiceStart(Service *service)
{
    INIT_ERROR_CHECK(service != NULL, return SERVICE_FAILURE, "start service failed! null ptr.");
    INIT_ERROR_CHECK(service->pid <= 0, return SERVICE_SUCCESS, "service : %s had started already.", service->name);
    INIT_ERROR_CHECK(service->pathArgs.count > 0,
        return SERVICE_FAILURE, "start service %s pathArgs is NULL.", service->name);
    if (service->attribute & SERVICE_ATTR_INVALID) {
        INIT_LOGE("start service %s invalid.", service->name);
        return SERVICE_FAILURE;
    }
    struct stat pathStat = { 0 };
    service->attribute &= (~(SERVICE_ATTR_NEED_RESTART | SERVICE_ATTR_NEED_STOP));
    if (stat(service->pathArgs.argv[0], &pathStat) != 0) {
        service->attribute |= SERVICE_ATTR_INVALID;
        INIT_LOGE("start service %s invalid, please check %s.", service->name, service->pathArgs.argv[0]);
        return SERVICE_FAILURE;
    }
    int pid = fork();
    if (pid == 0) {
        int ret = CreateServiceSocket(service->socketCfg);
        if (ret < 0) {
            INIT_LOGE("service %s exit! create socket failed!", service->name);
            _exit(PROCESS_EXIT_CODE);
        }
        if (service->attribute & SERVICE_ATTR_CONSOLE) {
            OpenConsole();
        }
        // permissions
        if (SetPerms(service) != SERVICE_SUCCESS) {
            INIT_LOGE("service %s exit! set perms failed! err %d.", service->name, errno);
            _exit(PROCESS_EXIT_CODE);
        }
        // write pid
        if (WritePid(service) != SERVICE_SUCCESS) {
            INIT_LOGE("service %s exit! write pid failed!", service->name);
            _exit(PROCESS_EXIT_CODE);
        }
        ServiceExec(service);
        _exit(PROCESS_EXIT_CODE);
    } else if (pid < 0) {
        INIT_LOGE("start service %s fork failed!", service->name);
        return SERVICE_FAILURE;
    }
    INIT_LOGI("service %s starting pid %d", service->name, pid);
    service->pid = pid;
    NotifyServiceChange(service->name, "running");
    return SERVICE_SUCCESS;
}

int ServiceStop(Service *service)
{
    INIT_ERROR_CHECK(service != NULL, return SERVICE_FAILURE, "stop service failed! null ptr.");
    service->attribute &= ~SERVICE_ATTR_NEED_RESTART;
    service->attribute |= SERVICE_ATTR_NEED_STOP;
    if (service->pid <= 0) {
        return SERVICE_SUCCESS;
    }
    CloseServiceSocket(service->socketCfg);
    if (kill(service->pid, SIGKILL) != 0) {
        INIT_LOGE("stop service %s pid %d failed! err %d.", service->name, service->pid, errno);
        return SERVICE_FAILURE;
    }
    NotifyServiceChange(service->name, "stopping");
    INIT_LOGI("stop service %s, pid %d.", service->name, service->pid);
    return SERVICE_SUCCESS;
}

static bool CalculateCrashTime(Service *service, int crashTimeLimit, int crashCountLimit)
{
    INIT_ERROR_CHECK(service != NULL && crashTimeLimit > 0 && crashCountLimit > 0, return 0,
        "Service name=%s, input params error.", service->name);
    time_t curTime = time(NULL);
    if (service->crashCnt == 0) {
        service->firstCrashTime = curTime;
        ++service->crashCnt;
    } else if (difftime(curTime, service->firstCrashTime) > crashTimeLimit) {
        service->firstCrashTime = curTime;
        service->crashCnt = 1;
    } else {
        ++service->crashCnt;
        if (service->crashCnt > crashCountLimit) {
            return false;
        }
    }
    return true;
}

static int ExecRestartCmd(const Service *service)
{
    INIT_ERROR_CHECK(service != NULL, return SERVICE_FAILURE, "Exec service failed! null ptr.");
    INIT_ERROR_CHECK(service->restartArg != NULL, return SERVICE_FAILURE, "restartArg is null");

    for (int i = 0; i < service->restartArg->cmdNum; i++) {
        INIT_LOGI("ExecRestartCmd cmdLine->cmdContent %s ", service->restartArg->cmds[i].cmdContent);
        DoCmdByIndex(service->restartArg->cmds[i].cmdIndex, service->restartArg->cmds[i].cmdContent);
    }
    free(service->restartArg);
    return SERVICE_SUCCESS;
}

void ServiceReap(Service *service)
{
    INIT_CHECK(service != NULL, return);
    INIT_LOGI("Reap service %s, pid %d.", service->name, service->pid);
    service->pid = -1;
    NotifyServiceChange(service->name, "stopped");

    if (service->attribute & SERVICE_ATTR_INVALID) {
        INIT_LOGE("Reap service %s invalid.", service->name);
        return;
    }

    CloseServiceSocket(service->socketCfg);
    // stopped by system-init itself, no need to restart even if it is not one-shot service
    if (service->attribute & SERVICE_ATTR_NEED_STOP) {
        service->attribute &= (~SERVICE_ATTR_NEED_STOP);
        service->crashCnt = 0;
        return;
    }

    // for one-shot service
    if (service->attribute & SERVICE_ATTR_ONCE) {
        // no need to restart
        if (!(service->attribute & SERVICE_ATTR_NEED_RESTART)) {
            service->attribute &= (~SERVICE_ATTR_NEED_STOP);
            return;
        }
        // the service could be restart even if it is one-shot service
    }

    if (service->attribute & SERVICE_ATTR_CRITICAL) { // critical
        if (CalculateCrashTime(service, CRITICAL_CRASH_TIME_LIMIT, CRITICAL_CRASH_COUNT_LIMIT) == false) {
            INIT_LOGE("Critical service \" %s \" crashed %d times, rebooting system",
                service->name, CRITICAL_CRASH_COUNT_LIMIT);
            ExecReboot("reboot");
        }
    } else if (!(service->attribute & SERVICE_ATTR_NEED_RESTART)) {
        if (CalculateCrashTime(service, CRASH_TIME_LIMIT, CRASH_COUNT_LIMIT) == false) {
            INIT_LOGE("Service name=%s, crash %d times, no more start.", service->name, CRASH_COUNT_LIMIT);
            return;
        }
    }

    int ret = 0;
    if (service->restartArg != NULL) {
        ret = ExecRestartCmd(service);
        if (ret != SERVICE_SUCCESS) {
            INIT_LOGE("Failed to exec restartArg for %s", service->name);
        }
    }
    ret = ServiceStart(service);
    if (ret != SERVICE_SUCCESS) {
        INIT_LOGE("reap service %s start failed!", service->name);
    }
    service->attribute &= (~SERVICE_ATTR_NEED_RESTART);
}
