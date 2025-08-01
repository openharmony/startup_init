/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef INIT_FSTAB_MOUNT_TEST_H
#define INIT_FSTAB_MOUNT_TEST_H
#include "fs_manager/fs_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

int MountWithCheckpoint(const char *source, const char *target,
    const char *fsType, unsigned long flags, const char *data);

int GetDataWithoutCheckpoint(char *fsSpecificData, size_t fsSpecificDataSize,
    char *checkpointData, size_t checkpointDataSize);

int DoMountOneItem(FstabItem *item);

#ifdef __cplusplus
}
#endif
#endif