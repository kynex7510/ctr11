/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_LOG_H
#define GUARD_CTR11_LOG_H

#include <CTR11/Defs.h>

#include <stdarg.h>

#define CTR_LOG(...)                                      \
    do {                                                  \
        impl_CTR11_GLOBAL_NS impl_ctr11_log(__VA_ARGS__); \
    } while (false)

#ifndef NDEBUG
#define CTR_LOG_DEBUG(...) CTR_LOG(__VA_ARGS__)
#else
#define CTR_LOG_DEBUG(...)
#endif // !NDEBUG

#define CTR_LOG_LOCATION(...)                                        \
    do {                                                             \
        CTR_LOG_DEBUG(__VA_ARGS__);                                  \
        CTR_LOG_DEBUG("- In file: " __FILE__);                       \
        CTR_LOG_DEBUG("- On line: " impl_CTR11_AS_STRING(__LINE__)); \
    } while (false)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void impl_ctr11_vlog(const char* fmt, va_list args);

CTR_INLINE void impl_ctr11_log(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    impl_ctr11_vlog(fmt, args);
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_LOG_H */