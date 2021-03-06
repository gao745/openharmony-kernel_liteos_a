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
#include "it_test_process.h"

static int Child1(void)
{
    int ret;
    siginfo_t info = { 0 };

    pid_t pid = fork();
    ICUNIT_GOTO_WITHIN_EQUAL(pid, 0, 100000, pid, EXIT); // 100000, assert pid equal to this.
    if (pid == 0) {
        ret = kill(getpid(), SIGKILL);
        ICUNIT_ASSERT_EQUAL(ret, 0, ret);
    }

    ret = waitid(P_PID, pid, &info, WEXITED);

    ICUNIT_ASSERT_EQUAL(ret, 0, ret);
    ICUNIT_ASSERT_EQUAL(info.si_status, SIGKILL, info.si_status);
    ICUNIT_ASSERT_EQUAL(info.si_code, 2, info.si_code); // 2, assert that function Result is equal to this.
    ICUNIT_ASSERT_EQUAL(info.si_pid, pid, info.si_pid);
    exit(8); // 8, exit args
EXIT:
    return 0;
}

static int TestCase(void)
{
    pid_t pid;
    int status;
    int ret;
    pid = fork();

    ICUNIT_GOTO_WITHIN_EQUAL(pid, 0, 100000, pid, EXIT); // 100000, assert pid equal to this.
    if (pid == 0) {
        Child1();
        printf("[Failed] - [Errline : %d RetCode : 0x%x\n", g_iCunitErrLineNo, g_iCunitErrCode);
        exit(g_iCunitErrLineNo);
    } else {
        siginfo_t info = { 0 };

        ret = waitid(P_ALL, getpgrp(), &info, WEXITED);

        ICUNIT_ASSERT_EQUAL(ret, 0, ret);
        ICUNIT_ASSERT_EQUAL(info.si_status, 8, info.si_status); // 8, assert that function Result is equal to this.
        ICUNIT_ASSERT_EQUAL(info.si_code, 1, info.si_code);
        ICUNIT_ASSERT_EQUAL(info.si_pid, pid, info.si_pid);

        pid = fork();
        ICUNIT_GOTO_WITHIN_EQUAL(pid, 0, 100000, pid, EXIT); // 100000, assert pid equal to this.

        if (pid == 0) {
            exit(11); // 11, exit args
        }

        sleep(1);

        ret = waitid(P_PGID, getpgrp(), &info, WNOHANG);

        ICUNIT_ASSERT_EQUAL(ret, 0, ret);
        ICUNIT_ASSERT_EQUAL(info.si_status, 11, info.si_status); // 11, assert that function Result is equal to this.
        ICUNIT_ASSERT_EQUAL(info.si_code, 1, info.si_code);
        ICUNIT_ASSERT_EQUAL(info.si_pid, pid, info.si_pid);
    }
    return 0;
EXIT:
    return 1;
}

void ItTestProcess055(void)
{
    TEST_ADD_CASE("IT_POSIX_PROCESS_055", TestCase, TEST_POSIX, TEST_MEM, TEST_LEVEL0, TEST_FUNCTION);
}