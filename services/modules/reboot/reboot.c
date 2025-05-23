/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <unistd.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/syscall.h>

#include "reboot_adp.h"
#include "init_cmdexecutor.h"
#include "init_module_engine.h"
#include "init_utils.h"
#include "plugin_adapter.h"
#include "securec.h"
#ifndef OHOS_LITE
#include "init_hisysevent.h"
#endif

#define BUFF_SIZE 256
#define POWEROFF_REASON_DEV_PATH    "/proc/poweroff_reason"

static int WritePowerOffReason(const char* reason)
{
    PLUGIN_CHECK(reason != NULL, return -1, "WritePowerOffReason: reason is NULL\n");
    PLUGIN_CHECK(access(POWEROFF_REASON_DEV_PATH, F_OK) == 0, return -1,
                "WritePowerOffReason: access %s failed, errno = %d, %s\n",
                POWEROFF_REASON_DEV_PATH, errno, strerror(errno));
    int fd = open(POWEROFF_REASON_DEV_PATH, O_RDWR);
    PLUGIN_CHECK(fd > 0, return -1, "WritePowerOffReason: errno = %d, %s\n", errno, strerror(errno));
    int writeBytes = strlen(reason);
    int ret = write(fd, reason, writeBytes);
    PLUGIN_CHECK(ret == writeBytes, writeBytes = -1, "WritePowerOffReason: write poweroff reason failed\n");
    close(fd);
    return writeBytes;
}

static void ParseRebootReason(const char *name, int argc, const char **argv)
{
    char str[BUFF_SIZE] = {0};
    int len = sizeof(str);
    char *tmp = str;
    int ret;
    for (int i = 0; i < argc; i++) {
        if (i != argc - 1) {
            ret = sprintf_s(tmp, len - 1, "%s ", argv[i]);
        } else {
            ret = sprintf_s(tmp, len - 1, "%s", argv[i]);
        }
        if (ret <= 0) {
            PLUGIN_LOGW("ParseRebootReason: sprintf_s arg %s failed!", argv[i]);
            break;
        }
        len -= ret;
        tmp += ret;
    }
    ret = WritePowerOffReason(str);
    PLUGIN_CHECK(ret >= 0, return, "ParseRebootReason: write poweroff reason failed\n");
}

PLUGIN_STATIC int DoRoot_(const char *jobName, int type)
{
    // by job to stop service and unmount
    if (jobName != NULL) {
        DoJobNow(jobName);
    }
#ifndef STARTUP_INIT_TEST
    return reboot(type);
#else
    return 0;
#endif
}

static int DoReboot(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    // clear misc
    (void)UpdateMiscMessage(NULL, "reboot", NULL, NULL);
    return DoRoot_("reboot", RB_AUTOBOOT);
}

static int DoRebootPanic(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    char str[BUFF_SIZE] = {0};
    int ret = sprintf_s(str, sizeof(str) - 1, "panic caused by %s:", name);
    if (ret <= 0) {
        PLUGIN_LOGW("DoRebootPanic sprintf_s name %s failed!", name);
    }

    int len = ret > 0 ? (sizeof(str) - ret) : sizeof(str);
    char *tmp = str + (sizeof(str) - len);
    for (int i = 0; i < argc; ++i) {
        ret = sprintf_s(tmp, len - 1, " %s", argv[i]);
        if (ret <= 0) {
            PLUGIN_LOGW("DoRebootPanic sprintf_s arg %s failed!", argv[i]);
            break;
        } else {
            len -= ret;
            tmp += ret;
        }
    }
    PLUGIN_LOGI("DoRebootPanic %s", str);
    ParseRebootReason(name, argc, argv);
    if (InRescueMode() == 0) {
        PLUGIN_LOGI("Don't panic in resuce mode!");
        return 0;
    }
    // clear misc
    (void)UpdateMiscMessage(NULL, "reboot", NULL, NULL);
    DoJobNow("reboot");
#ifndef STARTUP_INIT_TEST
    FILE *panic = fopen("/proc/sysrq-trigger", "wb");
    if (panic == NULL) {
        return reboot(RB_AUTOBOOT);
    }
    if (fwrite((void *)"c", 1, 1, panic) != 1) {
        (void)fclose(panic);
        PLUGIN_LOGI("fwrite to panic failed");
        return -1;
    }
    (void)fclose(panic);
#endif
    return 0;
}

