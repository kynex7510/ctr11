/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Memory.h>

void* AllocMemOrderedAligned(uint32_t* memTypes, size_t numTypes, size_t size, size_t alignment) {
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