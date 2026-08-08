/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_RANDOM_H
#define GUARD_CTR11_RANDOM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void SeedRandom(void);
uint32_t GenRandom(void);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_RANDOM_H */