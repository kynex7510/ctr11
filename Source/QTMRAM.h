/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_QTMRAM_H
#define GUARD_CTR11_QTMRAM_H

#include <CTR11/Defs.h>

void qtmramQueryRegion(uintptr_t* regionBase, size_t* regionSize);
void* qtmramMemAlign(size_t size, size_t alignment);

void qtmramFree(void* p);
size_t qtmramGetSize(const void* p);

#endif // GUARD_CTR11_QTMRAM_H