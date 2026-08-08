/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Random.h>

#include "Common.h"

DEFINE_TEST(Random) {
    uint32_t prev = GenRandom();
    FAIL_IF(GenRandom() == prev);

    SeedRandom();
    prev = GenRandom();

    SeedRandom();
    FAIL_IF(GenRandom() == prev);

    return true;
}