PLUGIN_STATIC int DoRebootShutdown(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
    PLUGIN_CHECK(argc >= 1, return -1, "Invalid parameter");
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    // clear misc
    (void)UpdateMiscMessage(NULL, "reboot", NULL, NULL);
    const size_t len = strlen("reboot,");
    const char *cmd = strstr(argv[0], "reboot,");
    if (cmd != NULL && strlen(cmd) > len) {
        PLUGIN_LOGI("DoRebootShutdown argv %s", cmd + len);
        // by job to stop service and unmount
        DoJobNow("reboot");
#ifndef STARTUP_INIT_TEST
        return syscall(__NR_reboot,
            LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_POWER_OFF, cmd + len);
#else
        return 0;
#endif
    }
    return DoRoot_("reboot", RB_POWER_OFF);
}

static int DoRebootUpdater(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
    PLUGIN_LOGI("DoRebootUpdater argc %d %s", argc, name);
    PLUGIN_CHECK(argc >= 1, return -1, "Invalid parameter");
    PLUGIN_LOGI("DoRebootUpdater argv %s", argv[0]);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    int ret = UpdateMiscMessage(argv[0], "updater", "updater:", "boot_updater");
    if (ret == 0) {
        return DoRoot_("reboot", RB_AUTOBOOT);
    }
    return ret;
}

static int DoRebootPenglai(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
    PLUGIN_LOGI("DoRebootPenglai argc %d %s", argc, name);
    PLUGIN_CHECK(argc >= 1, return -1, "Invalid parameter");
    PLUGIN_LOGI("DoRebootPenglai argv %s", argv[0]);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    int ret = UpdateMiscMessage(argv[0], "penglai", "penglai:", "boot_penglai");
    if (ret == 0) {
        return DoRoot_("reboot", RB_AUTOBOOT);
    }
    return ret;
}

PLUGIN_STATIC int DoRebootFlashed(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
    PLUGIN_LOGI("DoRebootFlashed argc %d %s", argc, name);
    PLUGIN_CHECK(argc >= 1, return -1, "Invalid parameter");
    PLUGIN_LOGI("DoRebootFlashd argv %s", argv[0]);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    int ret = UpdateMiscMessage(argv[0], "flash", "flash:", "boot_flash");
    if (ret == 0) {
        return DoRoot_("reboot", RB_AUTOBOOT);
    }
    return ret;
}

PLUGIN_STATIC int DoRebootCharge(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    int ret = UpdateMiscMessage(NULL, "charge", "charge:", "boot_charge");
    if (ret == 0) {
        return DoRoot_("reboot", RB_AUTOBOOT);
    }
    return ret;
}

static int DoRebootSuspend(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    ParseRebootReason(name, argc, argv);
    return DoRoot_("suspend", RB_AUTOBOOT);
}

PLUGIN_STATIC int DoRebootOther(int id, const char *name, int argc, const char **argv)
{
    UNUSED(id);
    PLUGIN_CHECK(argc >= 1, return -1, "Invalid parameter argc %d", argc);
#ifndef OHOS_LITE
    ReportStartupReboot(argv[0]);
#endif
    const char *cmd = strstr(argv[0], "reboot,");
    PLUGIN_CHECK(cmd != NULL, return -1, "Invalid parameter argc %s", argv[0]);
    PLUGIN_LOGI("DoRebootOther argv %s", argv[0]);
    ParseRebootReason(name, argc, argv);
    // clear misc
    (void)UpdateMiscMessage(NULL, "reboot", NULL, NULL);
    DoJobNow("reboot");
#ifndef STARTUP_INIT_TEST
    return syscall(__NR_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
        LINUX_REBOOT_CMD_RESTART2, cmd + strlen("reboot,"));
#else
    return 0;
#endif
}

static void RebootAdpInit(void)
{
    // add default reboot cmd
    (void)AddCmdExecutor("reboot", DoReboot);
    (void)AddCmdExecutor("reboot.other", DoRebootOther);
    AddRebootCmdExecutor("shutdown", DoRebootShutdown);
    AddRebootCmdExecutor("flashd", DoRebootFlashed);
    AddRebootCmdExecutor("updater", DoRebootUpdater);
    AddRebootCmdExecutor("charge", DoRebootCharge);
    AddRebootCmdExecutor("suspend", DoRebootSuspend);
    AddRebootCmdExecutor("panic", DoRebootPanic);
    AddRebootCmdExecutor("penglai", DoRebootPenglai);
    (void)AddCmdExecutor("panic", DoRebootPanic);
}

MODULE_CONSTRUCTOR(void)
{
    PLUGIN_LOGI("Reboot adapter plug-in init now ...");
    RebootAdpInit();
}

MODULE_DESTRUCTOR(void)
{
    PLUGIN_LOGI("Reboot adapter plug-in exit now ...");
}
