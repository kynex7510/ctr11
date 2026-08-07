/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <arm.h>
#include <debug.h>
#include <mem_map.h>
#include <arm11/fmt.h>
#include <arm11/allocator/fcram.h>
#include <arm11/allocator/vram.h>
#include <arm11/drivers/cfg11.h>
#include <arm11/drivers/timer.h>
#include <drivers/cache.h>
#include <kmutex.h>
#include <ksemaphore.h>

#include <CTR11/Break.h>
#include <CTR11/Log.h>
#include <CTR11/Assert.h>
#include <CTR11/Memory.h>
#include <CTR11/Sync.h>
#include <CTR11/Unreachable.h>
#include <CTR11/Tick.h>

#include "QTMRAM.h"

#include <stdarg.h>
#include <malloc.h>
#include <string.h>

// fake_heap_start is not set, heap starts after .bss.
extern char __bss_end__;
extern char* fake_heap_end;

// CTR_BREAK

void impl_ctr11_break() { panic(); }

// CTR_LOG

ssize_t con_write(const char *ptr, size_t len);

void impl_ctr11_vlog(const char* fmt, va_list args) {
    // TODO: ideally we would like to print to the dspico.
    char buf[256];
    ee_vsnprintf(buf, 256, fmt, args);
    con_write(buf, strlen(buf));
}

// Allocator

#ifdef CTR_ENABLE_QTMRAM

bool qtmramInitRegion(uintptr_t* regionBase, size_t* regionSize) {
    // TODO: mmu checks?

    const u16 gpuprot = getCfg11Regs()->gpuprot;
    const u8 qtmramFactor = (gpuprot >> 9) & 0x03;
    const size_t qtmramSize = QTM_RAM_SIZE - (qtmramFactor * 0x100000);
    
    *regionBase = QTM_RAM_BASE;
    *regionSize = qtmramSize;
    return true;
}

#endif // CTR_ENABLE_QTMRAM

void* AllocMemAligned(uint32_t memTypes, size_t size, size_t alignment) {
    if (!size)
        return NULL;

    if (!alignment)
        alignment = 1;

    if (memTypes & MemType_AppHeap) {
        void* p = memalign(alignment, size);
        if (p)
            return p;
    }

    if (memTypes & MemType_FCRAM) {
        void* p = fcramMemAlign(size, alignment);
        if (p)
            return p;
    }

    // Let the allocator decide.
    if ((memTypes & (MemType_VRAM_A | MemType_VRAM_B)) == (MemType_VRAM_A | MemType_VRAM_B)) {
        void* p = vramMemAlign(size, alignment);
        if (p)
            return p;
    } else {
        if (memTypes & MemType_VRAM_A) {
            void* p = vramMemAlignAt(size, alignment, VRAM_ALLOC_A);
            if (p)
                return p;
        }

        if (memTypes & MemType_VRAM_B) {
            void* p = vramMemAlignAt(size, alignment, VRAM_ALLOC_B);
            if (p)
                return p;
        }
    }

#ifdef CTR_ENABLE_QTMRAM
    if (memTypes & MemType_QTMRAM) {
        void* p = qtmramMemAlign(size, alignment);
        if (p)
            return p;
    }
#endif // CTR_ENABLE_QTMRAM

    return NULL;
}

void FreeMem(void* p) {
    if (!p)
        return;

    switch (GetMemType(p, 0)) {
        case MemType_AppHeap:
            free(p);
            break;
        case MemType_FCRAM:
            fcramFree(p);
            break;
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            vramFree(p);
            break;
#ifdef CTR_ENABLE_QTMRAM
        case MemType_QTMRAM:
            qtmramFree(p);
            break;
#endif // CTR_ENABLE_QTMRAM
        default:
            CTR_LOG_LOCATION("Invalid memory type");
    }
}

static inline bool checkRange(u32 p, size_t size, u32 rangeBase, size_t rangeSize) {
    const size_t offset = p - rangeBase;
    return p >= rangeBase && offset < rangeSize && (rangeSize - offset) > size;
}

uint32_t GetMemType(const void* p, size_t size) {
    const u32 addr = (u32)p;

    if (checkRange(addr, size, (u32)&__bss_end__, fake_heap_end - &__bss_end__))
        return MemType_AppHeap;

    if (checkRange(addr, size, FCRAM_BASE, FCRAM_SIZE + FCRAM_EXT_SIZE))
        return MemType_FCRAM;

    if (checkRange(addr, size, VRAM_BANK0, VRAM_BANK_SIZE))
        return MemType_VRAM_A;

    if (checkRange(addr, size, VRAM_BANK1, VRAM_BANK_SIZE))
        return MemType_VRAM_B;

    if (checkRange(addr, size, QTM_RAM_BASE, QTM_RAM_SIZE))
        return MemType_QTMRAM;

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return 0;
}

