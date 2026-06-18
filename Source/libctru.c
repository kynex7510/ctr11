/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Break.h>
#include <CTR11/Log.h>
#include <CTR11/Assert.h>
#include <CTR11/Allocator.h>
#include <CTR11/Sync.h>
#include <CTR11/Unreachable.h>

#include "QTMRAM.h"

#include <stdarg.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

// CTR_BREAK

void impl_ctr11_break(void) {
    svcBreak(USERBREAK_PANIC);
    while (true) {}
}

// CTR_LOG

void impl_ctr11_vlog(const char* fmt, va_list args) {
    vfprintf(stderr, fmt, args);
    fflush(stderr);
}

// Allocator

#ifdef CTR11_ENABLE_QTMRAM

bool qtmramInitRegion(uintptr_t* regionBase, size_t* regionSize) {
    s64 base = 0;
    s64 size = 0;

    if (R_SUCCEEDED(svcGetProcessInfo(&base, CUR_PROCESS_HANDLE, 22))) {
        if (R_SUCCEEDED(svcGetProcessInfo(&size, CUR_PROCESS_HANDLE, 23))) {
            *regionBase = base;
            *regionSize = size;
        }
    }

    return base && size;
}

#endif // CTR11_ENABLE_QTMRAM

void* AllocMemAligned(MemType memType, size_t size, size_t alignment) {
    if (!alignment) {
        switch (memType) {
            case MemType_Virtual:
                return malloc(size);
            case MemType_FCRAM:
                return linearAlloc(size);
            case MemType_VRAM:
                return vramAlloc(size);
#ifdef CTR11_ENABLE_QTMRAM
            case MemType_QTMRAM:
                return qtmramAlloc(size);
#endif // CTR11_ENABLE_QTMRAM
            default:
                return NULL;
        }
    }

    switch (memType) {
        case MemType_Virtual:
            return memalign(alignment, size);
        case MemType_FCRAM:
            return linearMemAlign(size, alignment);
        case MemType_VRAM:
            return vramMemAlign(size, alignment);
#ifdef CTR11_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return qtmramMemAlign(size, alignment);
#endif // CTR11_ENABLE_QTMRAM
        default:
            return NULL;
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
    switch (GetMemType(p)) {
        case MemType_Virtual:
            free(p);
            break;
        case MemType_FCRAM:
            linearFree(p);
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
            CTR_UNREACHABLE("Invalid memory type");
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
        case MemType_Virtual:
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
            CTR_UNREACHABLE("Invalid memory type");
    }
}

MemType GetMemType(const void* p) {
    const u32 addr = (u32)p;

    if (addr >= OS_HEAP_AREA_BEGIN && addr < OS_HEAP_AREA_END)
        return MemType_Virtual;

    if (addr >= OS_FCRAM_VADDR && addr < (OS_FCRAM_VADDR + OS_FCRAM_SIZE))
        return MemType_FCRAM;

    if (addr >= OS_VRAM_VADDR && addr < (OS_VRAM_VADDR + OS_VRAM_SIZE))
        return MemType_VRAM;

#ifdef CTR11_ENABLE_QTMRAM
    if (addr >= OS_QTMRAM_VADDR && addr < (OS_QTMRAM_VADDR + OS_QTMRAM_SIZE))
        return MemType_QTMRAM;
#endif // CTR11_ENABLE_QTMRAM

    CTR_UNREACHABLE("Invalid address: 0x%08X", addr);
}

VRAMBank GetVRAMBank(const void* p) {
    const size_t bankSize = OS_VRAM_SIZE / 2;
    const u32 addr = (u32)p;

    if (addr >= OS_VRAM_VADDR && addr < (OS_VRAM_VADDR + bankSize))
        return VRAMBank_A;

    if (addr >= (OS_VRAM_VADDR + bankSize) && addr < (OS_VRAM_VADDR + bankSize * 2))
        return VRAMBank_B;

    CTR_UNREACHABLE("Invalid address: 0x%08X", addr);
}

size_t GetAllocSize(const void* p) {
    switch (GetMemType(p)) {
        case MemType_Virtual:
            return malloc_usable_size((void*)p);
        case MemType_FCRAM:
            return linearGetSize((void*)p);
        case MemType_VRAM:
            return vramGetSize((void*)p);
#ifdef CTR11_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return qtmramGetSize(p);
#endif // CTR11_ENABLE_QTMRAM
        default:
            CTR_UNREACHABLE("Invalid memory type");
    }
}

uintptr_t GetPhysicalAddress(const void* addr) { return osConvertVirtToPhys(addr); }

void* GetVirtualAddress(uintptr_t addr) {
#define CONVERT_REGION(_name)                                             \
    if (addr >= OS_##_name##_PADDR &&                                     \
        addr < (OS_##_name##_PADDR + OS_##_name##_SIZE))                  \
        return (void*)(addr - (OS_##_name##_PADDR + OS_##_name##_VADDR));

    CONVERT_REGION(FCRAM);
    CONVERT_REGION(VRAM);
    CONVERT_REGION(OLD_FCRAM);
    CONVERT_REGION(DSPRAM);
    CONVERT_REGION(QTMRAM);
    CONVERT_REGION(MMIO);

#undef CONVERT_REGION
    return NULL;
}

// Cache

void InvalidateDataCache(const void* addr, size_t size) {
    if (!IsMemVirtual(addr)) {
        CTR_BREAK_IF(R_FAILED(svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
    }
}

void FlushDataCache(const void* addr, size_t size) {
    if (!IsMemVirtual(addr)) {
        CTR_BREAK_IF(R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
    }
}

// Sync

void Yield(void) { svcSleepThread(0); }

Mutex CreateMutex(void) {
    LightLock* l = AllocMem(MemType_Virtual, sizeof(LightLock));
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
    CondVar* cv = AllocMem(MemType_Virtual, sizeof(CondVar));
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