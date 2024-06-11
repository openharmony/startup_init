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

#include <stdio.h>
#ifdef EROFS_OVERLAY
#include "init_log.h"
#include "erofs_remount_overlay.h"
#include "remount_overlay.h"
#endif

int main(int argc, const char *argv[])
{
#ifdef EROFS_OVERLAY
    EnableInitLog(INIT_INFO);
    if (getuid() == 0 && IsOverlayEnable()) {
        bool ret = RemountRofsOverlay();
        printf("remount %s\n", ret ? "success" : "failed");
        return ret ? 0 : 1;
    }
    printf("not need erofs overlay, remount success\n");
#endif
    return 0;
}

