/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_TICK_H
#define GUARD_CTR11_TICK_H

#include <CTR11/Assert.h>

typedef uint64_t TickTimer;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void TickTimerStart(TickTimer* t);
uint64_t TickTimerStop(TickTimer* t);
uint64_t TickTimerInterval(TickTimer* t);

CTR_INLINE double TickTimerNs(uint64_t value) {
    const double ticksPerNs = 268111856u / 1000000000.0;
    return value / ticksPerNs;
}

CTR_INLINE double TickTimerUs(uint64_t value) { return TickTimerNs(value) / 1000.0; }
CTR_INLINE double TickTimerMs(uint64_t value) { return TickTimerUs(value) / 1000.0; }

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_TICK_H */