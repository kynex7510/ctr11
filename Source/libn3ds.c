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
#include <drivers/cache.h>
#include <kmutex.h>
#include <ksemaphore.h>

#include <CTR11/Break.h>
#include <CTR11/Log.h>
#include <CTR11/Assert.h>
#include <CTR11/Allocator.h>
#include <CTR11/Sync.h>
#include <CTR11/Unreachable.h>

#include "QTMRAM.h"

#include <stdarg.h>
#include <malloc.h>
#include <string.h>

// CTR_BREAK

void impl_ctr11_break() { panic(); }

// CTR_LOG

void impl_ctr11_vlog(const char* fmt, va_list args) {
    // TODO: ideally we would like to print to the dspico.
    char buf[256];
    ee_vsnprintf(buf, 256, fmt, args);
    ee_puts(buf);
}

// Allocator

#ifdef CTR11_ENABLE_QTMRAM

bool qtmramInitRegion(uintptr_t* regionBase, size_t* regionSize) {
    // TODO: mmu checks?

    const u16 gpuprot = getCfg11Regs()->gpuprot;
    const u8 qtmramFactor = (gpuprot >> 9) & 0x03;
    const size_t qtmramSize = QTM_RAM_SIZE - (qtmramFactor * 0x100000);
    
    *regionBase = QTM_RAM_BASE;
    *regionSize = qtmramSize;
    return true;
}

#endif // CTR11_ENABLE_QTMRAM

void* AllocMemAligned(MemType memType, size_t size, size_t alignment) {
    if (!alignment) {
        switch (memType) {
            case MemType_Application:
                return malloc(size);
            case MemType_FCRAM:
                return fcramAlloc(size);
            case MemType_VRAM:
                return vramAlloc(size);
#ifdef CTR11_ENABLE_QTMRAM
            case MemType_QTMRAM:
                return qtmramAlloc(size);
#endif // CTR11_ENABLE_QTMRAM
            default:
                CTR_UNREACHABLE("Invalid memory type");
        }
    }

    switch (memType) {
        case MemType_Application:
            return memalign(alignment, size);
        case MemType_FCRAM:
            return fcramMemAlign(size, alignment);
        case MemType_VRAM:
            return vramMemAlign(size, alignment);
#ifdef CTR11_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return qtmramMemAlign(size, alignment);
#endif // CTR11_ENABLE_QTMRAM
        default:
            CTR_UNREACHABLE("Invalid memory type");
    }
}

static inline vramAllocPos getVRAMPos(VRAMBank bank) {
    switch (bank) {
        case VRAMBank_A:
            return VRAM_ALLOC_A;
        case VRAMBank_B:
            return VRAM_ALLOC_B;
        case VRAMBank_Any:
            return VRAM_ALLOC_ANY;
        default:
            CTR_UNREACHABLE("Invalid VRAM bank");
    }
}

void* AllocMemAlignedVRAM(VRAMBank bank, size_t size, size_t aligment) {
    if (!aligment)
        return vramAllocAt(size, getVRAMPos(bank));
    
    return vramMemAlignAt(size, aligment, getVRAMPos(bank));
}

void FreeMem(void* p) {
    if (!p)
        return;

    switch (GetMemType(p)) {
        case MemType_Application:
            free(p);
            break;
        case MemType_FCRAM:
            fcramFree(p);
            break;
        case MemType_VRAM:
            vramFree(p);
            break;
#ifdef CTR11_ENABLE_QTMRAM
        case MemType_QTMRAM:
            qtmramFree(p);
            break;
#endif // CTR11_ENABLE_QTMRAM
        default:
            CTR_LOG_LOCATION("Invalid memory type");
    }
}

static void* genericRealloc(MemType type, void* p, size_t size) {
    void* q = AllocMem(type, size);
    if (q) {
        const size_t oldSize = GetAllocSize(p);
        memcpy(q, p, size < oldSize ? size : oldSize);
        FreeMem(p);
    }

    return q;
}

