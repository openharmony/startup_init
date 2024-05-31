/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <mntent.h>
#include "securec.h"
#include "init_log.h"
#include "init_utils.h"
#include "fs_manager/fs_manager.h"
#include "erofs_mount_overlay.h"
#include "erofs_remount_overlay.h"
#include "remount_overlay.h"

#define REMOUNT_NONE 0
#define REMOUNT_SUCC 1
#define REMOUNT_FAIL 2
#define MODE_MKDIR 0755
#define BLOCK_SIZE_UNIT 4096

#define PREFIX_LOWER "/mnt/lower"
#define MNT_VENDOR "/vendor"
#define REMOUNT_RESULT_FLAG "/dev/remount/remount.result.done"
#define ROOT_MOUNT_DIR "/"
#define SYSTEM_DIR "/usr"

static int GetRemountResult(void)
{
    int ret;
    int fd = open(REMOUNT_RESULT_FLAG, O_RDONLY);
    if (fd >= 0) {
        char buff[1];
        ret = read(fd, buff, 1);
        if (ret < 0) {
            INIT_LOGE("read remount.result.done failed errno %d", errno);
            close(fd);
            return REMOUNT_FAIL;
        }
        close(fd);
        if (buff[0] == '0' + REMOUNT_SUCC) {
            return REMOUNT_SUCC;
        } else {
            return REMOUNT_FAIL;
        }
    }
    return REMOUNT_NONE;
}

static void SetRemountResultFlag(bool result)
{
    struct stat st;
    int ret;

    int statRet = stat("/dev/remount/", &st);
    if (statRet != 0) {
        ret = mkdir("/dev/remount/", MODE_MKDIR);
        if (ret < 0 && errno != EEXIST) {
            INIT_LOGE("mkdir /dev/remount failed errno %d", errno);
            return;
        }
    }

    int fd = open(REMOUNT_RESULT_FLAG, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        INIT_LOGE("open  /dev/remount/remount.result.done failed errno %d", errno);
        return;
    }

    char buff[1];
    if (result) {
        buff[0] = '0' + REMOUNT_SUCC;
    } else {
        buff[0] = '0' + REMOUNT_FAIL;
    }

    ret = write(fd, buff, 1);
    if (ret < 0) {
        INIT_LOGE("write buff failed errno %d", errno);
    }
    close(fd);
    INIT_LOGI("set remount result flag successfully");
}

static bool IsSkipRemount(const struct mntent mentry)
{
    if (mentry.mnt_type == NULL || mentry.mnt_dir == NULL) {
        return true;
    }

    if (strncmp(mentry.mnt_type, "erofs", strlen("erofs")) != 0) {
        return true;
    }

    if (strncmp(mentry.mnt_fsname, "/dev/block/dm-", strlen("/dev/block/dm-")) != 0) {
        return true;
    }
    return false;
}

static int ExecCommand(int argc, char **argv)
{
    INIT_CHECK(!(argc == 0 || argv == NULL || argv[0] == NULL), return -1);

    INIT_LOGI("Execute %s begin", argv[0]);
    pid_t pid = fork();
    INIT_ERROR_CHECK(pid >= 0, return -1, "Fork new process to format failed: %d", errno);

    if (pid == 0) {
        execv(argv[0], argv);
        exit(-1);
    }
    int status;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        INIT_LOGE("Command %s failed with status %d", argv[0], WEXITSTATUS(status));
    }
    INIT_LOGI("Execute %s end", argv[0]);
    return WEXITSTATUS(status);
}

