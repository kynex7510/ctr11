/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <arm.h>
#include <types.h>
#include <mem_map.h>
#include <arm11/allocator/fcram.h>
#include <arm11/allocator/vram.h>
#include <arm11/drivers/cfg11.h>
#else
#include <3ds.h>
#endif // CTR_BM

#include <CTR11/Memory.h>
#include <CTR11/Log.h>
#include <CTR11/Break.h>

#include <malloc.h>

#include "QTMRAM.h"

#ifdef CTR_BM
// fake_heap_start is not set, heap starts after .bss.
extern char __bss_end__;
extern char* fake_heap_end;
#else
extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap;
extern u32 __ctru_heap_size;
extern u32 __ctru_linear_heap;
extern u32 __ctru_linear_heap_size;
#endif // CTR_BM

void* AllocMemAligned(uint32_t memType, size_t size, size_t alignment) {
    if (!size)
        return NULL;

    if (!alignment)
        alignment = 1;

    if (memType == MemType_AppHeap)
        return memalign(alignment, size);

    if (memType == MemType_FCRAM) {
#ifdef CTR_BM
        return fcramMemAlign(size, alignment);
#else
        return linearMemAlign(size, alignment);
#endif // CTR_BM
    }

    // Let the allocator decide.
    if (memType == (MemType_VRAM_A | MemType_VRAM_B)) {
        return vramMemAlign(size, alignment);
    } else if (memType == MemType_VRAM_A) {
        return vramMemAlignAt(size, alignment, VRAM_ALLOC_A);
    } else if (memType == MemType_VRAM_B) {
        return vramMemAlignAt(size, alignment, VRAM_ALLOC_B);
    }

    if (memType == MemType_QTMRAM)
        return qtmramMemAlign(size, alignment);

    return NULL;
}

void* AllocAnyMemAligned(const uint32_t* memTypes, size_t numTypes, size_t size, size_t alignment) {
    uint32_t mask = 0;

    for (size_t i = 0; i < numTypes; ++i) {
        const uint32_t memType = memTypes[i];

        if ((mask & memType) == memType)
            continue;

        void* p = AllocMemAligned(memType, size, alignment);
        if (p)
            return p;

        mask |= memType;
    }

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
#ifdef CTR_BM
            fcramFree(p);
#else
            linearFree(p);
#endif // CTR_BM
            break;
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            vramFree(p);
            break;
        case MemType_QTMRAM:
            qtmramFree(p);
            break;
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

#ifdef CTR_BM
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
#else
    if (checkRange(addr, size, (u32)fake_heap_start, fake_heap_end - fake_heap_start))
        return MemType_AppHeap;

    if (checkRange(addr, size, __ctru_linear_heap, __ctru_linear_heap_size))
        return MemType_FCRAM;

    if (checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE / 2))
        return MemType_VRAM_A;

    if (checkRange(addr, size, OS_VRAM_VADDR + (OS_VRAM_SIZE / 2), OS_VRAM_SIZE / 2))
        return MemType_VRAM_B;

    if (checkRange(addr, size, OS_QTMRAM_VADDR, OS_QTMRAM_SIZE))
        return MemType_QTMRAM;
#endif // CTR_BM

    CTR_LOG_LOCATION("Invalid address: 0x%08X", addr);
    return MemType_Unknown;
}

size_t GetAllocSize(const void* p) {
    switch (GetMemType(p, 0)) {
        case MemType_AppHeap:
            return malloc_usable_size((void*)p);
        case MemType_FCRAM:
#ifdef CTR_BM
            return fcramGetSize((void*)p);
#else
            return linearGetSize((void*)p);
#endif // CTR_BM
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            return vramGetSize((void*)p);
        case MemType_QTMRAM:
            return qtmramGetSize(p);
        default:
            CTR_LOG_LOCATION("Invalid memory type");
            return 0;
    }
}

#ifdef CTR_BM

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

    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }

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

    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }

    return b ? sharedAccess : 0;
}

#else

uintptr_t GetPhysicalAddress(const void* addr) {
    const uintptr_t p = (uintptr_t)addr;

    switch (GetMemType(addr, 0)) {
        case MemType_FCRAM:
            return p - OS_FCRAM_VADDR + OS_FCRAM_PADDR;
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            return p - OS_VRAM_VADDR + OS_VRAM_PADDR;
        case MemType_QTMRAM:
            return p - OS_QTMRAM_VADDR + OS_QTMRAM_PADDR;
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

    if (checkRange(addr, 0, OS_QTMRAM_PADDR, OS_QTMRAM_SIZE))
        return (void*)(addr - OS_QTMRAM_PADDR + OS_QTMRAM_VADDR);

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

    if (memInfo.size >= size) {
        if (memInfo.perm & MEMPERM_READ)
            *access |= MemAccess_Read;

        if (memInfo.perm & MEMPERM_WRITE)
            *access |= MemAccess_Write;
    }

    return 0;
}

uint32_t GetCPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // These are fixed.
    const uint32_t heapAccess = MemAccess_Read | MemAccess_Write;
    const uint32_t fcramAccess = MemAccess_Read | MemAccess_Write;

    // Check for libctru heap instead of fake heap because stack is mapped in the 
    // heap area for legacy reasons, hence it's more efficient for stack addresses.
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

    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }

    return b ? sharedAccess : 0;
}

#endif // CTR_BM