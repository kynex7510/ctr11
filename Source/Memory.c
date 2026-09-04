/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// Access functions can take any buffer as input, as such ranges must be explicit (ie. cannot assume it's the same for the allocators).

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
#include <CTR11/Unreachable.h>
#include <CTR11/Align.h>

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

static uint32_t getVRAMCPUAccess(void);
static uint32_t getQTMRAMCPUAccess(void);
#endif // CTR_BM

void* AllocTypedMemAligned(size_t size, size_t alignment, MemType memType) {
    if (!size)
        return NULL;

    if (!alignment)
        alignment = 8;

    switch (memType) {
        case MemType_AppHeap:
            return memalign(alignment, size);
        case MemType_FCRAM:
#ifdef CTR_BM
            return fcramMemAlign(size, alignment);
#else
            return linearMemAlign(size, alignment);
#endif // CTR_BM
        case MemType_VRAM:
            // Let the allocator decide.
            return vramMemAlign(size, alignment);
        case MemType_VRAM_A:
            return vramMemAlignAt(size, alignment, VRAM_ALLOC_A);
        case MemType_VRAM_B:
            return vramMemAlignAt(size, alignment, VRAM_ALLOC_B);
        case MemType_QTMRAM:
            return qtmramMemAlign(size, alignment);
        default:
            CTR_UNREACHABLE("Invalid memory type %u", (uint32_t)memType);
    }
}

void* AllocMemAligned(size_t size, size_t alignment, uint32_t access) {
    const uint32_t cpuMask = MemAccess_CPURead | MemAccess_CPUWrite;
    const uint32_t gpuMask = MemAccess_GPURead | MemAccess_GPUWrite;

    if (!(access & gpuMask)) {
        if (!(access & cpuMask))
            return NULL;

#ifdef CTR_BM
        // Everything is RW in baremetal.
        // Prioritize application memory for CPU usage.
        const MemType memTypes[4] = { MemType_AppHeap, MemType_FCRAM, MemType_QTMRAM, MemType_VRAM };
        return AllocAnyTypeMemAligned(size, alignment, memTypes, sizeof(memTypes) / sizeof(MemType));
#else
        // These two are always RW.
        // Prioritize application memory for CPU only usage.
        const MemType memTypes[2] = { MemType_AppHeap, MemType_FCRAM };
        void* p = AllocAnyTypeMemAligned(size, alignment, memTypes, sizeof(memTypes) / sizeof(MemType));

        // Try QTMRAM.
        if (!p && ((access & getQTMRAMCPUAccess()) == access))
            p = AllocTypedMemAligned(size, alignment, MemType_QTMRAM);

        // Try VRAM.
        if (!p && ((access & getVRAMCPUAccess()) == access))
            p = AllocTypedMemAligned(size, alignment, MemType_VRAM);

        return p;
#endif // CTR_BM
    }

    // The following operations all involve the GPU, hence ensure addresses are 8 bytes aligned.
    if (alignment) {
        alignment = AlignUp(alignment, 8);
    } else {
        alignment = 8;
    }

    if (!(access & cpuMask)) {
        if (!(access & gpuMask))
            return NULL;

        // Everything is RW for the GPU.
        // Prioritize FCRAM because it's the largest.
        const MemType memTypes[3] = { MemType_FCRAM, MemType_VRAM, MemType_QTMRAM };
        return AllocAnyTypeMemAligned(size, alignment, memTypes, sizeof(memTypes) / sizeof(MemType));
    }

#ifdef CTR_BM
    // Everything is RW in baremetal.
    // Prioritize FCRAM because it's the largest.
    const MemType memTypes[3] = { MemType_FCRAM, MemType_VRAM, MemType_QTMRAM };
    return AllocAnyTypeMemAligned(size, alignment, memTypes, sizeof(memTypes) / sizeof(MemType));
#else
    // FCRAM is always available as RW for both CPU and GPU.
    void* p = AllocTypedMemAligned(size, alignment, MemType_FCRAM);

    // QTMRAM CPU access depends on the ExHeader.
    if (!p && ((access & getQTMRAMCPUAccess()) == access))
        p = AllocTypedMemAligned(size, alignment, MemType_QTMRAM);

    // VRAM CPU access depends on the ExHeader.
    if (!p && ((access & getVRAMCPUAccess()) == access))
        p = AllocTypedMemAligned(size, alignment, MemType_VRAM);
    
    return p;
#endif // CTR_BM
}

