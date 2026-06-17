/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_CACHE_H
#define GUARD_CTR11_CACHE_H

#include <CTR11/Defs.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void InvalidateDataCache(const void* addr, size_t size);
void FlushDataCache(const void* addr, size_t size);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_CACHE_H */