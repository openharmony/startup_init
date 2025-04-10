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

#include "le_loop.h"

#include <errno.h>
#include <sys/socket.h>
#include "securec.h"

#include "le_socket.h"
#include "le_task.h"

static LE_STATUS HandleSendMsg_(const LoopHandle loopHandle,
    const TaskHandle taskHandle, const LE_SendMessageComplete complete)
{
    EventLoop *loop = (EventLoop *)loopHandle;
    StreamTask *stream = (StreamTask *)taskHandle;
    LE_Buffer *buffer = GetFirstBuffer(stream);
    while (buffer) {
        int ret = write(GetSocketFd(taskHandle), buffer->data, buffer->dataSize);
        if (ret < 0 || (size_t)ret < buffer->dataSize) {
            LE_LOGE("HandleSendMsg_ fd:%d send data size %d %d, err:%d", GetSocketFd(taskHandle),
                    buffer->dataSize, ret, errno);
        }
        LE_LOGV("HandleSendMsg_ fd:%d send data size %d %d", GetSocketFd(taskHandle), buffer->dataSize, ret);
        buffer->result = (ret == (int)buffer->dataSize) ? 0 : errno;
        if (complete != NULL) {
            complete(taskHandle, buffer);
        }
        FreeBuffer(loopHandle, stream, buffer);
        buffer = GetFirstBuffer(stream);
    }
    if (IsBufferEmpty(stream)) {
        LE_LOGV("HandleSendMsg_ fd:%d empty wait read", GetSocketFd(taskHandle));
        loop->modEvent(loop, (const BaseTask *)taskHandle, EVENT_READ);
        return LE_SUCCESS;
    }
    return LE_SUCCESS;
}

static LE_STATUS HandleRecvMsg_(const LoopHandle loopHandle,
    const TaskHandle taskHandle, const LE_RecvMessage recvMessage, const LE_HandleRecvMsg handleRecvMsg)
{
    LE_STATUS status = LE_SUCCESS;
    LE_Buffer *buffer = CreateBuffer(LOOP_DEFAULT_BUFFER);
    LE_CHECK(buffer != NULL, return LE_NO_MEMORY, "Failed to create buffer");
    int readLen = 0;
    while (1) {
        if (handleRecvMsg != NULL) {
            readLen = handleRecvMsg(taskHandle, buffer->data, LOOP_DEFAULT_BUFFER, 0);
        } else {
            readLen = recv(GetSocketFd(taskHandle), buffer->data, LOOP_DEFAULT_BUFFER, 0);
        }
        LE_LOGV("HandleRecvMsg fd:%d read msg len %d", GetSocketFd(taskHandle), readLen);
        if (readLen < 0) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            status = LE_DIS_CONNECTED;
            break;
        } else if (readLen == 0) {
            // 若另一端已关闭连接则返回0，这种关闭是对方主动且正常的关闭
            status = LE_DIS_CONNECTED;
            break;
        } else {
            break;
        }
    }
    if (status != LE_SUCCESS) {
        FreeBuffer(loopHandle, NULL, buffer);
        return status;
    }
    if (recvMessage) {
        recvMessage(taskHandle, buffer->data, readLen);
    }
    FreeBuffer(loopHandle, NULL, buffer);
    return status;
}

static LE_STATUS HandleStreamEvent_(const LoopHandle loopHandle, const TaskHandle handle, uint32_t oper)
{
    StreamConnectTask *stream = (StreamConnectTask *)handle;
    LE_LOGV("HandleStreamEvent_ fd:%d oper 0x%x", GetSocketFd(handle), oper);

    LE_STATUS status = LE_SUCCESS;
    if (LE_TEST_FLAGS(oper, EVENT_WRITE)) {
        status = HandleSendMsg_(loopHandle, handle, stream->sendMessageComplete);
    }
    if (LE_TEST_FLAGS(oper, EVENT_READ)) {
        status = HandleRecvMsg_(loopHandle, handle, stream->recvMessage, stream->handleRecvMsg);
    }
    if (LE_TEST_FLAGS(oper, EVENT_ERROR)) {
        if (stream->disConnectComplete) {
            stream->disConnectComplete(handle);
        }
        LE_CloseStreamTask(loopHandle, handle);
    }
    return status;
}

static LE_STATUS HandleClientEvent_(const LoopHandle loopHandle, const TaskHandle handle, uint32_t oper)
{
    StreamClientTask *client = (StreamClientTask *)handle;
    LE_LOGI("HandleClientEvent_ fd:%d oper 0x%x", GetSocketFd(handle), oper);

    LE_STATUS status = LE_SUCCESS;
    if (LE_TEST_FLAGS(oper, EVENT_WRITE)) {
        LE_ONLY_CHECK(!(client->connected == 0 && client->connectComplete), client->connectComplete(handle));
        client->connected = 1;
        status = HandleSendMsg_(loopHandle, handle, client->sendMessageComplete);
    }
    if (LE_TEST_FLAGS(oper, EVENT_READ)) {
        status = HandleRecvMsg_(loopHandle, handle, client->recvMessage, client->handleRecvMsg);
    }
    if (status == LE_DIS_CONNECTED) {
        if (client->disConnectComplete) {
            client->disConnectComplete(handle);
        }
        client->connected = 0;
        LE_CloseStreamTask(loopHandle, handle);
    }
    return status;
}

