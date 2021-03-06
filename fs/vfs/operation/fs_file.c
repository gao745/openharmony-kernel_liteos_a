/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
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

#include "fs_file.h"
#include "los_process_pri.h"
#include "fs/fd_table.h"
#include "fs/file.h"
#include "fs/fs.h"
#include "mqueue.h"
#ifdef LOSCFG_NET_LWIP_SACK
#include "lwip/sockets.h"
#endif

static void FileTableLock(struct fd_table_s *fdt)
{
    /* Take the semaphore (perhaps waiting) */
    while (sem_wait(&fdt->ft_sem) != 0) {
        /*
        * The only case that an error should occur here is if the wait was
        * awakened by a signal.
        */
        LOS_ASSERT(errno == EINTR);
    }
}

static void FileTableUnLock(struct fd_table_s *fdt)
{
    int ret = sem_post(&fdt->ft_sem);
    if (ret == -1) {
        PRINTK("sem_post error, errno %d \n", get_errno());
    }
}

static int AssignProcessFd(const struct fd_table_s *fdt, int minFd)
{
    if (fdt == NULL) {
        return VFS_ERROR;
    }

    if (minFd >= fdt->max_fds) {
        set_errno(EINVAL);
        return VFS_ERROR;
    }

    /* search unused fd from table */
    for (int i = minFd; i < fdt->max_fds; i++) {
        if (!FD_ISSET(i, fdt->proc_fds)) {
            return i;
        }
    }
    set_errno(EMFILE);
    return VFS_ERROR;
}

static struct fd_table_s *GetFdTable(void)
{
    struct fd_table_s *fdt = NULL;
    struct files_struct *procFiles = OsCurrProcessGet()->files;

    if (procFiles == NULL) {
        return NULL;
    }

    fdt = procFiles->fdt;
    if ((fdt == NULL) || (fdt->ft_fds == NULL)) {
        return NULL;
    }

    return fdt;
}

static bool IsValidProcessFd(struct fd_table_s *fdt, int procFd)
{
    if (fdt == NULL) {
        return false;
    }
    if ((procFd < 0) || (procFd >= fdt->max_fds)) {
        return false;
    }
    return true;
}

void AssociateSystemFd(int procFd, int sysFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return;
    }

    if (sysFd < 0) {
        return;
    }

    FileTableLock(fdt);
    fdt->ft_fds[procFd].sysFd = sysFd;
    FileTableUnLock(fdt);
}

int CheckProcessFd(int procFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return VFS_ERROR;
    }

    return OK;
}

int GetAssociatedSystemFd(int procFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return VFS_ERROR;
    }

    FileTableLock(fdt);
    if (fdt->ft_fds[procFd].sysFd < 0) {
        FileTableUnLock(fdt);
        return VFS_ERROR;
    }
    int sysFd = fdt->ft_fds[procFd].sysFd;
    FileTableUnLock(fdt);

    return sysFd;
}

/* Occupy the procFd, there are three circumstances:
 * 1.procFd is already associated, we need disassociate procFd with relevant sysfd.
 * 2.procFd is not allocated, we occupy it immediately.
 * 3.procFd is in open(), close(), dup() process, we return EBUSY immediately.
 */
int AllocSpecifiedProcessFd(int procFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return -EBADF;
    }

    FileTableLock(fdt);
    if (fdt->ft_fds[procFd].sysFd >= 0) {
        /* Disassociate procFd */
        fdt->ft_fds[procFd].sysFd = -1;
        FileTableUnLock(fdt);
        return OK;
    }

    if (FD_ISSET(procFd, fdt->proc_fds)) {
        /* procFd in race condition */
        FileTableUnLock(fdt);
        return -EBUSY;
    } else {
        /* Unused procFd */
        FD_SET(procFd, fdt->proc_fds);
    }

    FileTableUnLock(fdt);
    return OK;
}

void FreeProcessFd(int procFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return;
    }

    FileTableLock(fdt);
    FD_CLR(procFd, fdt->proc_fds);
    fdt->ft_fds[procFd].sysFd = -1;
    FileTableUnLock(fdt);
}

