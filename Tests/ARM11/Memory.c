/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Memory.h>

#include "Common.h"

DEFINE_TEST(RangeCheck) {
    const size_t allocSize = 8;

    void* p = AllocMem(MemType_AppHeap, allocSize);
    FAIL_IF(!p);
    FAIL_IF(!IsMemAppHeap(p, allocSize));
    FAIL_IF(!IsMemAppHeap(p, 0));
    FAIL_IF(IsMemFCRAM(p, allocSize));
    FAIL_IF(IsMemFCRAM(p, 0));
    FAIL_IF(IsMemVRAM(p, allocSize));
    FAIL_IF(IsMemVRAM(p, 0));
    FAIL_IF(IsMemQTMRAM(p, allocSize));
    FAIL_IF(IsMemQTMRAM(p, 0));
    FAIL_IF(GetAllocSize(p) < allocSize);
    FreeMem(p);

    p = AllocMem(MemType_FCRAM, allocSize);
    FAIL_IF(!p);
    FAIL_IF(IsMemAppHeap(p, allocSize));
    FAIL_IF(IsMemAppHeap(p, 0));
    FAIL_IF(!IsMemFCRAM(p, allocSize));
    FAIL_IF(!IsMemFCRAM(p, 0));
    FAIL_IF(IsMemVRAM(p, allocSize));
    FAIL_IF(IsMemVRAM(p, 0));
    FAIL_IF(IsMemQTMRAM(p, allocSize));
    FAIL_IF(IsMemQTMRAM(p, 0));
    FAIL_IF(GetAllocSize(p) < allocSize);
    FreeMem(p);

    p = AllocMem(MemType_VRAM_A, allocSize);
    FAIL_IF(!p);
    FAIL_IF(IsMemAppHeap(p, allocSize));
    FAIL_IF(IsMemAppHeap(p, 0));
    FAIL_IF(IsMemFCRAM(p, allocSize));
    FAIL_IF(IsMemFCRAM(p, 0));
    FAIL_IF(!IsMemVRAMA(p, allocSize));
    FAIL_IF(!IsMemVRAMA(p, 0));
    FAIL_IF(IsMemVRAMB(p, allocSize));
    FAIL_IF(IsMemVRAMB(p, 0));
    FAIL_IF(IsMemQTMRAM(p, allocSize));
    FAIL_IF(IsMemQTMRAM(p, 0));
    FAIL_IF(GetAllocSize(p) < allocSize);
    FreeMem(p);

    p = AllocMem(MemType_VRAM_B, allocSize);
    FAIL_IF(!p);
    FAIL_IF(IsMemAppHeap(p, allocSize));
    FAIL_IF(IsMemAppHeap(p, 0));
    FAIL_IF(IsMemFCRAM(p, allocSize));
    FAIL_IF(IsMemFCRAM(p, 0));
    FAIL_IF(IsMemVRAMA(p, allocSize));
    FAIL_IF(IsMemVRAMA(p, 0));
    FAIL_IF(!IsMemVRAMB(p, allocSize));
    FAIL_IF(!IsMemVRAMB(p, 0));
    FAIL_IF(IsMemQTMRAM(p, allocSize));
    FAIL_IF(IsMemQTMRAM(p, 0));
    FAIL_IF(GetAllocSize(p) < allocSize);
    FreeMem(p);

#ifdef CTR_ENABLE_QTMRAM
    p = AllocMem(MemType_QTMRAM, allocSize);
    FAIL_IF(!p);
    FAIL_IF(IsMemAppHeap(p, allocSize));
    FAIL_IF(IsMemAppHeap(p, 0));
    FAIL_IF(IsMemFCRAM(p, allocSize));
    FAIL_IF(IsMemFCRAM(p, 0));
    FAIL_IF(IsMemVRAM(p, allocSize));
    FAIL_IF(IsMemVRAM(p, 0));
    FAIL_IF(!IsMemQTMRAM(p, allocSize));
    FAIL_IF(!IsMemQTMRAM(p, 0));
    FAIL_IF(GetAllocSize(p) < allocSize);
    FreeMem(p);
#endif // CTR_ENABLE_QTMRAM

    return true;
}

