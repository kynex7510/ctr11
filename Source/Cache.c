/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <drivers/cache.h>

void InvalidateDataCache(const void* addr, size_t size) { invalidateDCacheRange(addr, size); }
void FlushDataCache(const void* addr, size_t size) { flushDCacheRange(addr, size); }
#else 
#include <3ds.h>

#include <CTR11/Break.h>

void InvalidateDataCache(const void* addr, size_t size) {
    CTR_BREAK_IF(R_FAILED(svcInvalidateProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
}

void FlushDataCache(const void* addr, size_t size) {
    CTR_BREAK_IF(R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE, (u32)addr, size)));
}
#endif // CTR_BM