int DisassociateProcessFd(int procFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return VFS_ERROR;
    }

    FileTableLock(fdt);
    if (fdt->ft_fds[procFd].sysFd < 0) {
        FileTableUnLock(fdt);
        return VFS_ERROR;
    }
    int sysFd = fdt->ft_fds[procFd].sysFd;
    if (procFd >= MIN_START_FD) {
        fdt->ft_fds[procFd].sysFd = -1;
    }
    FileTableUnLock(fdt);

    return sysFd;
}

int AllocProcessFd(void)
{
    return AllocLowestProcessFd(MIN_START_FD);
}

int AllocLowestProcessFd(int minFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (fdt == NULL) {
        return VFS_ERROR;
    }

    /* minFd should be a positive number,and 0,1,2 had be distributed to stdin,stdout,stderr */
    if (minFd < MIN_START_FD) {
        minFd = MIN_START_FD;
    }

    FileTableLock(fdt);

    int procFd = AssignProcessFd(fdt, minFd);
    if (procFd == VFS_ERROR) {
        FileTableUnLock(fdt);
        return VFS_ERROR;
    }

    /* occupy the fd set */
    FD_SET(procFd, fdt->proc_fds);
    FileTableUnLock(fdt);

    return procFd;
}

int AllocAndAssocProcessFd(int sysFd, int minFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (fdt == NULL) {
        return VFS_ERROR;
    }

    /* minFd should be a positive number,and 0,1,2 had be distributed to stdin,stdout,stderr */
    if (minFd < MIN_START_FD) {
        minFd = MIN_START_FD;
    }

    FileTableLock(fdt);

    int procFd = AssignProcessFd(fdt, minFd);
    if (procFd == VFS_ERROR) {
        FileTableUnLock(fdt);
        return VFS_ERROR;
    }

    /* occupy the fd set */
    FD_SET(procFd, fdt->proc_fds);
    fdt->ft_fds[procFd].sysFd = sysFd;
    FileTableUnLock(fdt);

    return procFd;
}

int AllocAndAssocSystemFd(int procFd, int minFd)
{
    struct fd_table_s *fdt = GetFdTable();

    if (!IsValidProcessFd(fdt, procFd)) {
        return VFS_ERROR;
    }

    int sysFd = alloc_fd(minFd);
    if (sysFd < 0) {
        return VFS_ERROR;
    }

    FileTableLock(fdt);
    fdt->ft_fds[procFd].sysFd = sysFd;
    FileTableUnLock(fdt);

    return sysFd;
}

static void FdRefer(int sysFd)
{
    if ((sysFd > STDERR_FILENO) && (sysFd < CONFIG_NFILE_DESCRIPTORS)) {
        files_refer(sysFd);
    }
#if defined(LOSCFG_NET_LWIP_SACK)
    if ((sysFd >= CONFIG_NFILE_DESCRIPTORS) && (sysFd < (CONFIG_NFILE_DESCRIPTORS + CONFIG_NSOCKET_DESCRIPTORS))) {
        socks_refer(sysFd);
    }
#endif
#if defined(LOSCFG_COMPAT_POSIX)
    if ((sysFd >= MQUEUE_FD_OFFSET) && (sysFd < (MQUEUE_FD_OFFSET + CONFIG_NQUEUE_DESCRIPTORS))) {
        mqueue_refer(sysFd);
    }
#endif
}

static void FdClose(int sysFd, unsigned int targetPid)
{
    UINT32 intSave;

    if ((sysFd > STDERR_FILENO) && (sysFd < CONFIG_NFILE_DESCRIPTORS)) {
        LosProcessCB *processCB = OS_PCB_FROM_PID(targetPid);
        SCHEDULER_LOCK(intSave);
        if (OsProcessIsInactive(processCB)) {
            SCHEDULER_UNLOCK(intSave);
            return;
        }
        SCHEDULER_UNLOCK(intSave);

        files_close_internal(sysFd, processCB);
    }
#if defined(LOSCFG_NET_LWIP_SACK)
    if ((sysFd >= CONFIG_NFILE_DESCRIPTORS) && (sysFd < (CONFIG_NFILE_DESCRIPTORS + CONFIG_NSOCKET_DESCRIPTORS))) {
        socks_close(sysFd);
    }
#endif
#if defined(LOSCFG_COMPAT_POSIX)
    if ((sysFd >= MQUEUE_FD_OFFSET) && (sysFd < (MQUEUE_FD_OFFSET + CONFIG_NQUEUE_DESCRIPTORS))) {
        mq_close((mqd_t)sysFd);
    }
#endif
}

