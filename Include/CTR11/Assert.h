/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_ASSERT_H
#define GUARD_CTR11_ASSERT_H

#include <CTR11/Defs.h>
#include <CTR11/Break.h>
#include <CTR11/Log.h>

#ifndef NDEBUG

#define CTR_ASSERT(cond)                                                       \
    do {                                                                       \
        if (!CTR_LIKELY(cond)) {                                               \
            CTR_LOG_LOCATION("Assertion failed: " impl_CTR11_AS_STRING(cond)); \
            impl_CTR11_GLOBAL_NS impl_ctr11_break();                           \
        }                                                                      \
    } while (false)

#else
#define CTR_ASSERT(cond) (void)((cond))
#endif // !NDEBUG

#endif /* GUARD_CTR11_ASSERT_H */