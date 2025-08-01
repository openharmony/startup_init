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
#include <gtest/gtest.h>

#include "if_system_ability_manager.h"
#include "init_param.h"
#include "iservice_registry.h"
#include "iwatcher.h"
#include "iwatcher_manager.h"
#include "message_parcel.h"
#include "parameter.h"
#include "param_manager.h"
#include "param_stub.h"
#include "param_utils.h"
#include "system_ability_definition.h"
#include "watcher.h"
#include "watcher_manager.h"
#include "watcher_manager_kits.h"
#include "watcher_manager_proxy.h"
#include "service_watcher.h"

using namespace testing::ext;
using namespace std;
using namespace OHOS;
using namespace OHOS::init_param;

int g_callbackCount = 0;
static void TestParameterChange(const char *key, const char *value, void *context)
{
    printf("TestParameterChange key:%s %s \n", key, value);
    g_callbackCount++;
}

static void TestWatcherCallBack(const char *key, const ServiceInfo *status)
{
    printf("TestWatcherCallBack key:%s %d", key, status->status);
}

using WatcherManagerPtr = WatcherManager *;
class WatcherAgentUnitTest : public ::testing::Test {
public:
    WatcherAgentUnitTest() {}
    virtual ~WatcherAgentUnitTest() {}

    void SetUp()
    {
        if (GetParamSecurityLabel() != nullptr) {
            GetParamSecurityLabel()->cred.uid = 1000;  // 1000 test uid
            GetParamSecurityLabel()->cred.gid = 1000;  // 1000 test gid
        }
        SetTestPermissionResult(0);
    }
    void TearDown() {}
    void TestBody() {}