static void HandleStreamTaskClose_(const LoopHandle loopHandle, const TaskHandle taskHandle)
{
    BaseTask *task = (BaseTask *)taskHandle;
    DelTask((EventLoop *)loopHandle, task);
    CloseTask(loopHandle, task);
    if (task->taskId.fd > 0) {
#ifndef __LITEOS__
        fdsan_close_with_tag(task->taskId.fd, BASE_DOMAIN);
#else
        close(task->taskId.fd);
#endif
    }
}

static void DumpStreamServerTaskInfo_(const TaskHandle task)
{
    INIT_CHECK(task != NULL, return);
    BaseTask *baseTask = (BaseTask *)task;
    StreamServerTask *serverTask = (StreamServerTask *)baseTask;
    printf("\tfd: %d \n", serverTask->base.taskId.fd);
    printf("\t  TaskType: %s \n", "ServerTask");
    if (strlen(serverTask->server) > 0) {
        printf("\t  Server socket:%s \n", serverTask->server);
    } else {
        printf("\t  Server socket:%s \n", "NULL");
    }
}

static void DumpStreamConnectTaskInfo_(const TaskHandle task)
{
    INIT_CHECK(task != NULL, return);
    BaseTask *baseTask = (BaseTask *)task;
    StreamConnectTask *connectTask = (StreamConnectTask *)baseTask;
    TaskHandle taskHandle = (TaskHandle)connectTask;
    printf("\tfd: %d \n", connectTask->stream.base.taskId.fd);
    printf("\t  TaskType: %s \n", "ConnectTask");
    printf("\t  ServiceInfo: \n");
    struct ucred cred = {-1, -1, -1};
    socklen_t credSize  = sizeof(struct ucred);
    if (getsockopt(LE_GetSocketFd(taskHandle), SOL_SOCKET, SO_PEERCRED, &cred, &credSize) == 0) {
        printf("\t    Service Pid: %d \n", cred.pid);
        printf("\t    Service Uid: %u \n", cred.uid);
        printf("\t    Service Gid: %u \n", cred.gid);
    } else {
        printf("\t    Service Pid: %s \n", "NULL");
        printf("\t    Service Uid: %s \n", "NULL");
        printf("\t    Service Gid: %s \n", "NULL");
    }
}

static LE_STATUS HandleServerEvent_(const LoopHandle loopHandle, const TaskHandle serverTask, uint32_t oper)
{
    LE_LOGV("HandleServerEvent_ fd %d oper 0x%x", GetSocketFd(serverTask), oper);
    if (!LE_TEST_FLAGS(oper, EVENT_READ)) {
        return LE_FAILURE;
    }
    StreamServerTask *server = (StreamServerTask *)serverTask;
    LE_ONLY_CHECK(server->incommingConnect != NULL, return LE_SUCCESS);

    int ret = server->incommingConnect(loopHandle, serverTask);
    if (ret != LE_SUCCESS) {
        LE_LOGE("HandleServerEvent_ fd %d do not accept socket", GetSocketFd(serverTask));
    }
    EventLoop *loop = (EventLoop *)loopHandle;
    loop->modEvent(loop, (const BaseTask *)serverTask, EVENT_READ);
    return LE_SUCCESS;
}

LE_STATUS LE_CreateStreamServer(const LoopHandle loopHandle,
    TaskHandle *taskHandle, const LE_StreamServerInfo *info)
{
    LE_CHECK(loopHandle != NULL && taskHandle != NULL && info != NULL, return LE_INVALID_PARAM, "Invalid parameters");
    LE_CHECK(info->server != NULL, return LE_INVALID_PARAM, "Invalid parameters server");
    LE_CHECK(info->incommingConnect != NULL, return LE_INVALID_PARAM,
        "Invalid parameters incommingConnect %s", info->server);

    int fd = info->socketId;
    int ret = 0;
    if (info->socketId <= 0) {
        fd = CreateSocket(info->baseInfo.flags, info->server);
        LE_CHECK(fd > 0, return LE_FAILURE, "Failed to create socket %s", info->server);
    } else {
        ret = listenSocket(fd, info->baseInfo.flags, info->server);
        LE_CHECK(ret == 0, return LE_FAILURE, "Failed to listen socket %s", info->server);
    }

    EventLoop *loop = (EventLoop *)loopHandle;
    StreamServerTask *task = (StreamServerTask *)CreateTask(loopHandle, fd, &info->baseInfo,
        sizeof(StreamServerTask) + strlen(info->server) + 1);
#ifndef __LITEOS__
    LE_CHECK(task != NULL, fdsan_close_with_tag(fd, BASE_DOMAIN);
        return LE_NO_MEMORY, "Failed to create task");
#else
    LE_CHECK(task != NULL, close(fd);
        return LE_NO_MEMORY, "Failed to create task");
#endif
    task->base.handleEvent = HandleServerEvent_;
    task->base.innerClose = HandleStreamTaskClose_;
    task->base.dumpTaskInfo = DumpStreamServerTaskInfo_;
    task->incommingConnect = info->incommingConnect;
    loop->addEvent(loop, (const BaseTask *)task, EVENT_READ);
    ret = memcpy_s(task->server, strlen(info->server) + 1, info->server, strlen(info->server) + 1);
    LE_CHECK(ret == 0, return LE_FAILURE, "Failed to copy server name %s", info->server);
    *taskHandle = (TaskHandle)task;
    return LE_SUCCESS;
}

