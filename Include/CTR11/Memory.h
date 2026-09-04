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
    MemType_VRAM_A,
    MemType_VRAM_B,
    MemType_QTMRAM,
} MemType;

typedef enum {
    MemAccess_CPURead = 0x01,
    MemAccess_CPUWrite = 0x02,
    MemAccess_GPURead = 0x04,
    MemAccess_GPUWrite = 0x08,
} MemAccess;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void* AllocMemAligned(size_t size, size_t alignment, uint32_t access);
void* AllocTypedMemAligned(size_t size, size_t alignment, MemType memType);
void* AllocAnyTypeMemAligned(size_t size, size_t alignment, const MemType* memTypes, size_t numTypes);

CTR_INLINE void* AllocMem(size_t size, uint32_t access) { return AllocMemAligned(size, 0, access); }
CTR_INLINE void* AllocTypedMem(size_t size, MemType memType) { return AllocTypedMemAligned(size, 0, memType); }
CTR_INLINE void* AllocAnyTypeMem(size_t size, const MemType* memTypes, size_t numTypes) { return AllocAnyTypeMemAligned(size, 0, memTypes, numTypes); }

void FreeMem(void* p);

void* GetMemRegionBase(MemType memType);
size_t GetMemRegionSize(MemType memType);
MemType GetMemType(const void* p, size_t size);
size_t GetAllocSize(const void* p);

uintptr_t GetPhysicalAddress(const void* addr);
void* GetVirtualAddress(uintptr_t addr);

// Can take in any kind of pointer.
uint32_t GetMemAccess(const void* p, size_t size);

CTR_INLINE bool IsAccessible(const void* p, size_t size, uint32_t access) {
    return (GetMemAccess(p, size) & access) == access;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_MEMORY_H */