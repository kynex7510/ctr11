/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <arm11/drivers/timer.h>
#else
#include <3ds.h>
#endif // CTR_BM

#include <CTR11/Tick.h>

#ifdef CTR_BM

void TickTimerStart(TickTimer* t) {
    CTR_ASSERT(t);
    // With a prescaler value of 1 the timer decrements every 2 clock cycles.
    TIMER_start(1, 0xFFFFFFFFu, TIMER_SINGLE_SHOT);
    *t = 0xFFFFFFFF;
}

uint64_t TickTimerStop(TickTimer* t) {
    CTR_ASSERT(t);

    if (*t) {
        const uint64_t delta = (*t - TIMER_stop()) << 1;
        *t = 0;
        return delta;
    }

    return 0;
}

uint64_t TickTimerInterval(TickTimer* t) {
    CTR_ASSERT(t);
    const uint64_t newp = TIMER_getTicks();
    const uint64_t delta = (*t - newp) << 1;
    *t = newp;
    return delta;
}

#else

void TickTimerStart(TickTimer* t) {
    CTR_ASSERT(t);
    *t = svcGetSystemTick();
}

uint64_t TickTimerStop(TickTimer* t) {
    CTR_ASSERT(t);

    const uint64_t invalid = (uint64_t)-1;
    if (*t != invalid) {
        const uint64_t delta = svcGetSystemTick() - *t;
        *t = invalid;
        return delta;
    }

    return 0;
}

uint64_t TickTimerInterval(TickTimer* t) {
    CTR_ASSERT(t);
    const uint64_t newp = svcGetSystemTick();
    const uint64_t delta = newp - *t;
    *t = newp;
    return delta;
}

#endif // CTR_BM