static struct fd_table_s *GetProcessFTable(unsigned int pid, sem_t *semId)
{
    UINT32 intSave;
    struct files_struct *procFiles = NULL;
    LosProcessCB *processCB = OS_PCB_FROM_PID(pid);

    SCHEDULER_LOCK(intSave);
    if (OsProcessIsInactive(processCB)) {
        SCHEDULER_UNLOCK(intSave);
        return NULL;
    }

    procFiles = processCB->files;
    if (procFiles == NULL || procFiles->fdt == NULL) {
        SCHEDULER_UNLOCK(intSave);
        return NULL;
    }

    *semId = procFiles->fdt->ft_sem;
    SCHEDULER_UNLOCK(intSave);

    return procFiles->fdt;
}

int CopyFdToProc(int fd, unsigned int targetPid)
{
#if !defined(LOSCFG_NET_LWIP_SACK) && !defined(LOSCFG_COMPAT_POSIX) && !defined(LOSCFG_FS_VFS)
    return -ENOSYS;
#else
    int sysFd;
    struct fd_table_s *fdt = NULL;
    int procFd;
    sem_t semId;

    if (OS_PID_CHECK_INVALID(targetPid)) {
        return -EINVAL;
    }

    sysFd = GetAssociatedSystemFd(fd);
    if (sysFd < 0) {
        return -EBADF;
    }

    FdRefer(sysFd);
    fdt = GetProcessFTable(targetPid, &semId);
    if (fdt == NULL || fdt->ft_fds == NULL) {
        FdClose(sysFd, targetPid);
        return -EPERM;
    }

    /* Take the semaphore (perhaps waiting) */
    if (sem_wait(&semId) != 0) {
        /* Target process changed */
        FdClose(sysFd, targetPid);
        return -ESRCH;
    }

    procFd = AssignProcessFd(fdt, 3);
    if (procFd < 0) {
        if (sem_post(&semId) == -1) {
            PRINT_ERR("sem_post error, errno %d \n", get_errno());
        }
        FdClose(sysFd, targetPid);
        return -EPERM;
    }

    /* occupy the fd set */
    FD_SET(procFd, fdt->proc_fds);
    fdt->ft_fds[procFd].sysFd = sysFd;
    if (sem_post(&semId) == -1) {
        PRINTK("sem_post error, errno %d \n", get_errno());
    }

    return procFd;
#endif
}

int CloseProcFd(int procFd, unsigned int targetPid)
{
#if !defined(LOSCFG_NET_LWIP_SACK) && !defined(LOSCFG_COMPAT_POSIX) && !defined(LOSCFG_FS_VFS)
    return -ENOSYS;
#else
    int sysFd;
    struct fd_table_s *fdt = NULL;
    sem_t semId;

    if (OS_PID_CHECK_INVALID(targetPid)) {
        return -EINVAL;
    }

    fdt = GetProcessFTable(targetPid, &semId);
    if (fdt == NULL || fdt->ft_fds == NULL) {
        return -EPERM;
    }

    /* Take the semaphore (perhaps waiting) */
    if (sem_wait(&semId) != 0) {
        /* Target process changed */
        return -ESRCH;
    }

    if (!IsValidProcessFd(fdt, procFd)) {
        if (sem_post(&semId) == -1) {
            PRINTK("sem_post error, errno %d \n", get_errno());
        }
        return -EPERM;
    }

    sysFd = fdt->ft_fds[procFd].sysFd;
    if (sysFd < 0) {
        if (sem_post(&semId) == -1) {
            PRINTK("sem_post error, errno %d \n", get_errno());
        }
        return -EPERM;
    }

    /* clean the fd set */
    FD_CLR(procFd, fdt->proc_fds);
    fdt->ft_fds[procFd].sysFd = -1;
    if (sem_post(&semId) == -1) {
        PRINTK("sem_post error, errno %d \n", get_errno());
    }
    FdClose(sysFd, targetPid);

    return 0;
#endif
}