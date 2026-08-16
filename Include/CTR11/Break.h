/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_BREAK_H
#define GUARD_CTR11_BREAK_H

#include <CTR11/Defs.h>
#include <CTR11/Log.h>

#define CTR_BREAK()                                  \
    do {                                             \
        CTR_LOG_LOCATION("Program broke execution"); \
        impl_CTR11_GLOBAL_NS impl_ctr11_break();     \
    } while (false)

#define CTR_BREAK_IF(cond)                                                            \
    do {                                                                              \
        if (CTR_UNLIKELY(cond)) {                                                     \
            CTR_LOG_LOCATION("Program broke execution: " impl_CTR11_AS_STRING(cond)); \
            impl_CTR11_GLOBAL_NS impl_ctr11_break();                                  \
        }                                                                             \
    } while (false)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

__attribute__((noreturn, cold)) void impl_ctr11_break(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_BREAK_H */