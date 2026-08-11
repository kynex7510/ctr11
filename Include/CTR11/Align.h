/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_ALIGN_H
#define GUARD_CTR11_ALIGN_H

#include <CTR11/Assert.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE bool IsPowerOf2(uint32_t v) { return v && !(v & (v - 1)); }

CTR_INLINE bool IsAligned(uint32_t v, uint32_t alignment) {
    CTR_ASSERT(IsPowerOf2(alignment));
    return !(v & (alignment - 1));
}

CTR_INLINE uint32_t AlignDown(uint32_t v, uint32_t alignment) {
    CTR_ASSERT(IsPowerOf2(alignment));
    return v & ~(alignment - 1);
}

CTR_INLINE uint32_t AlignUp(uint32_t v, uint32_t alignment) {
    CTR_ASSERT(IsPowerOf2(alignment));
    return (v + (alignment - 1)) & ~(alignment - 1);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_ALIGN_H */