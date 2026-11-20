/*
 * Host Simulator & Automated Verification Harness for PlexChem
 * 1) Verifies all 5 Kinetics solvers against Zumdahl 10th Ed. Ch. 12 Solutions PDF
 * 2) Renders pixel-exact 320x240 TI-84 Plus CE screenshots of every screen!
 */
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <assert.h>
#include "../src/hal.h"
#include "../src/gfx.h"
#include "../src/ui_app.h"

extern void host_save_ppm(const char *path);

static int g_pass = 0, g_fail = 0;

static void check_close(const char *name, double got, double expected, double tol_rel) {
    double err = fabs(got - expected) / (fabs(expected) > 1e-30 ? fabs(expected) : 1.0);
    if (err <= tol_rel) {
        printf("  [PASS] %-42s got=%g (exp=%g)\n", name, got, expected);
        g_pass++;
    } else {
        printf("  [FAIL] %-42s got=%g (exp=%g, rel_err=%.4f)\n", name, got, expected, err);
        g_fail++;
    }
}

static void run_zumdahl_verification(void) {
    printf("=== Zumdahl 10th Ed. Chapter 12 Automated Verification ===\n");

    /* 1) Notes 12.1 Table 12.1: NO2 0.0100 -> 0.0079 in 50s */
    RateCalcState rc = { .c1 = 0.0100, .c2 = 0.0079, .t1 = 0, .t2 = 50, .coeff = 2.0, .is_reactant = true };
    kinetics_rate_calc(&rc);
    check_close("12.1 NO2 species rate (-4.2e-5)", rc.species_rate, -4.2e-5, 0.01);
    check_close("12.1 Overall rxn rate (2.1e-5)", rc.overall_rate, 2.1e-5, 0.01);

    /* 2) Zumdahl Q25: 4 PH3 -> P4 + 6 H2, PH3 rate = 2.4e-3 */
    StoichRateState st = { .coeffs = {4, 1, 6, 0}, .is_product = {false, true, true, true}, .known_idx = 0, .known_rate = 2.4e-3 };
    kinetics_stoich_rates(&st);
    check_close("Q25 P4 production rate (6.0e-4)", st.rates[1], 6.0e-4, 0.01);
    check_close("Q25 H2 production rate (3.6e-3)", st.rates[2], 3.6e-3, 0.01);

    /* 3) Zumdahl Q31: 2NO + Cl2 -> 2NOCl (2nd in NO, 1st in Cl2, k=180) */
    InitialRatesState ir;
    memset(&ir, 0, sizeof(ir));
    ir.num_reactants = 2; ir.num_rows = 3;
    ir.conc[0][0] = 0.10; ir.conc[0][1] = 0.10; ir.rate[0] = 0.18;
    ir.conc[1][0] = 0.10; ir.conc[1][1] = 0.20; ir.rate[1] = 0.36;
    ir.conc[2][0] = 0.20; ir.conc[2][1] = 0.20; ir.rate[2] = 1.45;
    assert(kinetics_solve_initial_rates(&ir));
    check_close("Q31 NO order (2)", ir.order[0], 2.0, 0.01);
    check_close("Q31 Cl2 order (1)", ir.order[1], 1.0, 0.01);
    check_close("Q31 k_mean (180)", ir.k_mean, 180.4, 0.02);

    /* 4) Zumdahl Q100: Fractional order (x=1.5, y=1, k=33) with non-constant row 3! */
    memset(&ir, 0, sizeof(ir));
    ir.num_reactants = 2; ir.num_rows = 3;
    ir.conc[0][0] = 0.20; ir.conc[0][1] = 1.0; ir.rate[0] = 3.0;
    ir.conc[1][0] = 0.50; ir.conc[1][1] = 1.0; ir.rate[1] = 11.8;
    ir.conc[2][0] = 2.0;  ir.conc[2][1] = 2.0; ir.rate[2] = 189.5;
    assert(kinetics_solve_initial_rates(&ir));
    check_close("Q100 Fractional order A (1.5)", ir.order[0], 1.5, 0.01);
    check_close("Q100 Order B (1.0)", ir.order[1], 1.0, 0.01);
    check_close("Q100 k_mean (33.5)", ir.k_mean, 33.5, 0.03);

    /* 5) Zumdahl Q111: Negative order (-1 in OH-) with 3 reactants! */
    memset(&ir, 0, sizeof(ir));
    ir.num_reactants = 3; ir.num_rows = 4;
    ir.conc[0][0]=0.0013; ir.conc[0][1]=0.012; ir.conc[0][2]=0.10; ir.rate[0]=9.4e-3;
    ir.conc[1][0]=0.0026; ir.conc[1][1]=0.012; ir.conc[1][2]=0.10; ir.rate[1]=18.7e-3;
    ir.conc[2][0]=0.0013; ir.conc[2][1]=0.006; ir.conc[2][2]=0.10; ir.rate[2]=4.7e-3;
    ir.conc[3][0]=0.0013; ir.conc[3][1]=0.012; ir.conc[3][2]=0.20; ir.rate[3]=4.8e-3;
    assert(kinetics_solve_initial_rates(&ir));
    check_close("Q111 Order OH- (-1)", ir.order[2], -1.0, 0.01);
    check_close("Q111 k_mean (60.3)", ir.k_mean, 60.5, 0.03);

    /* 6) Zumdahl Q41: 2nd order NO2 data table regression */
    IntegratedTableState it;
    memset(&it, 0, sizeof(it));
    it.num_rows = 6;
    double t41[6] = {0, 1200, 3000, 4500, 9000, 18000};
    double c41[6] = {0.500, 0.444, 0.381, 0.340, 0.250, 0.174};
    memcpy(it.t, t41, sizeof(t41)); memcpy(it.c, c41, sizeof(c41));
    assert(kinetics_regress_order(&it));
    check_close("Q41 Best order (2nd)", (double)it.best_order, 2.0, 0.01);
    check_close("Q41 k (2.08e-4)", it.k, 2.08e-4, 0.02);

    /* 7) Zumdahl Q54: 1st order half-life 56.0d, time from 8.75e-5 to 1.41e-7 -> 519d */
    IntegratedEqState eq = { .order=1, .unknown=3, .A0=8.75e-5, .At=1.41e-7, .k=log(2.0)/56.0 };
    assert(kinetics_integrated_solve(&eq));
    check_close("Q54 First-order time (519 days)", eq.t, 519.0, 0.01);

    /* 8) Zumdahl Q73: Arrhenius 2-pt k1=3.52e-7, T1=555K, T2=645K, Ea=186 kJ -> k2=9.5e-5 */
    ArrhTwoPtState a2 = { .unknown=1, .k1=3.52e-7, .T1=555.0, .T2=645.0, .Ea_kj=186.0 };
    assert(kinetics_arrhenius_2pt(&a2));
    check_close("Q73 Arrhenius k2 (9.5e-5)", a2.k2, 9.5e-5, 0.03);

    /* 9) Zumdahl Q79c: Catalyst speedup dEa=2.1 kJ at 298 K -> 2.33x */
    CatalystState cat = { .Ea_uncat_kj=100.0, .Ea_cat_kj=97.9, .delta_E_kj=-50.0, .T_kelvin=298.0 };
    kinetics_catalyst_calc(&cat);
    check_close("Q79c Catalyst speedup (2.33x)", cat.speedup_ratio, 2.33, 0.02);

    /* 10) Mechanism Q115: CO + Cl2 -> COCl2 => Rate = k[CO][Cl2]^{3/2} */
    AppState tmp;
    app_init_defaults(&tmp);
    app_load_mech_preset(&tmp, 4);
    printf("  [INFO] Q115 Mech Rate Law: %s\n", tmp.mech.rate_law_math);
    assert(strstr(tmp.mech.rate_law_math, "3/2") != NULL);
    g_pass++;

    printf("=== Verification Summary: %d PASSED, %d FAILED ===\n", g_pass, g_fail);
    printf("=== Static AppState RAM footprint: %zu bytes ===\n", sizeof(AppState));
    assert(g_fail == 0);
}