static int GetDevSize(const char *fsBlkDev, uint64_t *devSize)
{
    int fd = -1;
    fd = open(fsBlkDev, O_RDONLY);
    if (fd < 0) {
        INIT_LOGE("open %s failed errno %d", fsBlkDev, errno);
        return -1;
    }

    if (ioctl(fd, BLKGETSIZE64, devSize) < 0) {
        INIT_LOGE("get block device [%s] size failed, errno %d", fsBlkDev, errno);
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int FormatExt4(const char *fsBlkDev, const char *fsMntPoint)
{
    uint64_t devSize;
    int ret = GetDevSize(fsBlkDev, &devSize);
    if (ret) {
        INIT_LOGE("get dev size failed.");
        return ret;
    }

    char blockSizeBuffer[MAX_BUFFER_LEN] = {0};
    if (snprintf_s(blockSizeBuffer, MAX_BUFFER_LEN, MAX_BUFFER_LEN - 1, "%llu", devSize / BLOCK_SIZE_UNIT) < 0) {
        BEGET_LOGE("Failed to copy nameRofs.");
        return -1;
    }

    const char *mke2fsArgs[] = {
        "/system/bin/mke2fs", "-t", "ext4", "-b", "4096", fsBlkDev, blockSizeBuffer, NULL
    };
    int mke2fsArgsLen = ARRAY_LENGTH(mke2fsArgs);
    ret = ExecCommand(mke2fsArgsLen, (char **)mke2fsArgs);
    if (ret) {
        INIT_LOGE("mke2fs failed returned %d", ret);
        return -1;
    }

    const char *e2fsdroidArgs[] = {
        "system/bin/e2fsdroid", "-e", "-a", fsMntPoint, fsBlkDev, NULL

    };
    int e2fsdroidArgsLen = ARRAY_LENGTH(e2fsdroidArgs);
    ret = ExecCommand(e2fsdroidArgsLen, (char **)e2fsdroidArgs);
    if (ret) {
        INIT_LOGE("e2fsdroid failed returned %d", ret);
    }
    return ret;
}

static void OverlayRemountPre(const char *mnt)
{
    if (strcmp(mnt, MNT_VENDOR) == 0) {
        OverlayRemountVendorPre();
    }
}

static void OverlayRemountPost(const char *mnt)
{
    if (strcmp(mnt, MNT_VENDOR) == 0) {
        OverlayRemountVendorPost();
    }
}

static bool DoRemount(struct mntent *mentry, bool *result)
{
    int devNum = 0;
    char *mnt = NULL;
    int ret = 0;
    ret = sscanf_s(mentry->mnt_fsname + strlen("/dev/block/dm-"), "%d", &devNum);
    if (ret < 0) {
        INIT_LOGE("get devNum failed returned");
        return false;
    }

    char devExt4[MAX_BUFFER_LEN] = {0};
    devNum = devNum + 1;
    if (snprintf_s(devExt4, MAX_BUFFER_LEN, MAX_BUFFER_LEN - 1, "/dev/block/dm-%d", devNum) < 0) {
        INIT_LOGE("Failed to copy devExt4.");
        return false;
    }

    if (strncmp(mentry->mnt_dir, PREFIX_LOWER, strlen(PREFIX_LOWER)) == 0) {
        mnt = mentry->mnt_dir + strlen(PREFIX_LOWER);
    } else {
        mnt = mentry->mnt_dir;
    }

    if (CheckIsExt4(devExt4, 0)) {
        INIT_LOGI("is ext4, not need format %s", devExt4);
    } else {
        ret = FormatExt4(devExt4, mnt);
        if (ret) {
            INIT_LOGE("Failed to format devExt4 %s.", devExt4);
            return false;
        }

        ret = MountExt4Device(devExt4, mnt, true);
        if (ret) {
            INIT_LOGE("Failed to mount devExt4 %s.", devExt4);
            return false;
        }
    }

    if (strncmp(mentry->mnt_dir, "/mnt/lower", strlen("/mnt/lower")) == 0) {
        return true;
    }

    OverlayRemountPre(mnt);
    if (MountOverlayOne(mnt)) {
        INIT_LOGE("Failed to mount overlay on mnt:%s.", mnt);
        return false;
    }
    OverlayRemountPost(mnt);
    *result = true;
    return true;
}

static bool DirectoryExists(const char *path)
{
    struct stat sb;
    return stat(path, &sb) != -1 && S_ISDIR(sb.st_mode);
}

int RootOverlaySetup(void)
{
    const char *rootOverlay = "/mnt/overlay/usr";
    const char *rootUpper = "/mnt/overlay/usr/upper";
    const char *rootWork = "/mnt/overlay/usr/work";
    char mntOpt[MAX_BUFFER_LEN] = {0};

    if (snprintf_s(mntOpt, MAX_BUFFER_LEN, MAX_BUFFER_LEN - 1,
        "upperdir=%s,lowerdir=/system,workdir=%s,override_creds=off", rootUpper, rootWork) < 0) {
        INIT_LOGE("copy mntOpt failed. errno %d", errno);
        return -1;
    }

    if (!DirectoryExists(rootOverlay)) {
        if (mkdir(rootOverlay, MODE_MKDIR) && (errno != EEXIST)) {
            INIT_LOGE("make dir failed on %s", rootOverlay);
            return -1;
        }

        if (mkdir(rootUpper, MODE_MKDIR) && (errno != EEXIST)) {
            INIT_LOGE("make dir failed on %s", rootUpper);
            return -1;
        }

        if (mkdir(rootWork, MODE_MKDIR) && (errno != EEXIST)) {
            INIT_LOGE("make dir failed on %s", rootWork);
            return -1;
        }
    }

    if (mount("overlay", "/system", "overlay", 0, mntOpt)) {
        INIT_LOGE("system mount overlay failed on %s", mntOpt);
        return -1;
    }
    INIT_LOGI("system mount overlay sucess");
    return 0;
}

static bool DoSystemRemount(struct mntent *mentry, bool *result)
{
    int devNum = 0;
    int ret = 0;
    ret = sscanf_s(mentry->mnt_fsname + strlen("/dev/block/dm-"), "%d", &devNum);
    if (ret < 0) {
        INIT_LOGE("get devNum failed returned");
        return false;
    }

    char devExt4[MAX_BUFFER_LEN] = {0};
    devNum = devNum + 1;
    if (snprintf_s(devExt4, MAX_BUFFER_LEN, MAX_BUFFER_LEN - 1, "/dev/block/dm-%d", devNum) < 0) {
        BEGET_LOGE("Failed to copy devExt4.");
        return false;
    }

    if (CheckIsExt4(devExt4, 0)) {
        INIT_LOGI("is ext4, not need format %s", devExt4);
    } else {
        ret = FormatExt4(devExt4, SYSTEM_DIR);
        if (ret) {
            INIT_LOGE("Failed to format devExt4 %s.", devExt4);
            return false;
        }

        ret = MountExt4Device(devExt4, SYSTEM_DIR, true);
        if (ret) {
            INIT_LOGE("Failed to mount devExt4 %s.", devExt4);
        }
    }

    if (RootOverlaySetup()) {
        INIT_LOGE("Failed to root overlay.");
        return false;
    }

    *result = true;
    return true;
}

bool RemountRofsOverlay()
{
    bool result = false;
    int lastRemountResult = GetRemountResult();
    INIT_LOGI("get last remount result is %d.", lastRemountResult);
    if (lastRemountResult != REMOUNT_NONE) {
        return (lastRemountResult == REMOUNT_SUCC) ? true : false;
    }
    FILE *fp;
    struct mntent *mentry = NULL;
    if ((fp = setmntent("/proc/mounts", "r")) == NULL) {
        INIT_LOGE("Failed to open /proc/mounts.");
        return false;
    }

    while (NULL != (mentry = getmntent(fp))) {
        if (IsSkipRemount(*mentry)) {
            INIT_LOGE("skip remount %s", mentry->mnt_dir);
            continue;
        }

        if (strcmp(mentry->mnt_dir, ROOT_MOUNT_DIR) == 0) {
            DoSystemRemount(mentry, &result);
            continue;
        }
        INIT_LOGI("do remount %s", mentry->mnt_dir);
        if (!DoRemount(mentry, &result)) {
            endmntent(fp);
            INIT_LOGE("do remount failed on %s", mentry->mnt_dir);
            return false;
        }
    }

    endmntent(fp);
    SetRemountResultFlag(result);
    return true;
}