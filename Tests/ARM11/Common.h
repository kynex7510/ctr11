/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_TEST_COMMON_H
#define GUARD_CTR11_TEST_COMMON_H

#include <CTR11/Testing.h>

#define FAIL_IF(cond)           \
    do {                        \
        if ((cond)) {           \
            *reason = __LINE__; \
            return false;       \
        }                       \
    } while (0)

#define DEFINE_TEST(name) \
    CTR_TEST(name, reason, unused)

#endif /* GUARD_CTR11_TEST_COMMON_H */