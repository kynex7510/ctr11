/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR_ALIGN_H
#define GUARD_CTR_ALIGN_H

#include <CTR/Assert.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

CTR_INLINE bool ctrIsPo2(uint32_t v) { return !(v & (v - 1)); }

CTR_INLINE bool ctrIsAligned(uint32_t v, uint32_t alignment) {
    CTR_ASSERT(ctrIsPo2(alignment));
    return !(v & (alignment - 1));
}

CTR_INLINE uint32_t ctrAlignDown(uint32_t v, uint32_t alignment) {
    CTR_ASSERT(ctrIsPo2(alignment));
    return v & ~(alignment - 1);
}

CTR_INLINE uint32_t kygxAlignUp(uint32_t v, uint32_t alignment) {
    CTR_ASSERT(ctrIsPo2(alignment));
    return (v + (alignment - 1)) & ~(alignment - 1);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR_ALIGN_H */