DEFINE_TEST(CPUAccess) {
    const size_t allocSize = 8;

    // CPU has read only access on .text.
    extern uint32_t memoryTestTextVar;
    FAIL_IF(GetCPUAccess(&memoryTestTextVar, sizeof(uint32_t)) != MemAccess_Read);

    // CPU has read only access on .rodata.
    extern uint32_t memoryTestRodataVar;
    FAIL_IF(GetCPUAccess(&memoryTestRodataVar, sizeof(uint32_t)) != MemAccess_Read);

    // CPU has RW access on .data.
    extern uint32_t memoryTestDataVar;
    FAIL_IF(GetCPUAccess(&memoryTestDataVar, sizeof(uint32_t)) != (MemAccess_Read | MemAccess_Write));

    // CPU has RW access on stack.
    FAIL_IF(GetCPUAccess(&reason, sizeof(uint32_t*)) != (MemAccess_Read | MemAccess_Write));

    // CPU has RW access on app heap.
    void* p = AllocMem(MemType_AppHeap, allocSize);
    FAIL_IF(!p);    
    FAIL_IF(GetCPUAccess(p, allocSize) != (MemAccess_Read | MemAccess_Write));
    FreeMem(p);

    // CPU has RW access on FCRAM.
    p = AllocMem(MemType_FCRAM, allocSize);
    FAIL_IF(!p);
    FAIL_IF(GetCPUAccess(p, allocSize) != (MemAccess_Read | MemAccess_Write));
    FreeMem(p);

    return true;
}

DEFINE_TEST(GPUAccess) {
    const size_t allocSize = 8;

    // GPU has no access on app heap.
    void* p = AllocMem(MemType_AppHeap, allocSize);
    FAIL_IF(!p);
    FAIL_IF(GetGPUAccess(p, allocSize));
    FreeMem(p);

    // GPU has RW access on FCRAM.
    p = AllocMem(MemType_FCRAM, allocSize);
    FAIL_IF(!p);
    FAIL_IF(GetGPUAccess(p, allocSize) != (MemAccess_Read | MemAccess_Write));
    FreeMem(p);

    // GPU has RW access on VRAM.
    p = AllocMem(MemType_VRAM_A | MemType_VRAM_B, allocSize);
    FAIL_IF(!p);
    FAIL_IF(GetGPUAccess(p, allocSize) != (MemAccess_Read | MemAccess_Write));
    FreeMem(p);

#ifdef CTR_ENABLE_QTMRAM
    // GPU has RW access on QTMRAM.
    p = AllocMem(MemType_QTMRAM, allocSize);
    FAIL_IF(!p);
    FAIL_IF(GetGPUAccess(p, allocSize) != (MemAccess_Read | MemAccess_Write));
    FreeMem(p);
#endif // CTR_ENABLE_QTMRAM

    return true;
}

DEFINE_TEST(VaToPa) {
    const size_t allocSize = 8;

    // FCRAM
    void* p = AllocMem(MemType_FCRAM, allocSize);
    FAIL_IF(!p);

    uintptr_t pa = GetPhysicalAddress(p);
    FAIL_IF(!pa);

    void* va = GetVirtualAddress(pa);
    FAIL_IF(va == NULL);
    FAIL_IF(va != p);

    FreeMem(p);

    // VRAM
    p = AllocMem(MemType_VRAM_A | MemType_VRAM_B, allocSize);
    FAIL_IF(!p);

    pa = GetPhysicalAddress(p);
    FAIL_IF(!pa);

    va = GetVirtualAddress(pa);
    FAIL_IF(va == NULL);
    FAIL_IF(va != p);

    FreeMem(p);

#ifdef CTR_ENABLE_QTMRAM
    // QTMRAM
    p = AllocMem(MemType_QTMRAM, allocSize);
    FAIL_IF(!p);

    pa = GetPhysicalAddress(p);
    FAIL_IF(!pa);

    va = GetVirtualAddress(pa);
    FAIL_IF(va == NULL);
    FAIL_IF(va != p);

    FreeMem(p);
#endif // CTR_ENABLE_QTMRAM

    return true;
}