static inline void* vramReallocCustom(void* p, size_t newSize) {
    const VRAMBank bank = GetVRAMBank(p);

    // If the new size is less than the old size, reallocation must succeed.
    const size_t oldSize = GetAllocSize(p);
    if (newSize < oldSize) {
        FreeMem(p);
        void* newp = AllocMemVRAM(bank, newSize);
        CTR_BREAK_IF(!newp);
        return newp;
    }

    // Try to realloc memory in the same bank first.
    void* q = AllocMemVRAM(bank, newSize);
    if (!q)
        q = AllocMemVRAM(bank == VRAMBank_A ? VRAMBank_B : VRAMBank_A, newSize);

    if (q)
        FreeMem(p);

    return q;
}

void* ReallocMem(void* p, size_t newSize) {
    if (newSize == 0) {
        FreeMem(p);
        return NULL;
    }

    switch (GetMemType(p)) {
        case MemType_Application:
            return realloc(p, newSize);
        case MemType_FCRAM:
            return genericRealloc(MemType_FCRAM, p, newSize);
        case MemType_VRAM:
            return vramReallocCustom(p, newSize);
#ifdef CTR11_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return genericRealloc(MemType_QTMRAM, p, newSize);
#endif // CTR11_ENABLE_QTMRAM
        default:
            CTR_LOG_LOCATION("Invalid memory type");
    }
}

static inline bool checkRange(u32 p, size_t size, u32 rangeBase, size_t rangeSize) {
    const size_t offset = p - rangeBase;
    return p >= rangeBase && offset < rangeSize && (rangeSize - offset) > size;
}

MemType GetMemType(const void* p, size_t size) {
    const u32 addr = (u32)p;

    if (checkRange(addr, size, AXI_RAM_BASE, AXI_RAM_SIZE))
        return MemType_Application;

    if (checkRange(addr, size, FCRAM_BASE, FCRAM_SIZE + FCRAM_EXT_SIZE))
        return MemType_FCRAM;

    if (checkRange(addr, size, VRAM_BASE, VRAM_SIZE))
        return MemType_VRAM;

    if (checkRange(addr, size, QTM_RAM_BASE, QTM_RAM_SIZE))
        return MemType_QTMRAM;

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return MemType_Unknown;
}

VRAMBank GetVRAMBank(const void* p, size_t size) {
    const u32 addr = (u32)p;

    if (checkRange(addr, size, VRAM_BANK0, VRAM_BANK_SIZE))
        return VRAMBank_A;

    if (checkRange(addr, size, VRAM_BANK1, VRAM_BANK_SIZE))
        return VRAMBank_B;

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return VRAMBank_Unknown;
}

size_t GetAllocSize(const void* p) {
    switch (GetMemType(p, 0)) {
        case MemType_Application:
            return malloc_usable_size((void*)p);
        case MemType_FCRAM:
            return fcramGetSize((void*)p);
        case MemType_VRAM:
            return vramGetSize((void*)p);
#ifdef CTR11_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return qtmramGetSize(p);
#endif // CTR11_ENABLE_QTMRAM
        default:
            CTR_LOG_LOCATION("Invalid memory type");
            return 0;
    }
}

uintptr_t GetPhysicalAddress(const void* addr) { return (uintptr_t)addr; }
void* GetVirtualAddress(uintptr_t addr) { return (void*)addr; }

bool IsCPUAccessible(const void* p, size_t size, uint32_t access) {
    const u32 addr = (u32)p;

    // Everything is mapped as RW.
    const uint32_t sharedAccess = MemAccess_Read | MemAccess_Write;

    // TODO: check this.
    bool b = checkRange(addr, size, AXI_RAM_BASE, AXI_RAM_SIZE) ||
        checkRange(addr, size, FCRAM_BASE, FCRAM_SIZE + FCRAM_EXT_SIZE) ||
        checkRange(addr, size, VRAM_BASE, VRAM_SIZE);

#ifdef CTR11_ENABLE_QTMRAM
    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }
#endif // CTR11_ENABLE_QTMRAM

    if (b)
        b = (access & sharedAccess) == access;

    return b;
}

bool IsGPUAccessible(const void* p, size_t size, uint32_t access) {
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

#ifdef CTR11_ENABLE_QTMRAM
    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }
#endif // CTR11_ENABLE_QTMRAM

    if (b)
        b = (access & sharedAccess) == access;

    return b;
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
    CV cv = AllocMem(MemType_Application, sizeof(*cv));
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