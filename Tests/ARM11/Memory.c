/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Memory.h>

#include "Common.h"

DEFINE_TEST(RangeCheck) {
    const size_t allocSize = 8;

    void* p = AllocTypedMem(allocSize, MemType_AppHeap);
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

    p = AllocTypedMem(allocSize, MemType_FCRAM);
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

    p = AllocTypedMem(allocSize, MemType_VRAM_A);
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

    p = AllocTypedMem(allocSize, MemType_VRAM_B);
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

    p = AllocTypedMem(allocSize, MemType_QTMRAM);
    // QTMRAM might not be available.
    if (p) {
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
    }

    return true;
}

DEFINE_TEST(CPUAccess) {
    const uint32_t cpuMask = MemAccess_CPURead | MemAccess_CPUWrite;
    const size_t allocSize = 8;

    // CPU has read only access on .text.
    extern uint32_t memoryTestTextVar;
    FAIL_IF((GetMemAccess(&memoryTestTextVar, sizeof(uint32_t)) & cpuMask) != MemAccess_CPURead);

    // CPU has read only access on .rodata.
    extern uint32_t memoryTestRodataVar;
    FAIL_IF((GetMemAccess(&memoryTestRodataVar, sizeof(uint32_t)) & cpuMask) != MemAccess_CPURead);

    // CPU has RW access on .data.
    extern uint32_t memoryTestDataVar;
    FAIL_IF((GetMemAccess(&memoryTestDataVar, sizeof(uint32_t)) & cpuMask) != (MemAccess_CPURead | MemAccess_CPUWrite));

    // CPU has RW access on stack.
    FAIL_IF((GetMemAccess(&reason, sizeof(uint32_t*)) & cpuMask) != (MemAccess_CPURead | MemAccess_CPUWrite));

    // CPU has RW access on app heap.
    void* p = AllocTypedMem(allocSize, MemType_AppHeap);
    FAIL_IF(!p);    
    FAIL_IF((GetMemAccess(p, allocSize) & cpuMask) != (MemAccess_CPURead | MemAccess_CPUWrite));
    FreeMem(p);

    // CPU has RW access on FCRAM.
    p = AllocTypedMem(allocSize, MemType_FCRAM);
    FAIL_IF(!p);
    FAIL_IF((GetMemAccess(p, allocSize) & cpuMask) != (MemAccess_CPURead | MemAccess_CPUWrite));
    FreeMem(p);

    return true;
}

DEFINE_TEST(GPUAccess) {
    const uint32_t gpuMask = MemAccess_GPURead | MemAccess_GPUWrite;
    const size_t allocSize = 8;

    // GPU has no access on app heap.
    void* p = AllocTypedMem(allocSize, MemType_AppHeap);
    FAIL_IF(!p);
    FAIL_IF(GetMemAccess(p, allocSize) & gpuMask);
    FreeMem(p);

    // GPU has RW access on FCRAM.
    p = AllocTypedMem(allocSize, MemType_FCRAM);
    FAIL_IF(!p);
    FAIL_IF((GetMemAccess(p, allocSize) & gpuMask) != (MemAccess_GPURead | MemAccess_GPUWrite));
    FreeMem(p);

    // GPU has RW access on VRAM.
    p = AllocTypedMem(allocSize, MemType_VRAM);
    FAIL_IF(!p);
    FAIL_IF((GetMemAccess(p, allocSize) & gpuMask) != (MemAccess_GPURead | MemAccess_GPUWrite));
    FreeMem(p);

    // GPU has RW access on QTMRAM.
    p = AllocTypedMem(allocSize, MemType_QTMRAM);
    // QTMRAM might not be available.
    if (p) {
        FAIL_IF((GetMemAccess(p, allocSize) & gpuMask) != (MemAccess_GPURead | MemAccess_GPUWrite));
        FreeMem(p);
    }

    return true;
}

DEFINE_TEST(AccessFlags) {
    const size_t allocSize = 8;

    void* p = AllocMem(allocSize, 0);
    FAIL_IF(p);

    p = AllocMem(allocSize, MemAccess_CPURead);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & MemAccess_CPURead));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_CPUWrite);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & MemAccess_CPUWrite));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_CPURead | MemAccess_CPUWrite);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & (MemAccess_CPURead | MemAccess_CPUWrite)));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_GPURead);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & MemAccess_GPURead));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_GPUWrite);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & MemAccess_GPURead));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_GPURead | MemAccess_GPUWrite);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & (MemAccess_GPURead | MemAccess_GPUWrite)));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_CPURead | MemAccess_GPURead);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & (MemAccess_CPURead | MemAccess_GPURead)));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_CPURead | MemAccess_CPUWrite | MemAccess_GPURead);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & (MemAccess_CPURead | MemAccess_CPUWrite | MemAccess_GPURead)));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_CPURead | MemAccess_GPURead | MemAccess_GPUWrite);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & (MemAccess_CPURead | MemAccess_GPURead | MemAccess_GPUWrite)));
    FreeMem(p);

    p = AllocMem(allocSize, MemAccess_CPURead | MemAccess_CPUWrite | MemAccess_GPURead | MemAccess_GPUWrite);
    FAIL_IF(!p);
    FAIL_IF(!(GetMemAccess(p, allocSize) & (MemAccess_CPURead | MemAccess_CPUWrite | MemAccess_GPURead | MemAccess_GPUWrite)));
    FreeMem(p);

    return true;
}

DEFINE_TEST(VaToPa) {
    const size_t allocSize = 8;

    // FCRAM.
    void* p = AllocTypedMem(allocSize, MemType_FCRAM);
    FAIL_IF(!p);

    uintptr_t pa = GetPhysicalAddress(p);
    FAIL_IF(!pa);

    void* va = GetVirtualAddress(pa);
    FAIL_IF(va == NULL);
    FAIL_IF(va != p);

    FreeMem(p);

    // VRAM.
    p = AllocTypedMem(allocSize, MemType_VRAM);
    FAIL_IF(!p);

    pa = GetPhysicalAddress(p);
    FAIL_IF(!pa);

    va = GetVirtualAddress(pa);
    FAIL_IF(va == NULL);
    FAIL_IF(va != p);

    FreeMem(p);

    // QTMRAM (might not be available).
    p = AllocTypedMem(allocSize, MemType_QTMRAM);
    if (p) {
        pa = GetPhysicalAddress(p);
        FAIL_IF(!pa);

        va = GetVirtualAddress(pa);
        FAIL_IF(va == NULL);
        FAIL_IF(va != p);

        FreeMem(p);
    }

    return true;
}