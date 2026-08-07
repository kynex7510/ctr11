/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_MEMORY_H
#define GUARD_CTR11_MEMORY_H

#include <CTR11/Defs.h>

typedef enum {
    MemType_AppHeap = 0x01,
    MemType_FCRAM = 0x02,
    MemType_VRAM_A = 0x04,
    MemType_VRAM_B = 0x08,
    MemType_QTMRAM = 0x10,
} MemType;

typedef enum {
    MemAccess_Read = 0x01,
    MemAccess_Write = 0x02,
} MemAccess;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void* AllocMemAligned(uint32_t memTypes, size_t size, size_t alignment);
void* AllocMemOrderedAligned(const uint32_t* memTypes, size_t numTypes, size_t size, size_t alignment);

CTR_INLINE void* AllocMem(uint32_t memTypes, size_t size) { return AllocMemAligned(memTypes, size, 0); }
CTR_INLINE void* AllocMemOrdered(const uint32_t* memTypes, size_t numTypes, size_t size) { return AllocMemOrderedAligned(memTypes, numTypes, size, 0); }

void FreeMem(void* p);

uint32_t GetMemType(const void* p, size_t size);
size_t GetAllocSize(const void* p);

CTR_INLINE bool IsMemAppHeap(const void* p, size_t size) { return GetMemType(p, size) == MemType_AppHeap; }
CTR_INLINE bool IsMemFCRAM(const void* p, size_t size) { return GetMemType(p, size) == MemType_FCRAM; }

CTR_INLINE bool IsMemVRAM(const void* p, size_t size) {
    const MemType memType = GetMemType(p, size);
    return memType == MemType_VRAM_A || memType == MemType_VRAM_B;
}

CTR_INLINE bool IsMemVRAMA(const void* p, size_t size) { return GetMemType(p, size) == MemType_VRAM_A; }
CTR_INLINE bool IsMemVRAMB(const void* p, size_t size) { return GetMemType(p, size) == MemType_VRAM_B; }
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