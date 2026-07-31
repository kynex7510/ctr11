/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_MEMORY_H
#define GUARD_CTR11_MEMORY_H

#include <CTR11/Defs.h>

typedef enum {
    MemType_AppHeap,
    MemType_FCRAM,
    MemType_VRAM,
    MemType_QTMRAM,
    MemType_Unknown,
} MemType;

typedef enum {
    MemAccess_Read = 0x01,
    MemAccess_Write = 0x02,
} MemAccess;

typedef enum {
    VRAMBank_A,
    VRAMBank_B,
    VRAMBank_Any,
    VRAMBank_Unknown,
} VRAMBank;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void* AllocMemAligned(MemType memType, size_t size, size_t alignment);
void* AllocMemAlignedVRAM(VRAMBank bank, size_t size, size_t aligment);

CTR_INLINE void* AllocMem(MemType memType, size_t size) { return AllocMemAligned(memType, size, 0); }
CTR_INLINE void* AllocMemVRAM(VRAMBank bank, size_t size) { return AllocMemAlignedVRAM(bank, size, 0); }

void FreeMem(void* p);

// newSize == 0 frees the buffer. VRAM reallocation doesn't retain content.
void* ReallocMem(void* p, size_t newSize);

MemType GetMemType(const void* p, size_t size);
VRAMBank GetVRAMBank(const void* p, size_t size);
size_t GetAllocSize(const void* p);

CTR_INLINE bool IsMemAppHeap(const void* p, size_t size) { return GetMemType(p, size) == MemType_AppHeap; }
CTR_INLINE bool IsMemFCRAM(const void* p, size_t size) { return GetMemType(p, size) == MemType_FCRAM; }
CTR_INLINE bool IsMemVRAM(const void* p, size_t size) { return GetMemType(p, size) == MemType_VRAM; }
CTR_INLINE bool IsMemQTMRAM(const void* p, size_t size) { return GetMemType(p, size) == MemType_QTMRAM; }

uintptr_t GetPhysicalAddress(const void* addr);
void* GetVirtualAddress(uintptr_t addr);

uint32_t GetCPUAccess(const void* p, size_t size);
uint32_t GetGPUAccess(const void* p, size_t size);

CTR_INLINE bool IsCPUAccessible(const void* p, size_t size, uint32_t access) {
    return (GetCPUAccess(p, size) & access) == access;
}

CTR_INLINE bool IsGPUAccessible(const void* p, size_t size, uint32_t access) {
    return (GetGPUAccess(p, size) & access) == access;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_MEMORY_H */