LE_STATUS LE_CreateStreamClient(const LoopHandle loopHandle,
    TaskHandle *taskHandle, const LE_StreamInfo *info)
{
    LE_CHECK(loopHandle != NULL && taskHandle != NULL && info != NULL, return LE_INVALID_PARAM, "Invalid parameters");
    LE_CHECK(info->recvMessage != NULL, return LE_FAILURE, "Invalid parameters recvMessage %s", info->server);

    int fd = CreateSocket(info->baseInfo.flags, info->server);
    LE_CHECK(fd > 0, return LE_FAILURE, "Failed to create socket %s", info->server);

    StreamClientTask *task = (StreamClientTask *)CreateTask(loopHandle, fd, &info->baseInfo, sizeof(StreamClientTask));
#ifndef __LITEOS__
    LE_CHECK(task != NULL, fdsan_close_with_tag(fd, BASE_DOMAIN);
        return LE_NO_MEMORY, "Failed to create task");
#else
    LE_CHECK(task != NULL, close(fd);
        return LE_NO_MEMORY, "Failed to create task");
#endif
    task->stream.base.handleEvent = HandleClientEvent_;
    task->stream.base.innerClose = HandleStreamTaskClose_;
    OH_ListInit(&task->stream.buffHead);
    LoopMutexInit(&task->stream.mutex);

    task->connectComplete = info->connectComplete;
    task->sendMessageComplete = info->sendMessageComplete;
    task->recvMessage = info->recvMessage;
    task->disConnectComplete = info->disConnectComplete;
    task->handleRecvMsg = info->handleRecvMsg;
    EventLoop *loop = (EventLoop *)loopHandle;
    loop->addEvent(loop, (const BaseTask *)task, EVENT_READ);
    *taskHandle = (TaskHandle)task;
    return LE_SUCCESS;
}

LE_STATUS LE_AcceptStreamClient(const LoopHandle loopHandle, const TaskHandle server,
    TaskHandle *taskHandle, const LE_StreamInfo *info)
{
    LE_CHECK(loopHandle != NULL && info != NULL, return LE_INVALID_PARAM, "Invalid parameters");
    LE_CHECK(server != NULL && taskHandle != NULL, return LE_INVALID_PARAM, "Invalid parameters");
    LE_CHECK(info->recvMessage != NULL, return LE_INVALID_PARAM, "Invalid parameters recvMessage");
    int fd = -1;
    if ((info->baseInfo.flags & TASK_TEST) != TASK_TEST) {
        fd = AcceptSocket(GetSocketFd(server), info->baseInfo.flags);
        LE_CHECK(fd > 0, return LE_FAILURE, "Failed to accept socket %d", GetSocketFd(server));
    }
    StreamConnectTask *task = (StreamConnectTask *)CreateTask(
        loopHandle, fd, &info->baseInfo, sizeof(StreamConnectTask));
#ifndef __LITEOS__
    LE_CHECK(task != NULL, fdsan_close_with_tag(fd, BASE_DOMAIN);
        return LE_NO_MEMORY, "Failed to create task");
#else
    LE_CHECK(task != NULL, close(fd);
        return LE_NO_MEMORY, "Failed to create task");
#endif
    task->stream.base.handleEvent = HandleStreamEvent_;
    task->stream.base.innerClose = HandleStreamTaskClose_;
    task->stream.base.dumpTaskInfo = DumpStreamConnectTaskInfo_;
    task->disConnectComplete = info->disConnectComplete;
    task->sendMessageComplete = info->sendMessageComplete;
    task->recvMessage = info->recvMessage;
    task->serverTask = (StreamServerTask *)server;
    task->handleRecvMsg = info->handleRecvMsg;
    OH_ListInit(&task->stream.buffHead);
    LoopMutexInit(&task->stream.mutex);
    if ((info->baseInfo.flags & TASK_TEST) != TASK_TEST) {
        EventLoop *loop = (EventLoop *)loopHandle;
        loop->addEvent(loop, (const BaseTask *)task, EVENT_READ);
    }
    *taskHandle = (TaskHandle)task;
    return 0;
}

void LE_CloseStreamTask(const LoopHandle loopHandle, const TaskHandle taskHandle)
{
    LE_CHECK(loopHandle != NULL && taskHandle != NULL, return, "Invalid parameters");
    LE_CloseTask(loopHandle, taskHandle);
}

int LE_GetSocketFd(const TaskHandle taskHandle)
{
    LE_CHECK(taskHandle != NULL, return -1, "Invalid parameters");
    return GetSocketFd(taskHandle);
}
