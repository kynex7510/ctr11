/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CTR11/Testing.h>
#include <CTR11/Log.h>
#include <CTR11/Tick.h>

extern const TestEntry __start_ctrtests;
extern const TestEntry __stop_ctrtests;

static double parseTime(double nsec, const char** timeString) {
    double time = nsec;
    *timeString = "ns";

    if (time > 1000) {
        time /= 1000;
        *timeString = "us";

        if (time > 1000) {
            time /= 1000;
            *timeString = "ms";

            if (time > 1000) {
                time /= 1000;
                *timeString = "s";
            }
        }
    }

    return time;
}

static void testCallback(size_t index, const TestResult* result) {
    const char* timeString = NULL;
    const double time = parseTime(result->nsec, &timeString);

    CTR_LOG("[%u/%u] %s:", index + 1, GetNumTests(), GetTestName(index));
    CTR_LOG(" %s", (result->passed ? "\x1b[92mPASS\x1b[0m" : "\x1b[91mFAIL\x1b[0m"));

    if (!result->passed) {
        CTR_LOG(" (R: %lu)", result->reason);
    }

    // libn3ds doesn't support doubles.
#ifdef CTR_BM
    CTR_LOG(" (%u%s)", (uint32_t)time, timeString);
#else
    CTR_LOG(" (%.2f%s)", time, timeString);
#endif // CTR_BAREMETAL

    CTR_LOG("\n");
}

size_t RunTests(TestCallback callback) {
    size_t success = 0;

    for (const TestEntry* current = &__start_ctrtests; current != &__stop_ctrtests; ++current) {
        TestResult result;

        TickTimer timer;
        TickTimerStart(&timer);
        result.passed = current->func(&result.reason);
        result.nsec = TickTimerNs(TickTimerStop(&timer));

        (callback ? callback : testCallback)(current - &__start_ctrtests, &result);

        if (result.passed)
            ++success;
    }

    if (!callback) {
        const size_t numTests = GetNumTests();
        const size_t ratio = (double)success / numTests * 100;
        const size_t failed = numTests - success;
        CTR_LOG("-----------------------------------\n");
        CTR_LOG("%u%% tests passed, %u tests failed out of %u\n", ratio, failed, numTests);
    }

    return success;
}

size_t GetNumTests(void) { return &__stop_ctrtests - &__start_ctrtests; }

const char* GetTestName(size_t index) {
    const TestEntry* test = &(&__start_ctrtests)[index];
    if (test < &__stop_ctrtests)
        return test->name;

    return NULL;
}