/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_ATOMIC_H
#define GUARD_CTR11_ATOMIC_H

#include <CTR11/Defs.h>

// --v
#ifndef AtomicDecrement
#define AtomicDecrement(v) __atomic_sub_fetch(&v, 1, __ATOMIC_SEQ_CST)
#endif // AtomicDecrement

// v++
#ifndef AtomicPostIncrement
#define AtomicPostIncrement(v) __atomic_fetch_add(&v, 1, __ATOMIC_SEQ_CST)
#endif // AtomicPostIncrement

#endif /* GUARD_CTR11_ATOMIC_H */