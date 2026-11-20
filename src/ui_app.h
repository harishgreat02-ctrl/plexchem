#ifndef PLEXCHEM_UI_APP_H
#define PLEXCHEM_UI_APP_H

#include "kinetics.h"

typedef enum {
    SCR_HOME = 0,
    SCR_RATES = 1,
    SCR_INIT_RATES = 2,
    SCR_INTEGRATED = 3,
    SCR_MECH = 4,
    SCR_ARRHENIUS = 5,
    SCR_SUMMARY = 6
} ScreenId;

typedef struct {
    ScreenId screen;
    int home_sel;       /* 0..5 */
    int sub_mode;       /* sub-view within current module */
    int step_idx;       /* active PlexMath step highlight (0..3) */
    int cursor_r;
    int cursor_c;
    int mech_preset;    /* 0..5 */

    RateCalcState       rates;
    StoichRateState     stoich;
    InitialRatesState   init_rates;
    IntegratedTableState int_table;
    IntegratedEqState   int_eq;
    MechState           mech;
    ArrhTwoPtState      arrh2;
    ArrhTableState      arrh_tbl;
    CatalystState       cat;
} AppState;

void app_init_defaults(AppState *app);
void app_load_mech_preset(AppState *app, int idx);
void app_render(const AppState *app);
bool app_handle_key(AppState *app, int key);

#endif /* PLEXCHEM_UI_APP_H */
