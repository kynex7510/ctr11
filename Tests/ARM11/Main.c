/**
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef CTR_BM
#include <arm11/power.h>
#include <arm11/console.h>
#include <arm11/drivers/hid.h>
#else
#include <3ds.h>
#endif // CTR_BM

#include <CTR11/Testing.h>

#ifdef CTR_BM
int main(int argc, char* argv[]) {
    GFX_init(GFX_BGR565, GFX_BGR565, GFX_TOP_2D);
    GFX_setLcdLuminance(100);
    consoleInit(GFX_LCD_TOP, NULL);

    RunTests(NULL);

    while (true) {
        hidScanInput();
        const u32 keyDown = hidKeysDown();

        if (keyDown & KEY_START)
            break;

        GFX_swapBuffers();
        GFX_waitForVBlank0();
    }

    GFX_deinit();
    power_off();
    return 0;
}
#else
int main(int argc, char* argv[]) {
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);

    RunTests(NULL);

    while (aptMainLoop()) {
        hidScanInput();
        const u32 keyDown = hidKeysDown();

        if (keyDown & KEY_START)
            break;

        gfxSwapBuffers();
        gspWaitForVBlank();
    }

    gfxExit();
    return 0;
}
#endif // CTR_BM