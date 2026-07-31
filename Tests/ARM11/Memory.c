/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Testing.h>
#include <CTR11/Memory.h>

#define FAIL_IF(cond)           \
    do {                        \
        if ((cond)) {           \
            *reason = __LINE__; \
            return false;       \
        }                       \
    } while (0)

static bool rangeChecksTest(uint32_t* reason) {
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

    p = AllocMem(MemType_VRAM, allocSize);
    FAIL_IF(!p);
    FAIL_IF(IsMemAppHeap(p, allocSize));
    FAIL_IF(IsMemAppHeap(p, 0));
    FAIL_IF(IsMemFCRAM(p, allocSize));
    FAIL_IF(IsMemFCRAM(p, 0));
    FAIL_IF(!IsMemVRAM(p, allocSize));
    FAIL_IF(!IsMemVRAM(p, 0));
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
CTR_TEST(RangeChecks, rangeChecksTest);

static bool vramBankTest(uint32_t* reason) {
    void* p = AllocMemVRAM(VRAMBank_A, 8);
    FAIL_IF(!p);
    FAIL_IF(GetVRAMBank(p, 8) != VRAMBank_A);
    FAIL_IF(GetVRAMBank(p, 0) != VRAMBank_A);

    FreeMem(p);

    p = AllocMemVRAM(VRAMBank_B, 8);
    FAIL_IF(!p);
    FAIL_IF(GetVRAMBank(p, 8) != VRAMBank_B);
    FAIL_IF(GetVRAMBank(p, 0) != VRAMBank_B);

    FreeMem(p);
    return true;
}
CTR_TEST(VRAMBank, vramBankTest);

static bool cpuAccessTest(uint32_t* reason) {
    const size_t allocSize = 8;

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
CTR_TEST(CPUAccess, cpuAccessTest);

static bool gpuAccessTest(uint32_t* reason) {
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
    p = AllocMem(MemType_VRAM, allocSize);
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
CTR_TEST(GPUAccess, gpuAccessTest);

static bool vaToPaTest(uint32_t* reason) {
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
    p = AllocMem(MemType_VRAM, allocSize);
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
CTR_TEST(VaToPa, vaToPaTest);