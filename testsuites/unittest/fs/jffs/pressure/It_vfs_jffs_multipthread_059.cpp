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

#include "It_vfs_jffs.h"

static INT32 g_testNum = 2;

static VOID *MutiJffs05901(void *arg)
{
    INT32 ret, i;
    CHAR pathname[JFFS_STANDARD_NAME_LENGTH] = { JFFS_PATH_NAME0 };
    dprintf(" start muti_jffs_059_01  1 \n");
    strcat_s(pathname, JFFS_STANDARD_NAME_LENGTH, "/test_59");

    // file size: 1024, write size: 16
    ret = JffsMultiWrite(pathname, 1024, 16, O_RDWR | O_CREAT, JFFS_WR_TYPE_TEST);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT);

    g_TestCnt++;

    while (g_TestCnt < g_testNum) {
        sleep(1);
        dprintf(" sleep 59_01 \n");
    }

    dprintf(" end muti_jffs_059_01  2 \n");

    return NULL;
EXIT:

    g_TestCnt++;
    return NULL;
}

static VOID *MutiJffs05902(void *arg)
{
    INT32 ret, i, len;
    INT32 fd = -1;
    CHAR pathname[JFFS_STANDARD_NAME_LENGTH] = { JFFS_PATH_NAME0 };
    CHAR readbuf[JFFS_STANDARD_NAME_LENGTH] = "liteos";
    CHAR filebuf[JFFS_STANDARD_NAME_LENGTH] = "0000000000111111111122222222223333333333";

    dprintf(" start muti_jffs_059_02  1 \n");
    strcat_s(pathname, JFFS_STANDARD_NAME_LENGTH, "/test_59");
    ICUNIT_GOTO_EQUAL(g_TestCnt, 0, g_TestCnt, EXIT);

    fd = open(pathname, O_WRONLY | O_CREAT, HIGHEST_AUTHORITY);
    ICUNIT_GOTO_NOT_EQUAL(fd, JFFS_IS_ERROR, fd, EXIT);

    ret = lseek(fd, 0, SEEK_SET);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT1);

    read(fd, readbuf, JFFS_STANDARD_NAME_LENGTH);

    len = write(fd, filebuf, strlen(filebuf));
    ICUNIT_GOTO_EQUAL(len, strlen(filebuf), len, EXIT1);

    ret = close(fd);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT1);

    g_TestCnt++;

    while (g_TestCnt < g_testNum) {
        sleep(1);
        dprintf(" sleep 59_02 \n");
    }

    dprintf(" end muti_jffs_059_02  2 \n");

    return NULL;

EXIT1:
    close(fd);

EXIT:
    g_TestCnt++;
    return NULL;
}


static UINT32 TestCase(VOID)
{
    INT32 fd, ret, i;
    CHAR pathname[JFFS_STANDARD_NAME_LENGTH] = { JFFS_PATH_NAME0 };
    CHAR bufname1[JFFS_STANDARD_NAME_LENGTH] = { JFFS_PATH_NAME0 };
    INT32 threadNum = 2;
    pthread_attr_t attr[threadNum];
    pthread_t threadId[threadNum];
    INT32 priority = 20;

    g_TestCnt = 0;

    ret = mkdir(bufname1, HIGHEST_AUTHORITY);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT);

    strcat_s(pathname, JFFS_STANDARD_NAME_LENGTH, "/test_59");
    // file size: 1024, write size: 10
    ret = JffsMultiWrite(pathname, 1024, 10, O_RDWR | O_CREAT, JFFS_WR_TYPE_TEST);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT1);

    dprintf("start create task\n");
    ret = PosixPthreadInit(&attr[0], priority);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT2);

    ret = pthread_create(&threadId[0], &attr[0], MutiJffs05901, (void *)0);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT2);

    ret = PosixPthreadInit(&attr[1], priority);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT2);

    ret = pthread_create(&threadId[1], &attr[1], MutiJffs05902, (void *)1);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT2);
    dprintf("end create task\n");

    while (g_TestCnt < g_testNum) {
        sleep(10); // sleep time: 10
        dprintf(" sleep  \n");
    }

    for (i = 0; i < threadNum; i++) {
        ret = pthread_join(threadId[i], NULL);
        ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT2);
    }

    for (i = 0; i < threadNum; i++) {
        ret = pthread_attr_destroy(&attr[i]);
        ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT2);
    }

    ICUNIT_ASSERT_EQUAL(g_TestCnt, 2, g_TestCnt); // test number: 2

    ret = unlink(pathname);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT1);

    ret = rmdir(bufname1);
    ICUNIT_GOTO_EQUAL(ret, JFFS_NO_ERROR, ret, EXIT);

    return JFFS_NO_ERROR;

EXIT2:
    for (i = 0; i < threadNum; i++) {
        pthread_join(threadId[i], NULL);
    }
    for (i = 0; i < threadNum; i++) {
        pthread_attr_destroy(&attr[i]);
    }
EXIT1:
    unlink(pathname);
EXIT:
    rmdir(bufname1);
    return JFFS_NO_ERROR;
}

VOID ItFsJffsMultipthread059(VOID)
{
    TEST_ADD_CASE("ItFsJffsMultipthread059", TestCase, TEST_VFS, TEST_JFFS, TEST_LEVEL3, TEST_PRESSURE);
}
