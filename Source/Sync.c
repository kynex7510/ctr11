/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <types.h>
#include <arm.h>
#include <kmutex.h>
#include <ksemaphore.h>
#include <arm11/drivers/timer.h>
#else
#include <3ds.h>
#endif // CTR_BM

#include <CTR11/Sync.h>
#include <CTR11/Memory.h>
#include <CTR11/Break.h>
#include <CTR11/Assert.h>

#ifdef CTR_BM

struct CTRCVImpl {
    KHandle sema;
    u32 waiters;
};

void Sleep(uint64_t ns) {
    if (!ns)
        yieldTask();

    TIMER_sleepNs(ns);
}

Mutex CreateMutex(void) {
    KHandle m = createMutex();
    CTR_BREAK_IF(!m);
    return (Mutex)m;
}

void DestroyMutex(Mutex m) {
    CTR_ASSERT(m);
    deleteMutex((KHandle)m);
}

void AcquireMutex(Mutex m)  {
    CTR_ASSERT(m);
    CTR_BREAK_IF(lockMutex((KHandle)m) != KRES_OK);
}

void ReleaseMutex(Mutex m) {
    CTR_ASSERT(m);
    CTR_BREAK_IF(unlockMutex((KHandle)m) != KRES_OK);
}

CV CreateCV(void)  {
    CV cv = AllocTypedMem(sizeof(*cv), MemType_AppHeap);
    CTR_BREAK_IF(cv == NULL);
    cv->sema = createSemaphore(0);
    CTR_BREAK_IF(!cv->sema);
    cv->waiters = 0;
    return cv;
}

void DestroyCV(CV cv) {
    CTR_ASSERT(cv);
    deleteSemaphore(cv->sema);
    FreeMem(cv);
}

void WaitCV(CV cv, Mutex m) {
    CTR_ASSERT(cv);
    CTR_ASSERT(m);

    u32 waiters;
    do {
        waiters = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, waiters + 1));

    ReleaseMutex(m);
    CTR_BREAK_IF(waitForSemaphore(cv->sema) != KRES_OK);
    AcquireMutex(m);
}

void NotifyCV(CV cv, size_t count) {
    CTR_ASSERT(cv);

    u32 waiters;

    __dmb();
    do {
        waiters = __ldrex(&cv->waiters);
    } while (__strex(&cv->waiters, waiters > count ? waiters - count : 0));

    if (waiters) {
        signalSemaphore(cv->sema, waiters, false);
    } else {
        __dmb();
    }
}

void BroadcastCV(CV cv) {
    CTR_ASSERT(cv);
    NotifyCV(cv, UINT32_MAX);
}

#else

void Sleep(uint64_t ns) { svcSleepThread(ns); }

Mutex CreateMutex(void) {
    LightLock* l = AllocTypedMem(sizeof(LightLock), MemType_AppHeap);
    CTR_BREAK_IF(l == NULL);
    LightLock_Init(l);
    return (Mutex)l;
}

void DestroyMutex(Mutex m) {
    CTR_ASSERT(m);
    FreeMem(m);
}

void AcquireMutex(Mutex m) {
    CTR_ASSERT(m);
    LightLock_Lock((LightLock*)m);
}

void ReleaseMutex(Mutex m) {
    CTR_ASSERT(m);
    LightLock_Unlock((LightLock*)m);
}

CV CreateCV(void) {
    CondVar* cv = AllocTypedMem(sizeof(CondVar), MemType_AppHeap);
    CTR_BREAK_IF(cv == NULL);
    CondVar_Init(cv);
    return (CV)cv;
}

void DestroyCV(CV cv) {
    CTR_ASSERT(cv);
    FreeMem(cv);
}

void WaitCV(CV cv, Mutex m) {
    CTR_ASSERT(cv);
    CTR_ASSERT(m);

    CondVar_Wait((CondVar*)cv, (LightLock*)m);
}

void NotifyCV(CV cv, size_t count) {
    CTR_ASSERT(cv);
    CondVar_WakeUp((CondVar*)cv, count);
}

void BroadcastCV(CV cv) {
    CTR_ASSERT(cv);
    CondVar_Broadcast((CondVar*)cv);
}

#endif // CTR_BM