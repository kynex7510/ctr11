/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <arm.h>
#include <drivers/prng.h>
#else
#include <3ds.h>
#endif // CTR_BM

#include <CTR11/Random.h>

#ifdef CTR_BM
typedef u32 RandomValue;
#else
typedef s32 RandomValue;
#endif // CTR_BM

static RandomValue g_RandomState = 0x75107510;

void SeedRandom(void) {
    RandomValue x = g_RandomState;

#ifdef CTR_BM
    for (size_t i = 0; i < 4; ++i)
        x += (i & 1 ? PRNG_GetRand1() : PRNG_GetRand0());
#else
    RandomValue stack[4];

    if (R_SUCCEEDED(psInit())) {
        PS_GenerateRandomBytes(stack, sizeof(stack));
        psExit();
    }

    for (size_t i = 0; i < sizeof(stack) / sizeof(RandomValue); ++i)
        x += stack[i];
#endif // CTR_BM

    do {
        __ldrex(&g_RandomState);
    } while (__strex(&g_RandomState, x));
}

uint32_t GenRandom(void) {
    RandomValue x = g_RandomState;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;

    do {
        __ldrex(&g_RandomState);
    } while (__strex(&g_RandomState, x));

	return x;
}