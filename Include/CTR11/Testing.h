/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_TESTING_H
#define GUARD_CTR11_TESTING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(CTR_ENABLE_TESTING)

#define CTR_TEST(name, func) \
    __attribute__((section("ctrtests"))) const TestEntry g_TestEntry_##name = { #name, func }

typedef bool (*TestFunc)(uint32_t* reason);

typedef struct {
    const char* name;
    TestFunc func;
} TestEntry;

typedef struct {
    double nsec;
    uint32_t reason;
    bool passed;
} TestResult;

typedef void (*TestCallback)(size_t index, const TestResult* result);

size_t RunTests(TestCallback callback);
size_t GetNumTests(void);
const char* GetTestName(size_t index);

#endif // CTR_ENABLE_TESTING

#endif /* GUARD_CTR11_TESTING_H */