/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <3ds.h>

#include <CTR11/Break.h>
#include <CTR11/Log.h>
#include <CTR11/Assert.h>
#include <CTR11/Memory.h>
#include <CTR11/Sync.h>
#include <CTR11/Unreachable.h>
#include <CTR11/Tick.h>

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

#ifdef CTR_ENABLE_QTMRAM

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

#endif // CTR_ENABLE_QTMRAM

void* AllocMemAligned(MemType memType, size_t size, size_t alignment) {
    if (!alignment) {
        switch (memType) {
            case MemType_AppHeap:
                return malloc(size);
            case MemType_FCRAM:
                return linearAlloc(size);
            case MemType_VRAM:
                return vramAlloc(size);
#ifdef CTR_ENABLE_QTMRAM
            case MemType_QTMRAM:
                return qtmramAlloc(size);
#endif // CTR_ENABLE_QTMRAM
            default:
                CTR_UNREACHABLE("Invalid memory type");
        }
    }

    switch (memType) {
        case MemType_AppHeap:
            return memalign(alignment, size);
        case MemType_FCRAM:
            return linearMemAlign(size, alignment);
        case MemType_VRAM:
            return vramMemAlign(size, alignment);
#ifdef CTR_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return qtmramMemAlign(size, alignment);
#endif // CTR_ENABLE_QTMRAM
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
        case MemType_AppHeap:
            free(p);
            break;
        case MemType_FCRAM:
            linearFree(p);
            break;
        case MemType_VRAM:
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
        case MemType_AppHeap:
            return realloc(p, newSize);
        case MemType_FCRAM:
            return genericRealloc(MemType_FCRAM, p, newSize);
        case MemType_VRAM:
            return vramReallocCustom(p, newSize);
#ifdef CTR_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return genericRealloc(MemType_QTMRAM, p, newSize);
#endif // CTR_ENABLE_QTMRAM
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
        return MemType_AppHeap;

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
        case MemType_AppHeap:
            return malloc_usable_size((void*)p);
        case MemType_FCRAM:
            return linearGetSize((void*)p);
        case MemType_VRAM:
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

uintptr_t GetPhysicalAddress(const void* addr) {
    const uintptr_t p = (uintptr_t)addr;

    switch (GetMemType(addr, 0)) {
        case MemType_FCRAM:
            return p - OS_FCRAM_VADDR + OS_FCRAM_PADDR;
        case MemType_VRAM:
            return p - OS_VRAM_VADDR + OS_VRAM_PADDR;
#ifdef CTR_ENABLE_QTMRAM
        case MemType_QTMRAM:
            return p - OS_QTMRAM_VADDR + OS_QTMRAM_PADDR;
#endif // CTR_ENABLE_QTMRAM
        default:
            CTR_LOG_LOCATION("Invalid address: 0x%08X", p);
            return 0;
    }
}

void* GetVirtualAddress(uintptr_t addr) {
    if (checkRange(addr, 0, OS_FCRAM_PADDR, OS_FCRAM_SIZE))
        return (void*)(addr - OS_FCRAM_PADDR + OS_FCRAM_VADDR);

    if (checkRange(addr, 0, OS_VRAM_PADDR, OS_VRAM_SIZE))
        return (void*)(addr - OS_VRAM_PADDR + OS_VRAM_VADDR);

#ifdef CTR_ENABLE_QTMRAM
    if (checkRange(addr, 0, OS_QTMRAM_PADDR, OS_QTMRAM_SIZE))
        return (void*)(addr - OS_QTMRAM_PADDR + OS_QTMRAM_VADDR);
#endif // CTR_ENABLE_QTMRAM

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return 0;
}

static Result queryRegionAccess(u32 base, size_t size, uint32_t* access) {
    MemInfo memInfo;
    PageInfo pageInfo;
    
    Result ret = svcQueryMemory(&memInfo, &pageInfo, base);
    if (R_FAILED(ret))
        return ret;

    while (memInfo.size < size) {
        MemInfo tmp;
        ret = svcQueryMemory(&tmp, &pageInfo, memInfo.base_addr + memInfo.size);
        if (R_FAILED(ret))
            return ret;

        if (tmp.state != memInfo.state || tmp.perm != memInfo.perm)
            break;

        memInfo.size += tmp.size;
    }

    *access = 0;
        
    if (memInfo.perm & MEMPERM_READ)
        *access |= MemAccess_Read;

    if (memInfo.perm & MEMPERM_WRITE)
        *access |= MemAccess_Write;

    return 0;
}

uint32_t GetCPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // These are fixed.
    const uint32_t heapAccess = MemAccess_Read | MemAccess_Write;
    const uint32_t fcramAccess = MemAccess_Read | MemAccess_Write;

    if (checkRange(addr, size, __ctru_heap, __ctru_heap_size))
        return heapAccess;

    if (checkRange(addr, size, __ctru_linear_heap, __ctru_linear_heap_size))
        return fcramAccess;

    // VRAM access depends on the ExHeader.
    if (checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE)) {
        static uint32_t vramAccess = 0xFFFFFFFF;

        if (vramAccess == 0xFFFFFFFF) {
            uint32_t tmp;
            CTR_BREAK_IF(R_FAILED(queryRegionAccess(OS_VRAM_VADDR, OS_VRAM_SIZE, &tmp)));

            do {
                __ldrex((s32*)&vramAccess);
            } while (__strex((s32*)&vramAccess, tmp));
        }

        return vramAccess;
    }

#ifdef CTR_ENABLE_QTMRAM
    uintptr_t qtmramBase = 0;
    size_t qtmramSize = 0;
    qtmramQueryRegion(&qtmramBase, &qtmramSize);

    // QTMRAM access depends on the ExHeader.
    if (checkRange(addr, size, qtmramBase, qtmramSize)) {
        static uint32_t qtmramAccess = 0xFFFFFFFF;

        if (qtmramAccess == 0xFFFFFFFF) {
            uint32_t tmp;
            CTR_BREAK_IF(R_FAILED(queryRegionAccess(qtmramBase, qtmramSize, &tmp)));

            do {
                __ldrex((s32*)&qtmramAccess);
            } while (__strex((s32*)&qtmramAccess, tmp));
        }

        return qtmramAccess;
    }
#endif // CTR_ENABLE_QTMRAM

    // Handle other memory.
    uint32_t otherAccess = 0;
    queryRegionAccess(addr, size, &otherAccess);
    return otherAccess;
}

