/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_BREAK_H
#define GUARD_CTR11_BREAK_H

#include <CTR11/Defs.h>
#include <CTR11/Log.h>

#define CTR_BREAK() impl_CTR11_GLOBAL_NS impl_ctr11_break()

#define CTR_BREAK_IF(cond)                                                         \
    do {                                                                           \
        if (CTR_UNLIKELY(cond)) {                                                  \
            CTR_LOG_DEBUG("Program broke execution: " impl_CTR11_AS_STRING(cond)); \
            CTR_LOG_DEBUG("- In file: " __FILE__);                                 \
            CTR_LOG_DEBUG("- On line: " impl_CTR11_AS_STRING(__LINE__));           \
            CTR_BREAK();                                                           \
        }                                                                          \
    } while (false)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

__attribute__((noreturn, cold)) void impl_ctr11_break(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_BREAK_H */