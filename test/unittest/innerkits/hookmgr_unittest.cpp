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

#include <cinttypes>
#include <gtest/gtest.h>
#include "hookmgr.h"
#include "bootstage.h"
using namespace testing::ext;
using namespace std;

namespace init_ut {
class HookMgrUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp() {};
    void TearDown() {};
};

struct HookExecCtx {
    int result;
    int retErr;
};

static int OhosHookTestCommon(void *executionContext, int result)
{
    struct HookExecCtx *ctx;

    if (executionContext == nullptr) {
        return 0;
    }

    ctx = (struct HookExecCtx *)executionContext;
    ctx->result = result;
    if (ctx->retErr) {
        return -1;
    }
    return 0;
}

static int OhosTestHookRetOK(const HOOK_INFO *hookInfo, void *executionContext)
{
    return OhosHookTestCommon(executionContext, 1);
}

static int OhosTestHookRetOKEx(const HOOK_INFO *hookInfo, void *executionContext)
{
    return OhosHookTestCommon(executionContext, 2);
}

static int OhosTestHookRetOKEx2(const HOOK_INFO *hookInfo, void *executionContext)
{
    return OhosHookTestCommon(executionContext, 3);
}

static void OhosHookPrint(const HOOK_INFO *hookInfo, void *traversalCookie)
{
    printf("\tstage[%02d] prio[%02d].\n", hookInfo->stage, hookInfo->prio);
}

static void dumpAllHooks(HOOK_MGR *hookMgr)
{
    printf("----------All Hooks---------------\n");
    HookMgrTraversal(hookMgr, nullptr, OhosHookPrint);
    printf("----------------------------------\n\n");
}

#define STAGE_TEST_ONE 0

HWTEST_F(HookMgrUnitTest, HookMgrAdd_one_stage_unitest, TestSize.Level1)
{
    int ret;
    int cnt;

    cnt = HookMgrGetStagesCnt(nullptr);
    EXPECT_EQ(cnt, 0);

    // Add the first hook
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 1);
    cnt = HookMgrGetStagesCnt(nullptr);
    EXPECT_EQ(cnt, 1);
    dumpAllHooks(nullptr);

    // Add the same hook with the same priority again
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 1);
    dumpAllHooks(nullptr);

    // Add the same hook with different priority
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 10, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 2);
    dumpAllHooks(nullptr);

    // Add the another hook
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 10, OhosTestHookRetOKEx);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 3);
    dumpAllHooks(nullptr);

    // Add the same hook with the same priority again
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 3);
    dumpAllHooks(nullptr);

    // Add the same hook with the same priority again
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 10, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 3);
    dumpAllHooks(nullptr);

    // Add the another hook
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 10, OhosTestHookRetOKEx);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 3);
    dumpAllHooks(nullptr);

    // Insert to the end of already exist prio
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOKEx);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 4);
    dumpAllHooks(nullptr);

    // Insert to the end of already exist prio
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOKEx2);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 5);
    dumpAllHooks(nullptr);

    // Insert a new prio hook
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 5, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 6);
    dumpAllHooks(nullptr);

    // Insert a new prio hook to the beginning
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, -5, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(nullptr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 7);
    dumpAllHooks(nullptr);

    // All hooks are in the same stage
    cnt = HookMgrGetStagesCnt(nullptr);
    EXPECT_EQ(cnt, 1);

    // Delete all hooks in stage 0
    HookMgrDel(nullptr, STAGE_TEST_ONE, nullptr);
    cnt = HookMgrGetHooksCnt(nullptr, 0);
    EXPECT_EQ(cnt, 0);
    cnt = HookMgrGetStagesCnt(nullptr);
    EXPECT_EQ(cnt, 0);

    dumpAllHooks(nullptr);
    HookMgrDestroy(nullptr);
}