static uint32_t getMemTypeMask(MemType memType) {
    switch (memType) {
        case MemType_AppHeap:
            return 0x01;
        case MemType_FCRAM:
            return 0x02;
        case MemType_VRAM_A:
            return 0x04;
        case MemType_VRAM_B:
            return 0x08;
        case MemType_VRAM:
            return 0x04 | 0x08;
        case MemType_QTMRAM:
            return 0x10;
        default:
            CTR_UNREACHABLE("Invalid memory type %u", (uint32_t)memType);
    }
}

void* AllocAnyTypeMemAligned(size_t size, size_t alignment, const MemType* memTypes, size_t numTypes) {
    uint32_t mask = 0;

    for (size_t i = 0; i < numTypes; ++i) {
        const MemType memType = memTypes[i];
        const uint32_t thisMask = getMemTypeMask(memType);

        if ((mask & thisMask) == thisMask)
            continue;

        void* p = AllocTypedMemAligned(size, alignment, memType);
        if (p)
            return p;

        mask |= thisMask;
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
            CTR_UNREACHABLE("This shouldn't be happening, maybe some case is missing?");
    }
}

static inline bool checkRange(u32 p, size_t size, u32 rangeBase, size_t rangeSize) {
    const size_t offset = p - rangeBase;
    return p >= rangeBase && offset < rangeSize && (rangeSize - offset) > size;
}

MemType GetMemType(const void* p, size_t size) {
    const u32 addr = (u32)p;

    if (checkRange(addr, size, (u32)GetMemRegionBase(MemType_AppHeap), GetMemRegionSize(MemType_AppHeap)))
        return MemType_AppHeap;

    if (checkRange(addr, size, (u32)GetMemRegionBase(MemType_FCRAM), GetMemRegionSize(MemType_FCRAM)))
        return MemType_FCRAM;

    if (checkRange(addr, size, (u32)GetMemRegionBase(MemType_VRAM_A), GetMemRegionSize(MemType_VRAM_A)))
        return MemType_VRAM_A;

    if (checkRange(addr, size, (u32)GetMemRegionBase(MemType_VRAM_B), GetMemRegionSize(MemType_VRAM_B)))
        return MemType_VRAM_B;

    if (checkRange(addr, size, (u32)GetMemRegionBase(MemType_QTMRAM), GetMemRegionSize(MemType_QTMRAM)))
        return MemType_QTMRAM;

    CTR_UNREACHABLE("Invalid address: 0x%08X", addr);
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
            CTR_UNREACHABLE("This shouldn't be happening, maybe some case is missing?");
    }
}

#ifdef CTR_BM

static inline bool isNew3DS(void) { return getCfg11Regs()->socinfo & SOCINFO_LGR2; }

void* GetMemRegionBase(MemType memType) {
    uintptr_t qtmramBase = 0;

    switch (memType) {
        case MemType_AppHeap:
            return &__bss_end__;
        case MemType_FCRAM:
            return (void*)FCRAM_BASE;
        case MemType_VRAM:
            return (void*)VRAM_BASE;
        case MemType_VRAM_A:
            return (void*)VRAM_BANK0;
        case MemType_VRAM_B:
            return (void*)VRAM_BANK1;
        case MemType_QTMRAM:
            qtmramQueryRegion(&qtmramBase, NULL);
            return (void*)qtmramBase;
        default:
            CTR_UNREACHABLE("Invalid memory type %u", (uint32_t)memType);
    }
}

