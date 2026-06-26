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

extern u32 __ctru_heap;
extern u32 __ctru_linear_heap;
extern u32 __ctru_heap_size;
extern u32 __ctru_linear_heap_size;

// CTR_BREAK

void impl_ctr11_break(void) {
    while (true)
        svcBreak(USERBREAK_PANIC);
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
            case MemType_Application:
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
                CTR_UNREACHABLE("Invalid memory type");
        }
    }

    switch (memType) {
        case MemType_Application:
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

    switch (GetMemType(p, 0)) {
        case MemType_Application:
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
    const VRAMBank bank = GetVRAMBank(p, 0);

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

    switch (GetMemType(p, 0)) {
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
            return NULL;
    }
}

static inline bool checkRange(u32 p, size_t size, u32 rangeBase, size_t rangeSize) {
    const size_t offset = p - rangeBase;
    return p >= rangeBase && offset < rangeSize && (rangeSize - offset) > size;
}

MemType GetMemType(const void* p, size_t size) {
    const u32 addr = (u32)p;

    if (checkRange(addr, size, OS_HEAP_AREA_BEGIN, OS_HEAP_AREA_END))
        return MemType_Application;

    if (checkRange(addr, size, OS_FCRAM_VADDR, OS_FCRAM_SIZE))
        return MemType_FCRAM;

    if (checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE))
        return MemType_VRAM;

    if (checkRange(addr, size, OS_QTMRAM_VADDR, OS_QTMRAM_SIZE))
        return MemType_QTMRAM;

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return MemType_Unknown;
}

VRAMBank GetVRAMBank(const void* p, size_t size) {
    const size_t bankSize = OS_VRAM_SIZE / 2;
    const u32 addr = (u32)p;

    if (checkRange(addr, size, OS_VRAM_VADDR, bankSize))
        return VRAMBank_A;

    if (checkRange(addr, size, OS_VRAM_VADDR + bankSize, bankSize))
        return VRAMBank_B;

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return VRAMBank_Unknown;
}

size_t GetAllocSize(const void* p) {
    switch (GetMemType(p, 0)) {
        case MemType_Application:
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
            CTR_LOG_LOCATION("Invalid memory type");
            return 0;
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

bool IsCPUAccessible(const void* p, size_t size, uint32_t access) {
    const u32 addr = (u32)p;

    // These two are fixed.
    const uint32_t heapAccess = MemAccess_Read | MemAccess_Write;
    const uint32_t fcramAccess = MemAccess_Read | MemAccess_Write;

    if (checkRange(addr, size, __ctru_heap, __ctru_heap_size))
        return (access & heapAccess) == access;

    if (checkRange(addr, size, __ctru_linear_heap, __ctru_linear_heap_size))
        return (access & fcramAccess) == access;

    // VRAM access depends on the ExHeader.
    if (checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE)) {
        MemInfo memInfo;
        PageInfo pageInfo;

        CTR_BREAK_IF(R_FAILED(svcQueryMemory(&memInfo, &pageInfo, (u32)p)));

        uint32_t vramAccess = 0;
        
        if (memInfo.perm & MEMPERM_READ)
            vramAccess |= MemAccess_Read;

        if (memInfo.perm & MEMPERM_WRITE)
            vramAccess |= MemAccess_Write;

        return (access & vramAccess) == access;
    }
    
#ifdef CTR11_ENABLE_QTMRAM
    uintptr_t qtmramBase = 0;
    size_t qtmramSize = 0;
    qtmramQueryRegion(&qtmramBase, &qtmramSize);

    // QTMRAM access depends on the ExHeader.
    if (checkRange(addr, size, qtmramBase, qtmramSize)) {
        MemInfo memInfo;
        PageInfo pageInfo;

        CTR_BREAK_IF(R_FAILED(svcQueryMemory(&memInfo, &pageInfo, (u32)p)));

        uint32_t qtmramAccess = 0;
        
        if (memInfo.perm & MEMPERM_READ)
            qtmramAccess |= MemAccess_Read;

        if (memInfo.perm & MEMPERM_WRITE)
            qtmramAccess |= MemAccess_Write;

        return (access & qtmramAccess) == access;
    }
#endif // CTR11_ENABLE_QTMRAM

    return false;
}

bool IsGPUAccessible(const void* p, size_t size, uint32_t access) {
    const u32 addr = (u32)p;

    // If it's accessible GPU has RW access.
    const uint32_t sharedAccess = MemAccess_Read | MemAccess_Write;

    bool b = checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE) ||
        checkRange(addr, size, __ctru_linear_heap, __ctru_linear_heap_size);

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

void InvalidateDataCache(const void* addr, size_t size) {
    CTR_BREAK_IF(R_FAILED(svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
}

void FlushDataCache(const void* addr, size_t size) {
    CTR_BREAK_IF(R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
}

// Sync

void Yield(void) { svcSleepThread(0); }

Mutex CreateMutex(void) {
    LightLock* l = AllocMem(MemType_Application, sizeof(LightLock));
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
    CondVar* cv = AllocMem(MemType_Application, sizeof(CondVar));
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