size_t GetAllocSize(const void* p) {
    switch (GetMemType(p, 0)) {
        case MemType_AppHeap:
            return malloc_usable_size((void*)p);
        case MemType_FCRAM:
            return fcramGetSize((void*)p);
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            return vramGetSize((void*)p);
#ifdef CTR_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return qtmramGetSize(p);
#endif // CTR_ENABLE_QTMRAM
        default:
            CTR_LOG_LOCATION("Invalid memory type");
            return 0;
    }
}

uintptr_t GetPhysicalAddress(const void* addr) { return (uintptr_t)addr; }
void* GetVirtualAddress(uintptr_t addr) { return (void*)addr; }

static uint32_t queryMemoryAccess(u32 addr) {
    // Walk the translation tables.
    const u32* l1Table = (const u32*)(__getTtbr0() & 0xFFFFF000);
    const u32 l1Entry = l1Table[addr >> 20];

    // Check L2 entries.
    if ((l1Entry & 0x03) == 0x01) {
        const u32* l2Table = (const u32*)(l1Entry & 0xFFFFFC00);
        const u32 l2Entry = l2Table[(addr >> 12) & 0xFF];
        const u32 perm = (l2Entry >> 4) & 0x3F;
        if (perm)
            return (perm >> 5) ? MemAccess_Read : (MemAccess_Read | MemAccess_Write);

        return 0;
    }

    // Check sections/supersections.
    if ((l1Entry & 0x03) == 0x02) {
        // Always RW if mapped.
        // TODO: implement generic logic.
        return ((l1Entry >> 10) & 0x3F) ? (MemAccess_Read | MemAccess_Write) : 0;
    }

    return 0;
}

static uint32_t queryRegionAccess(u32 addr, size_t size) {
    const uint32_t access = queryMemoryAccess(addr);

    if (!access)
        return 0;

    for (size_t i = 0x1000; i < size; i += 0x1000) {
        if (queryMemoryAccess(addr + i) != access)
            return 0;
    }

    return access;
}

uint32_t GetCPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // These are all RW.
    const uint32_t sharedAccess = MemAccess_Read | MemAccess_Write;

    bool b = checkRange(addr, size, (u32)&__bss_end__, fake_heap_end - &__bss_end__) ||
        checkRange(addr, size, FCRAM_BASE, FCRAM_SIZE + FCRAM_EXT_SIZE) ||
        checkRange(addr, size, VRAM_BASE, VRAM_SIZE);

#ifdef CTR_ENABLE_QTMRAM
    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }
#endif // CTR_ENABLE_QTMRAM

    return b ? sharedAccess : queryRegionAccess(addr, size);
}

uint32_t GetGPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // If it's accessible GPU has RW access.
    const uint32_t sharedAccess = MemAccess_Read | MemAccess_Write;

    const u16 gpuprot = getCfg11Regs()->gpuprot;

    const u8 fcramFactor = gpuprot & 0x0F;
    const u8 fcramExtFactor = (gpuprot >> 4) & 0x0F;
    const bool axiwramCutoff = (gpuprot >> 8) & 0x01;

    const size_t fcramSize = FCRAM_SIZE - (fcramFactor * 0x800000);
    const size_t fcramExtSize = FCRAM_EXT_SIZE - (fcramExtFactor * 0x800000);

    bool b = axiwramCutoff && checkRange(addr, size, AXI_RAM_BASE, AXI_RAM_SIZE);

    if (!b) {
        b = checkRange(addr, size, FCRAM_BASE, fcramSize) ||
            checkRange(addr, size, FCRAM_EXT_BASE, fcramExtSize) ||
            checkRange(addr, size, VRAM_BASE, VRAM_SIZE);
    }

#ifdef CTR_ENABLE_QTMRAM
    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }
#endif // CTR_ENABLE_QTMRAM

    return b ? sharedAccess : 0;
}

// Cache

void InvalidateDataCache(const void* addr, size_t size) { invalidateDCacheRange(addr, size); }
void FlushDataCache(const void* addr, size_t size) { flushDCacheRange(addr, size); }

// Sync

struct CTRCVImpl {
    KHandle sema;
    u32 waiters;
};

void Yield(void) { yieldTask(); }

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
    CV cv = AllocMem(MemType_AppHeap, sizeof(*cv));
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

// Tick

void TickTimerStart(TickTimer* t) {
    CTR_ASSERT(t);
    // With a prescaler value of 1 the timer decrements every 2 clock cycles.
    TIMER_start(1, 0xFFFFFFFFu, TIMER_SINGLE_SHOT);
    *t = 0xFFFFFFFF;
}

uint64_t TickTimerStop(TickTimer* t) {
    CTR_ASSERT(t);

    if (*t) {
        const uint64_t delta = (*t - TIMER_stop()) << 1;
        *t = 0;
        return delta;
    }

    return 0;
}

uint64_t TickTimerInterval(TickTimer* t) {
    CTR_ASSERT(t);
    const uint64_t newp = TIMER_getTicks();
    const uint64_t delta = (*t - newp) << 1;
    *t = newp;
    return delta;
}