int main(void) {
    run_zumdahl_verification();

    hal_init();
    gfx_init_palette();

    AppState app;
    app_init_defaults(&app);

    /* Render all 6 flagship screens at exact 320x240 TI-84 Plus CE resolution */
    app.screen = SCR_HOME;
    app_render(&app);
    host_save_ppm("preview_01_home.ppm");

    app.screen = SCR_INIT_RATES; app.sub_mode = 0;
    app_render(&app);
    host_save_ppm("preview_02_init_table.ppm");

    app.screen = SCR_INIT_RATES; app.sub_mode = 1; app.step_idx = 0;
    app_render(&app);
    host_save_ppm("preview_03_init_steps.ppm");

    app.screen = SCR_INTEGRATED; app.sub_mode = 0;
    app_render(&app);
    host_save_ppm("preview_04_integ_plots.ppm");

    app.screen = SCR_MECH;
    app_load_mech_preset(&app, 1); /* O3 -> O2 with [O2] denominator! */
    app_render(&app);
    host_save_ppm("preview_05_mech.ppm");

    app.screen = SCR_ARRHENIUS; app.sub_mode = 1;
    app_render(&app);
    host_save_ppm("preview_06_energy_plot.ppm");

    printf("Rendered 6 pixel-exact 320x240 TI-84 Plus CE previews.\n");
    return 0;
}
