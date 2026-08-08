/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <debug.h>
#include <arm11/fmt.h>

#include <string.h>
#else
#include <3ds.h>
#include <stdio.h>
#endif // CTR_BM

#include <CTR11/Break.h>
#include <CTR11/Log.h>

#ifdef CTR_BM

void impl_ctr11_break() { panic(); }

ssize_t con_write(const char *ptr, size_t len);

void impl_ctr11_vlog(const char* fmt, va_list args) {
    // TODO: ideally we would like to print to the dspico.
    char buf[256];
    ee_vsnprintf(buf, 256, fmt, args);
    con_write(buf, strlen(buf));
}

#else

void impl_ctr11_break(void) {
    while (true)
        svcBreak(USERBREAK_PANIC);
}

// CTR_LOG

void impl_ctr11_vlog(const char* fmt, va_list args) {
    vfprintf(stderr, fmt, args);
    fflush(stderr);
}

#endif // CTR_BM