size_t GetMemRegionSize(MemType memType) {
    size_t qtmramSize = 0;

    switch (memType) {
        case MemType_AppHeap:
            return fake_heap_end - &__bss_end__;
        case MemType_FCRAM:
            return isNew3DS() ? (FCRAM_SIZE + FCRAM_EXT_SIZE) : FCRAM_SIZE;
        case MemType_VRAM:
            return VRAM_SIZE;
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            return VRAM_BANK_SIZE;
        case MemType_QTMRAM:
            qtmramQueryRegion(NULL, &qtmramSize);
            return qtmramSize;
        default:
            CTR_UNREACHABLE("Invalid memory type %u", (uint32_t)memType);
    }
}

uintptr_t GetPhysicalAddress(const void* addr) { return (uintptr_t)addr; }
void* GetVirtualAddress(uintptr_t addr) { return (void*)addr; }

static uint32_t queryCPUMemAccess(u32 addr) {
    // Walk the translation tables.
    const u32* l1Table = (const u32*)(__getTtbr0() & 0xFFFFF000);
    const u32 l1Entry = l1Table[addr >> 20];

    // Check L2 entries.
    if ((l1Entry & 0x03) == 0x01) {
        const u32* l2Table = (const u32*)(l1Entry & 0xFFFFFC00);
        const u32 l2Entry = l2Table[(addr >> 12) & 0xFF];
        const u32 perm = (l2Entry >> 4) & 0x3F;
        if (perm)
            return (perm >> 5) ? MemAccess_CPURead : (MemAccess_CPURead | MemAccess_CPUWrite);

        return 0;
    }

    // Check sections/supersections.
    if ((l1Entry & 0x03) == 0x02) {
        // Always RW if mapped.
        // TODO: implement generic logic.
        return ((l1Entry >> 10) & 0x3F) ? (MemAccess_CPURead | MemAccess_CPUWrite) : 0;
    }

    return 0;
}

static uint32_t queryCPUAccess(u32 addr, size_t size) {
    const uint32_t access = queryCPUMemAccess(addr);

    if (!access)
        return 0;

    for (size_t i = 0x1000; i < size; i += 0x1000) {
        if (queryCPUMemAccess(addr + i) != access)
            return 0;
    }

    return access;
}

uint32_t GetCPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // These are all RW.
    const uint32_t sharedAccess = MemAccess_CPURead | MemAccess_CPUWrite;
    const size_t fcramSize = isNew3DS() ? (FCRAM_SIZE + FCRAM_EXT_SIZE) : FCRAM_SIZE;

    bool b = checkRange(addr, size, (u32)&__bss_end__, fake_heap_end - &__bss_end__) ||
        checkRange(addr, size, FCRAM_BASE, fcramSize) ||
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
    const uint32_t sharedAccess = MemAccess_GPURead | MemAccess_GPUWrite;

    const u16 gpuprot = getCfg11Regs()->gpuprot;

    const u8 fcramFactor = gpuprot & 0x0F;
    const u8 fcramExtFactor = (gpuprot >> 4) & 0x0F;
    const bool axiwramCutoff = (gpuprot >> 8) & 0x01;

    const size_t fcramSize = FCRAM_SIZE - (fcramFactor * 0x800000);
    const size_t fcramExtSize = FCRAM_EXT_SIZE - (fcramExtFactor * 0x800000);

    bool b = axiwramCutoff && checkRange(addr, size, AXI_RAM_BASE, AXI_RAM_SIZE);

    if (!b)
        b = checkRange(addr, size, FCRAM_BASE, fcramSize) || checkRange(addr, size, VRAM_BASE, VRAM_SIZE);

    // Check ext fcram only for new systems.
    if (!b && isNew3DS())
        b = checkRange(addr, size, FCRAM_EXT_BASE, fcramExtSize);

    if (!b) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);
        b = checkRange(addr, size, qtmramBase, qtmramSize);
    }

    return b ? sharedAccess : 0;
}

#else

void* GetMemRegionBase(MemType memType) {
    uintptr_t qtmramBase = 0;

    switch (memType) {
        case MemType_AppHeap:
            return fake_heap_start;
        case MemType_FCRAM:
            return (void*)__ctru_linear_heap;
        case MemType_VRAM:
        case MemType_VRAM_A:
            return (void*)OS_VRAM_VADDR;
        case MemType_VRAM_B:
            return (void*)(OS_VRAM_VADDR + OS_VRAM_SIZE);
        case MemType_QTMRAM:
            qtmramQueryRegion(&qtmramBase, NULL);
            return (void*)qtmramBase;
        default:
            CTR_UNREACHABLE("Invalid memory type %u", (uint32_t)memType);
    }
}

