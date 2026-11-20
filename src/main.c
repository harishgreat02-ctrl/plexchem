/*
 * PlexChem — AP Chemistry Chapter 12 (Chemical Kinetics) for TI-84 Plus CE
 * Single unified Flash App (app_tools) / .8xp with ZERO external .8xv libs
 * and ZERO dynamic memory allocation (static RAM < 5 KB, Flash ROM ~38 KB).
 */
#include "hal.h"
#include "gfx.h"
#include "ui_app.h"

/* Single static AppState in BSS — zero heap fragmentation, zero crash risk! */
static AppState g_app;

int main(void) {
    hal_init();
    gfx_init_palette();
    app_init_defaults(&g_app);

    bool running = true;
    while (running) {
        app_render(&g_app);
        KeyCode key = hal_wait_key();
        running = app_handle_key(&g_app, (int)key);
    }

    hal_shutdown();
    return 0;
}
