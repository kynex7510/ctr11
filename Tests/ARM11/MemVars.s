/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

.global memoryTestTextVar
.global memoryTestRodataVar
.global memoryTestDataVar

.section .text
memoryTestTextVar:
.word 0

.section .rodata
memoryTestRodataVar:
.word 0

.section .data
memoryTestDataVar:
.word 0