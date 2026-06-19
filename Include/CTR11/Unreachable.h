/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_UNREACHABLE_H
#define GUARD_CTR11_UNREACHABLE_H

#include <CTR11/Break.h>
#include <CTR11/Log.h>

/* CTR_UNREACHABLE */

#define CTR_UNREACHABLE(...)           \
    do {                               \
        CTR_LOG_LOCATION(__VA_ARGS__); \
        CTR_BREAK();                   \
    } while (false)

#endif /* GUARD_CTR11_UNREACHABLE_H */