    int TestAddWatcher0(size_t index)
    {
        int ret = 0;
        // has beed deleted
        ret = RemoveParameterWatcher("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_NE(ret, 0);

        // delete all
        ret = SystemWatchParameter("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        index++;
        ret = SystemWatchParameter("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        ret = RemoveParameterWatcher("test.permission.watcher.test1", nullptr, nullptr);
        EXPECT_EQ(ret, 0);
        // 非法
        ret = SystemWatchParameter("test.permission.watcher.tes^^^^t1*", TestParameterChange, nullptr);
        EXPECT_EQ(ret, PARAM_CODE_INVALID_NAME);
        ret = SystemWatchParameter("test.permission.read.test1*", TestParameterChange, nullptr);
        EXPECT_EQ(ret, DAC_RESULT_FORBIDED);
        return ret;
    }
    int TestAddWatcher()
    {
        size_t index = 1;
        int ret = SystemWatchParameter("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        ret = SystemWatchParameter("test.permission.watcher.test1*",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        // repeat add, return fail
        ret = SystemWatchParameter("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_NE(ret, 0);
        index++;
        ret = SystemWatchParameter("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        index++;
        ret = SystemWatchParameter("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);

        // delete
        ret = RemoveParameterWatcher("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        ret = RemoveParameterWatcher("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        index--;
        ret = RemoveParameterWatcher("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        index--;
        ret = RemoveParameterWatcher("test.permission.watcher.test1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        return TestAddWatcher0(index);
    }

    int TestDelWatcher()
    {
        size_t index = 1;
        int ret = SystemWatchParameter("test.permission.watcher.test3.1",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        ret = SystemWatchParameter("test.permission.watcher.test3.1*",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        ret = SystemWatchParameter("test.permission.watcher.test3.2",
            TestParameterChange, reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, 0);
        ret = SystemWatchParameter("test.permission.watcher.test3.2", TestParameterChange,
            reinterpret_cast<void *>(index));
        EXPECT_EQ(ret, PARAM_WATCHER_CALLBACK_EXIST);
        ret = SystemWatchParameter("test.permission.watcher.test3.1", nullptr, nullptr);
        EXPECT_EQ(ret, 0);
        ret = SystemWatchParameter("test.permission.watcher.test3.1*", nullptr, nullptr);
        EXPECT_EQ(ret, 0);
        ret = SystemWatchParameter("test.permission.watcher.test3.2", nullptr, nullptr);
        EXPECT_EQ(ret, 0);

        // 非法
        ret = SystemWatchParameter("test.permission.watcher.tes^^^^t1*", nullptr, nullptr);
        EXPECT_EQ(ret, PARAM_CODE_INVALID_NAME);
        ret = SystemWatchParameter("test.permission.read.test1*", nullptr, nullptr);
        EXPECT_EQ(ret, DAC_RESULT_FORBIDED);
        return 0;
    }

    int TestRecvMessage(const std::string &name)
    {
        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        data.WriteInterfaceToken(IWatcher::GetDescriptor());
        data.WriteString16(Str8ToStr16(name));
        data.WriteString16(Str8ToStr16(name));
        data.WriteString16(Str8ToStr16("watcherId"));
        g_callbackCount = 0;
        int ret = SystemWatchParameter(name.c_str(), TestParameterChange, nullptr);
        EXPECT_EQ(ret, 0);
        WatcherManagerKits &instance = OHOS::init_param::WatcherManagerKits::GetInstance();
        if (instance.remoteWatcher_ != nullptr) {
            instance.remoteWatcher_->OnRemoteRequest(static_cast<uint32_t>
                (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE), data, reply, option);
            instance.remoteWatcher_->OnRemoteRequest(static_cast<uint32_t>
                (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE) + 1, data, reply, option);
            instance.remoteWatcher_->OnParameterChange(name.c_str(), "testname", "testvalue");
            EXPECT_EQ(g_callbackCount, 2); // 2 is callback Count
            instance.remoteWatcher_->OnParameterChange(name.c_str(), "testname.2", "testvalue");
            EXPECT_EQ(g_callbackCount, 3); // 3 is callback Count
            instance.remoteWatcher_->OnParameterChange(name.c_str(), "testname.2", "testvalue");
            EXPECT_EQ(g_callbackCount, 3); // 3 is callback Count

            // prefix not exit
            instance.remoteWatcher_->OnParameterChange("44444444444444444444", "testname.2", "testvalue");
        }
        EXPECT_EQ(g_callbackCount, 3); // 3 is callback Count
        return 0;
    }

    int TestResetService()
    {
        sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
        WATCHER_CHECK(samgr != nullptr, return -1, "Get samgr failed");
        sptr<IRemoteObject> object = samgr->GetSystemAbility(PARAM_WATCHER_DISTRIBUTED_SERVICE_ID);
        WATCHER_CHECK(object != nullptr, return -1, "Get watcher manager object from samgr failed");
        OHOS::init_param::WatcherManagerKits &instance = OHOS::init_param::WatcherManagerKits::GetInstance();
        if (instance.GetDeathRecipient() != nullptr) {
            instance.GetDeathRecipient()->OnRemoteDied(object);
        }
        return 0;
    }

    WatcherManagerPtr GetWatcherManager()
    {
        static WatcherManagerPtr watcherManager_ = nullptr;
        if (watcherManager_ == nullptr) {
            watcherManager_ = new WatcherManager(0, true);
            if (watcherManager_ == nullptr) {
                return nullptr;
            }
            watcherManager_->OnStart();
        }
        return watcherManager_;
    }

    void TestWatcherProxy()
    {
        WatcherManagerPtr watcherManager = GetWatcherManager();
        ASSERT_NE(watcherManager, nullptr);

        WatcherManagerKits &instance = OHOS::init_param::WatcherManagerKits::GetInstance();
        sptr<Watcher> remoteWatcher = new OHOS::init_param::WatcherManagerKits::RemoteWatcher(&instance);
        ASSERT_NE(remoteWatcher, nullptr);

        MessageParcel data;
        MessageParcel reply;
        MessageOption option;
        data.WriteInterfaceToken(IWatcher::GetDescriptor());
        data.WriteString16(Str8ToStr16("name"));
        data.WriteString16(Str8ToStr16("name"));
        data.WriteString16(Str8ToStr16("watcherId"));
        remoteWatcher->OnRemoteRequest(static_cast<uint32_t>
            (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE), data, reply, option);

        // invalid parameter
        data.WriteInterfaceToken(IWatcher::GetDescriptor());
        data.WriteString16(Str8ToStr16("name"));
        data.WriteString16(Str8ToStr16("watcherId"));
        remoteWatcher->OnRemoteRequest(static_cast<uint32_t>
            (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE), data, reply, option);

        data.WriteInterfaceToken(IWatcher::GetDescriptor());
        data.WriteString16(Str8ToStr16("name"));
        remoteWatcher->OnRemoteRequest(static_cast<uint32_t>
            (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE), data, reply, option);

        data.WriteInterfaceToken(IWatcher::GetDescriptor());
        remoteWatcher->OnRemoteRequest(static_cast<uint32_t>
            (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE), data, reply, option);

        data.WriteInterfaceToken(IWatcher::GetDescriptor());
        data.WriteString16(Str8ToStr16("name"));
        data.WriteString16(Str8ToStr16("name"));
        data.WriteString16(Str8ToStr16("watcherId"));
        remoteWatcher->OnRemoteRequest(static_cast<uint32_t>
            (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE) + 1, data, reply, option);
        remoteWatcher->OnRemoteRequest(static_cast<uint32_t>
            (OHOS::init_param::IWatcherIpcCode::COMMAND_ON_PARAMETER_CHANGE) + 1, data, reply, option);

        uint32_t watcherId = 0;
        int32_t ret = watcherManager->AddRemoteWatcher(1000, watcherId, remoteWatcher);
        // add watcher
        ret = watcherManager->AddWatcher("test.watcher.proxy", watcherId);
        ASSERT_EQ(ret, 0);
        ret = watcherManager->DelWatcher("test.watcher.proxy", watcherId);
        ASSERT_EQ(ret, 0);
        ret = watcherManager->RefreshWatcher("test.watcher.proxy", watcherId);
        ASSERT_EQ(ret, 0);
        watcherManager->DelRemoteWatcher(watcherId);
    }
};

HWTEST_F(WatcherAgentUnitTest, Init_TestAddWatcher_001, TestSize.Level0)
{
    WatcherAgentUnitTest test;
    test.TestAddWatcher();
}

HWTEST_F(WatcherAgentUnitTest, Init_TestRecvMessage_001, TestSize.Level0)
{
    WatcherAgentUnitTest test;
    test.TestRecvMessage("test.permission.watcher.agent.test1");
}

HWTEST_F(WatcherAgentUnitTest, Init_TestDelWatcher_001, TestSize.Level0)
{
    WatcherAgentUnitTest test;
    test.TestDelWatcher();
}

HWTEST_F(WatcherAgentUnitTest, Init_TestResetService_001, TestSize.Level0)
{
    WatcherAgentUnitTest test;
    test.TestResetService();
    test.TestResetService();
}

HWTEST_F(WatcherAgentUnitTest, Init_TestWatcherService_001, TestSize.Level0)
{
    const char *errstr = "111111111111111111111111111111111111111111111111111111111111111111111111111111111111";
    ServiceWatchForStatus("param_watcher", TestWatcherCallBack);
    ServiceWaitForStatus("param_watcher", SERVICE_STARTED, 1);
    EXPECT_NE(ServiceWatchForStatus(errstr, TestWatcherCallBack), 0);
    EXPECT_NE(ServiceWatchForStatus(nullptr, TestWatcherCallBack), 0);
    WatchParameter("testParam", nullptr, nullptr);
    WatchParameter(nullptr, nullptr, nullptr);
}

HWTEST_F(WatcherAgentUnitTest, Init_TestInvalidWatcher_001, TestSize.Level0)
{
    int ret = SystemWatchParameter(nullptr, TestParameterChange, nullptr);
    ASSERT_NE(ret, 0);
    ret = RemoveParameterWatcher(nullptr, nullptr, nullptr);
    ASSERT_NE(ret, 0);
}

HWTEST_F(WatcherAgentUnitTest, Init_TestWatcherProxy_001, TestSize.Level0)
{
    WatcherAgentUnitTest test;
    test.TestWatcherProxy();
}