uint32_t GetGPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // If it's accessible GPU has RW access.
    const uint32_t sharedAccess = MemAccess_Read | MemAccess_Write;

    bool b = checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE) ||
        checkRange(addr, size, __ctru_linear_heap, __ctru_linear_heap_size);

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

void InvalidateDataCache(const void* addr, size_t size) {
    CTR_BREAK_IF(R_FAILED(svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
}

void FlushDataCache(const void* addr, size_t size) {
    CTR_BREAK_IF(R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
}

// Sync

void Yield(void) { svcSleepThread(0); }

Mutex CreateMutex(void) {
    LightLock* l = AllocMem(MemType_AppHeap, sizeof(LightLock));
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
    CondVar* cv = AllocMem(MemType_AppHeap, sizeof(CondVar));
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

// Tick

void TickTimerStart(TickTimer* t) {
    CTR_ASSERT(t);
    *t = svcGetSystemTick();
}

uint64_t TickTimerStop(TickTimer* t) {
    CTR_ASSERT(t);

    const uint64_t invalid = (uint64_t)-1;
    if (*t != invalid) {
        const uint64_t delta = svcGetSystemTick() - *t;
        *t = invalid;
        return delta;
    }

    return 0;
}

uint64_t TickTimerInterval(TickTimer* t) {
    CTR_ASSERT(t);
    const uint64_t newp = svcGetSystemTick();
    const uint64_t delta = newp - *t;
    *t = newp;
    return delta;
}