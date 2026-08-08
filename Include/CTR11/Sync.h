/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef GUARD_CTR11_SYNC_H
#define GUARD_CTR11_SYNC_H

#include <CTR11/Defs.h>

typedef struct CTRMtxImpl* Mutex;
typedef struct CTRCVImpl* CV;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

void Sleep(uint64_t ns);

CTR_INLINE void Yield(void) { Sleep(0); }

Mutex CreateMutex(void);
void DestroyMutex(Mutex m);
void AcquireMutex(Mutex m);
void ReleaseMutex(Mutex m);

CV CreateCV(void);
void DestroyCV(CV cv);
void WaitCV(CV cv, Mutex m);
void NotifyCV(CV cv, size_t count);
void BroadcastCV(CV cv);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif /* GUARD_CTR11_SYNC_H */