HWTEST_F(HookMgrUnitTest, HookMgrDel_unitest, TestSize.Level1)
{
    int ret;
    int cnt;

    HOOK_MGR *hookMgr = HookMgrCreate("test");
    ASSERT_NE(hookMgr, nullptr);

    // Add one, delete one
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 1);

    HookMgrDel(hookMgr, STAGE_TEST_ONE, OhosTestHookRetOK);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 0);

    // Add three same hook with different prio, delete together
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 1);
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 5, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 2);
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 10, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 3);
    dumpAllHooks(hookMgr);

    HookMgrDel(hookMgr, STAGE_TEST_ONE, OhosTestHookRetOK);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 0);

    // Add three different hook with same prio, delete one by one
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 1);
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 0, OhosTestHookRetOKEx);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 2);
    ret = HookMgrAdd(hookMgr, STAGE_TEST_ONE, 0, OhosTestHookRetOKEx2);
    EXPECT_EQ(ret, 0);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 3);
    dumpAllHooks(hookMgr);

    HookMgrDel(hookMgr, STAGE_TEST_ONE, OhosTestHookRetOK);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 2);
    dumpAllHooks(hookMgr);
    HookMgrDel(hookMgr, STAGE_TEST_ONE, OhosTestHookRetOKEx2);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 1);
    dumpAllHooks(hookMgr);
    HookMgrDel(hookMgr, STAGE_TEST_ONE, OhosTestHookRetOKEx);
    cnt = HookMgrGetHooksCnt(hookMgr, STAGE_TEST_ONE);
    EXPECT_EQ(cnt, 0);

    HookMgrDestroy(hookMgr);
}

HWTEST_F(HookMgrUnitTest, HookMgrExecute_unitest, TestSize.Level1)
{
    int ret;
    struct HookExecCtx ctx;
    HOOK_EXEC_OPTIONS options;

    ctx.result = 0;
    ctx.retErr = 0;

    options.flags = 0;
    options.preHook = nullptr;
    options.postHook = nullptr;

    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOK);
    EXPECT_EQ(ret, 0);
    ret = HookMgrExecute(nullptr, STAGE_TEST_ONE, (void *)&ctx, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.result, 1);

    // Check ignore error
    ctx.retErr = 1;
    ret = HookMgrExecute(nullptr, STAGE_TEST_ONE, (void *)&ctx, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.result, 1);

    // Do not ignore return errors
    ctx.retErr = 1;
    options.flags = HOOK_EXEC_EXIT_WHEN_ERROR;
    ret = HookMgrExecute(nullptr, STAGE_TEST_ONE, (void *)&ctx, &options);
    ASSERT_NE(ret, 0);
    EXPECT_EQ(ctx.result, 1);
    options.flags = 0;

    // Add another hook with same priority
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, 0, OhosTestHookRetOKEx);
    EXPECT_EQ(ret, 0);
    ret = HookMgrExecute(nullptr, STAGE_TEST_ONE, (void *)&ctx, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.result, 2);

    // Add another hook with higher priority
    ret = HookMgrAdd(nullptr, STAGE_TEST_ONE, -1, OhosTestHookRetOKEx);
    EXPECT_EQ(ret, 0);
    ret = HookMgrExecute(nullptr, STAGE_TEST_ONE, (void *)&ctx, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.result, 2);

    HookMgrDel(nullptr, STAGE_TEST_ONE, OhosTestHookRetOKEx);
    ret = HookMgrExecute(nullptr, STAGE_TEST_ONE, (void *)&ctx, nullptr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(ctx.result, 1);
}

HWTEST_F(HookMgrUnitTest, HookMgrExecuteInit_unitest, TestSize.Level1)
{
    int ret = HookMgrExecute(GetBootStageHookMgr(), INIT_GLOBAL_INIT, nullptr, nullptr);
    EXPECT_NE(ret, -1);
    ret = HookMgrExecute(GetBootStageHookMgr(), INIT_PRE_CFG_LOAD, nullptr, nullptr);
    EXPECT_NE(ret, -1);
}
} // namespace init_ut
