/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR_LOG_H
#define GUARD_CTR_LOG_H

#include <CTR/Defs.h>

#define CTR_LOG(...) impl_CTR_GLOBAL_NS impl_ctr_log(__VA_ARGS__)

#ifndef NDEBUG
#define CTR_LOG_DEBUG(...) CTR_LOG(__VA_ARGS__)
#else
#define CTR_LOG_DEBUG(...)
#endif // !NDEBUG

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void impl_ctr_log(const char* fmt, ...);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR_LOG_H */