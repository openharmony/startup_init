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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "init_reboot.h"

#define REBOOT_CMD_NUMBER 2
#define USAGE_INFO "usage: reboot shutdown\n"\
    "       reboot updater\n"\
    "       reboot updater[:options]\n" \
    "       reboot flash\n" \
    "       reboot flash[:options]\n" \
    "       reboot\n"

int main(int argc, char* argv[])
{
    if (argc > REBOOT_CMD_NUMBER) {
        printf("%s", USAGE_INFO);
        return 0;
    }

    if (argc == REBOOT_CMD_NUMBER && strcmp(argv[1], "shutdown") != 0 &&
        strcmp(argv[1], "updater") != 0 &&
        strcmp(argv[1], "flash") != 0 &&
        strncmp(argv[1], "updater:", strlen("updater:")) != 0 &&
        strncmp(argv[1], "flash:", strlen("flash:")) != 0) {
        printf("%s", USAGE_INFO);
        return 0;
    }
    int ret = 0;
    if (argc == REBOOT_CMD_NUMBER) {
        ret = DoReboot(argv[1]);
    } else {
        ret = DoReboot(NULL);
    }
    if (ret != 0) {
        printf("[reboot command] DoReboot Api return error\n");
    } else {
        printf("[reboot command] DoReboot Api return ok\n");
    }
    while (1) {
        pause();
    }
    return 0;
}

