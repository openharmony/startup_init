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

#ifndef BASE_STARTUP_INIT_PARAM_H
#define BASE_STARTUP_INIT_PARAM_H
#include <stdint.h>
#include <stdio.h>
#ifdef PARAM_SUPPORT_TRIGGER
#include "cJSON.h"
#endif
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define DEFAULT_PARAM_WAIT_TIMEOUT 30 // 30s
#define DEFAULT_PARAM_SET_TIMEOUT 15 // 10s

#ifndef PARAM_NAME_LEN_MAX
#define PARAM_CONST_VALUE_LEN_MAX 4096
#define PARAM_VALUE_LEN_MAX  96
#define PARAM_NAME_LEN_MAX  96
#endif

typedef enum {
    PARAM_CODE_ERROR = -1,
    PARAM_CODE_SUCCESS = 0,
    PARAM_CODE_INVALID_PARAM = 100,
    PARAM_CODE_INVALID_NAME,
    PARAM_CODE_INVALID_VALUE,
    PARAM_CODE_REACHED_MAX,
    PARAM_CODE_NOT_SUPPORT,
    PARAM_CODE_TIMEOUT,
    PARAM_CODE_NOT_FOUND,
    PARAM_CODE_READ_ONLY,
    PARAM_CODE_IPC_ERROR,
    PARAM_CODE_NODE_EXIST,
    PARAM_WATCHER_CALLBACK_EXIST,
    PARAM_WATCHER_GET_SERVICE_FAILED,
    PARAM_CODE_MEMORY_MAP_FAILED,
    PARAM_WORKSPACE_NOT_INIT,
    PARAM_CODE_FAIL_CONNECT,
    PARAM_CODE_MEMORY_NOT_ENOUGH,
    PARAM_CODE_PERMISSION_DENIED,
    PARAM_DEFAULT_PARAM_MEMORY_NOT_ENOUGH,
    DAC_RESULT_INVALID_PARAM = 1000,
    DAC_RESULT_FORBIDED,
    SELINUX_RESULT_FORBIDED,
    PARAM_CODE_MAX
} PARAM_CODE;

typedef enum {
    EVENT_TRIGGER_PARAM,
    EVENT_TRIGGER_BOOT,
    EVENT_TRIGGER_PARAM_WAIT,
    EVENT_TRIGGER_PARAM_WATCH
} EventType;

#define LOAD_PARAM_NORMAL 0x00
#define LOAD_PARAM_ONLY_ADD 0x01
#define LOAD_PARAM_PERSIST 0x02
#define LOAD_PARAM_UPDATE_CONST 0x04

typedef uint32_t ParamHandle;

/**
 * Init 接口
 * 初始化参数服务
 *
 */
int InitParamService(void);
void LoadSpecialParam(void);

/**
 * Init 接口
 * 启动参数服务，在main启动的最后调用，阻赛当前线程
 *
 */
int StartParamService(void);

/**
 * Init 接口
 * 停止参数服务
 *
 */
void StopParamService(void);

/**
 * Init 接口
 * 加载默认的参数值
 *
 */
int LoadDefaultParams(const char *fileName, uint32_t mode);

/**
 * Init 接口
 * 加载持久化参数。
 *
 */
int LoadPersistParams(void);

/**
 * Init 接口
 * 加载加密目录下持久化参数。
 *
 */
int LoadPrivatePersistParams(void);

/**
 * Init 接口
 * 设置参数，主要用于其他进程使用，通过管道修改参数
 *
 */
int SystemWriteParam(const char *name, const char *value);

#ifndef OHOS_LITE
/**
 * Init 内部接口
 * 更新只读参数，主要用于Init其他进程使用，通过管道更新只读参数。
 * 
 */
int SystemUpdateConstParam(const char *name, const char *value);
#endif

#ifdef PARAM_SUPPORT_TRIGGER
/**
 * 对外接口
 * 触发一个trigger操作。
 *
 */
void PostTrigger(EventType type, const char *content, uint32_t contentLen);

/**
 * 对外接口
 * 解析trigger文件。
 *
 */
int ParseTriggerConfig(const cJSON *fileRoot, int (*checkJobValid)(const char *jobName), void *context);

/**
 * 对外接口
 * 按名字执行对应的trigger。
 *
 */
void DoTriggerExec(const char *triggerName);
void DoJobExecNow(const char *triggerName);

/**
 * 对外接口
 * 按名字添加一个job，用于group支持。
 *
 */
int AddCompleteJob(const char *name, const char *condition, const char *cmdContent);

void RegisterBootStateChange(void (*bootStateChange)(int start, const char *));

/**
 * 对外接口
 * dump 参数和trigger信息
 *
 */
void SystemDumpTriggers(int verbose, int (*dump)(const char *fmt, ...));
#endif

/**
 * 对外接口
 * 设置参数，主要用于其他进程使用，通过管道修改参数。
 *
 */
int SystemSetParameter(const char *name, const char *value);

/**
 * 对外接口
 * 保存共享内存中的所有持久化参数
 * 
*/
int SystemSaveParameters(void);

/**
 * Init 接口
 * 查询参数。
 *
 */
int SystemReadParam(const char *name, char *value, uint32_t *len);

/**
 * 对外接口
 * 查询参数，主要用于其他进程使用，需要给定足够的内存保存参数。
 * 如果 value == null，获取value的长度
 * 否则value的大小认为是len
 *
 */
#define SystemGetParameter SystemReadParam


/**
 * 外部接口
 * 等待某个参数值被修改，阻塞直到参数值被修改或超时
 *
 */
int SystemWaitParameter(const char *name, const char *value, int32_t timeout);

typedef void (*ParameterChangePtr)(const char *key, const char *value, void *context);
int SystemWatchParameter(const char *keyprefix, ParameterChangePtr change, void *context);

int SystemCheckParamExist(const char *name);

void SystemDumpParameters(int verbose, int index, int (*dump)(const char *fmt, ...));

int WatchParamCheck(const char *keyprefix);

void ResetParamSecurityLabel(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif