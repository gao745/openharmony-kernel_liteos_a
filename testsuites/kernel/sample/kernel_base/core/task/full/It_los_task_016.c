/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 * conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 * of conditions and the following disclaimer in the documentation and/or other materials
 * provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific prior written
 * permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "It_los_task.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

static void TaskF04(UINTPTR arg1, UINTPTR arg2, UINTPTR arg3, UINTPTR arg4)
{
    ICUNIT_ASSERT_EQUAL_VOID(arg1, 1, arg1);
    ICUNIT_ASSERT_EQUAL_VOID(arg2, 0xffffffff, arg2);
    ICUNIT_ASSERT_EQUAL_VOID(arg3, -1, arg3);
    ICUNIT_ASSERT_EQUAL_VOID(arg4, 0xffffffff, arg4);
    g_testCount++;

    return;
}

static void TaskF03(UINTPTR arg1, UINTPTR arg2, UINTPTR arg3)
{
    ICUNIT_ASSERT_EQUAL_VOID(arg1, 0, arg1);
    ICUNIT_ASSERT_EQUAL_VOID(arg2, 0xffff, arg2);
    ICUNIT_ASSERT_EQUAL_VOID(arg3, -1, arg3);

    g_testCount++;

    return;
}

static void TaskF02(UINTPTR arg1, UINTPTR arg2)
{
    ICUNIT_ASSERT_EQUAL_VOID(arg1, 0, arg1);
    ICUNIT_ASSERT_EQUAL_VOID(arg2, 0xffff, arg2);

    g_testCount++;

    return;
}
static void TaskF01(UINTPTR arg1)
{
    ICUNIT_ASSERT_EQUAL_VOID(arg1, 0xffff, arg1);

    g_testCount++;

    return;
}

static UINT32 Testcase(void)
{
    UINT32 ret;
    TSK_INIT_PARAM_S task1 = { 0 };
    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskF01;
    task1.uwStackSize = TASK_STACK_SIZE_TEST;
    task1.pcName = "Tsk016A";
    task1.usTaskPrio = TASK_PRIO_TEST_TASK - 1;
    task1.uwResved = LOS_TASK_STATUS_DETACHED;
    // 0xffff, initializes the args. this parameter has no special meaning.
    task1.auwArgs[0] = 0xffff;
#if (LOSCFG_KERNEL_SMP == YES)
    task1.usCpuAffiMask = CPUID_TO_AFFI_MASK(ArchCurrCpuid());
#endif

    g_testCount = 0;
    ret = LOS_TaskCreate(&g_testTaskID01, &task1);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);

    ICUNIT_ASSERT_EQUAL(g_testCount, 1, g_testCount);

    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskF02;
    task1.auwArgs[0] = 0;
    // 0xffff, initializes the args. this parameter has no special meaning.
    task1.auwArgs[1] = 0xffff;
    ret = LOS_TaskCreate(&g_testTaskID01, &task1);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);

    ICUNIT_ASSERT_EQUAL(g_testCount, 2, g_testCount); // 2, assert that g_testCount is equal to this.

    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskF03;
    task1.auwArgs[0] = 0;
    // 0xffff, initializes the args. this parameter has no special meaning.
    task1.auwArgs[1] = 0xffff;
    // 2, auwArgs array subscript.
    task1.auwArgs[2] = -1;
    ret = LOS_TaskCreate(&g_testTaskID01, &task1);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);

    ICUNIT_ASSERT_EQUAL(g_testCount, 3, g_testCount); // 3, assert that g_testCount is equal to this.

    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskF04;
    task1.auwArgs[0] = 1;
    // 0xffffffff, initializes the args. this parameter has no special meaning.
    task1.auwArgs[1] = 0xffffffff;
    // 2, auwArgs array subscript.
    task1.auwArgs[2] = -1;
    // 3, auwArgs array subscript.
    task1.auwArgs[3] = 0xffffffff;
    ret = LOS_TaskCreate(&g_testTaskID01, &task1);
    ICUNIT_ASSERT_EQUAL(ret, LOS_OK, ret);

    ICUNIT_ASSERT_EQUAL(g_testCount, 4, g_testCount); // 4, assert that g_testCount is equal to this.

    return LOS_OK;
}

void ItLosTask016(void) // IT_Layer_ModuleORFeature_No
{
    TEST_ADD_CASE("ItLosTask016", Testcase, TEST_LOS, TEST_TASK, TEST_LEVEL2, TEST_FUNCTION);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */
