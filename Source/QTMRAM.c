/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

// TODO: consider using a truly platform-agnostic data structure.
#ifdef CTR_BM
#include <types.h>
#include <arm11/drivers/cfg11.h>
#include <arm11/util/rbtree.h>
#else
#include <3ds.h>
#include <3ds/util/rbtree.h>
#endif // CTR_BM

#include <CTR11/Memory.h>
#include <CTR11/Align.h>
#include <CTR11/Unreachable.h>

#include "QTMRAM.h"

typedef struct {
    rbtree_node_t node;
    uintptr_t base;
    size_t size;
} MemoryBlock;

static rbtree_t g_Tree;
static bool g_Initialized = false;
static uintptr_t g_AllocBase = 0;
static size_t g_MaxAllocSize = 0;

static int blockComparator(const rbtree_node_t* lhs, const rbtree_node_t* rhs) {
    uintptr_t a = ((MemoryBlock*)lhs)->base;
    uintptr_t b = ((MemoryBlock*)rhs)->base;

    if (a < b)
        return -1;

    if (a > b)
        return 1;

    return 0;
}

#ifdef CTR_BM

static bool initRegion(void) {
#ifdef HAS_QTMRAM
    const u16 gpuprot = getCfg11Regs()->gpuprot;
    const u8 qtmramFactor = (gpuprot >> 9) & 0x03;
    const size_t qtmramSize = QTM_RAM_SIZE - (qtmramFactor * 0x100000);
    
    g_AllocBase = QTM_RAM_BASE;
    g_MaxAllocSize = qtmramSize;
    return true;
#else
    return false;
#endif // HAS_QTMRAM
}

#else

static bool initRegion(void) {
    s64 base = 0;
    s64 size = 0;

    if (R_SUCCEEDED(svcGetProcessInfo(&base, CUR_PROCESS_HANDLE, 22))) {
        if (R_SUCCEEDED(svcGetProcessInfo(&size, CUR_PROCESS_HANDLE, 23))) {
            g_AllocBase = base;
            g_MaxAllocSize = size;
        }
    }

    return base && size;
}

#endif // CTR_BM

static bool lazyInit(void) {
    if (!g_Initialized) {
        if (!initRegion())
            return false;

        rbtree_init(&g_Tree, blockComparator);
        g_Initialized = true;
    }

    return true;
}

static void* insertNode(uintptr_t base, size_t size) {
    MemoryBlock* b = (MemoryBlock*)AllocMem(MemType_AppHeap, sizeof(MemoryBlock));
    if (b) {
        b->base = base;
        b->size = size;
        if (rbtree_insert(&g_Tree, &b->node));
        return (void*)b->base;
    }

    return NULL;
}

void qtmramQueryRegion(uintptr_t* regionBase, size_t* regionSize) {
    if (regionBase)
        *regionBase = g_AllocBase;

    if (regionSize)
        *regionSize = g_MaxAllocSize;
}

void* qtmramMemAlign(size_t size, size_t alignment) {
    if (!lazyInit())
        return NULL;

    if (alignment < 8)
        alignment = 8;

    if (!IsPowerOf2(alignment))
        return NULL;

    // Get last memory block.
    MemoryBlock* last = (MemoryBlock*)rbtree_max(&g_Tree);
    if (!last) {
        // Insert if we have nothing.
        return insertNode(g_AllocBase, size);
    }

    // If there's space after the last block, use it.
    const uintptr_t lastEndAligned = AlignUp(last->base + last->size, alignment);
    if ((g_AllocBase + g_MaxAllocSize) - lastEndAligned >= size)
        return insertNode(lastEndAligned, size);

    // Look for space between existing memory blocks.
    MemoryBlock* current = last;
    MemoryBlock* prev = (MemoryBlock*)rbtree_node_prev(&current->node);

    while (prev) {
        const uintptr_t prevEndAligned = AlignUp(prev->base + prev->size, alignment);
        if (current->base - prevEndAligned >= size)
            return insertNode(prevEndAligned, size);

        current = prev;
        prev = (MemoryBlock*)rbtree_node_prev(&current->node);
    }

    // If there's space before the first block, use it.
    if (current->base - g_AllocBase >= size)
        return insertNode(g_AllocBase, size);

    return NULL;
}

void qtmramFree(void* p) {
    if (g_Initialized) {
        MemoryBlock b;
        b.base = (uintptr_t)p;

        rbtree_node_t* found = rbtree_find(&g_Tree, &b.node);
        if (found) {
            rbtree_remove(&g_Tree, found, NULL);
            FreeMem(found);
        }
    } else {
        CTR_UNREACHABLE("qtmramFree called without being initialized");
    }
}

size_t qtmramGetSize(const void* p) {
    if (g_Initialized) {
        MemoryBlock b;
        b.base = (uintptr_t)p;

        rbtree_node_t* found = rbtree_find(&g_Tree, &b.node);
        return found ? ((MemoryBlock*)found)->size : 0;
    } else {
        CTR_UNREACHABLE("qtmramGetSize called without being initialized");
    }
}