size_t GetMemRegionSize(MemType memType) {
    size_t qtmramSize = 0;

    switch (memType) {
        case MemType_AppHeap:
            return fake_heap_end - fake_heap_start;
        case MemType_FCRAM:
            return __ctru_linear_heap_size;
        case MemType_VRAM:
            return OS_VRAM_SIZE;
        case MemType_VRAM_A:
        case MemType_VRAM_B:
            return OS_VRAM_SIZE / 2;
        case MemType_QTMRAM:
            qtmramQueryRegion(NULL, &qtmramSize);
            return qtmramSize;
        default:
            CTR_UNREACHABLE("Invalid memory type %u", (uint32_t)memType);
    }
}

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

static Result queryCPUAccess(u32 base, size_t size, uint32_t* access) {
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
            *access |= MemAccess_CPURead;

        if (memInfo.perm & MEMPERM_WRITE)
            *access |= MemAccess_CPUWrite;
    }

    return 0;
}

static uint32_t getVRAMCPUAccess(void) {
    static uint32_t vramAccess = 0xFFFFFFFF;

    if (vramAccess == 0xFFFFFFFF) {
        uint32_t tmp;
        CTR_BREAK_IF(R_FAILED(queryCPUAccess(OS_VRAM_VADDR, OS_VRAM_SIZE, &tmp)));

        do {
            __ldrex((s32*)&vramAccess);
        } while (__strex((s32*)&vramAccess, tmp));
    }

    return vramAccess;
}

static uint32_t getQTMRAMCPUAccess(void) {
    static uint32_t qtmramAccess = 0xFFFFFFFF;

    if (qtmramAccess == 0xFFFFFFFF) {
        uintptr_t qtmramBase = 0;
        size_t qtmramSize = 0;
        qtmramQueryRegion(&qtmramBase, &qtmramSize);

        uint32_t tmp;
        CTR_BREAK_IF(R_FAILED(queryCPUAccess(qtmramBase, qtmramSize, &tmp)));

        do {
            __ldrex((s32*)&qtmramAccess);
        } while (__strex((s32*)&qtmramAccess, tmp));
    }

    return qtmramAccess;
}

static uint32_t getCPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // These are fixed.
    const uint32_t heapAccess = MemAccess_CPURead | MemAccess_CPUWrite;
    const uint32_t fcramAccess = MemAccess_CPURead | MemAccess_CPUWrite;

    // Check for libctru heap instead of fake heap because stack is mapped in the 
    // heap area for legacy reasons, hence it's more efficient for stack addresses.
    if (checkRange(addr, size, __ctru_heap, __ctru_heap_size))
        return heapAccess;

    if (checkRange(addr, size, __ctru_linear_heap, __ctru_linear_heap_size))
        return fcramAccess;

    // VRAM access depends on the ExHeader.
    if (checkRange(addr, size, OS_VRAM_VADDR, OS_VRAM_SIZE))
        return getVRAMCPUAccess();

    // QTMRAM access depends on the ExHeader.
    uintptr_t qtmramBase = 0;
    size_t qtmramSize = 0;
    qtmramQueryRegion(&qtmramBase, &qtmramSize);

    if (checkRange(addr, size, qtmramBase, qtmramSize))
        return getQTMRAMCPUAccess();

    // Handle other memory.
    uint32_t otherAccess = 0;
    queryCPUAccess(addr, size, &otherAccess);
    return otherAccess;
}

static uint32_t getGPUAccess(const void* p, size_t size) {
    const u32 addr = (u32)p;

    // If it's accessible GPU has RW access.
    const uint32_t sharedAccess = MemAccess_GPURead | MemAccess_GPUWrite;

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

uint32_t GetMemAccess(const void* p, size_t size) {
    return getCPUAccess(p, size) | getGPUAccess(p, size);
}