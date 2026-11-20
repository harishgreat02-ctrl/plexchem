#include "ui_app.h"
#include "hal.h"
#include "gfx.h"
#include "math_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static bool ui_edit_double(const char *prompt, double *val) {
    char buf[24];
    fmt_compact(*val, buf, sizeof(buf));
    int len = (int)strlen(buf);
    bool first_key = true;

    for (;;) {
        gfx_fill_rrect(32, 74, 256, 90, 4, COL_WHITE);
        gfx_draw_rrect(32, 74, 256, 90, 4, COL_ACCENT);
        gfx_fill_rect(33, 75, 254, 22, COL_BLUE_BG);
        gfx_draw_text(42, 79, &FONT_UI_11B, prompt, RAMP_ACCENT_BLUE);

        gfx_fill_rrect(44, 105, 232, 26, 3, COL_GRAY_BG);
        gfx_draw_rrect(44, 105, 232, 26, 3, COL_BORDER);
        int tw = gfx_draw_text(52, 111, &FONT_UI_13B, buf, RAMP_INK_GRAY);
        gfx_vline(52 + tw + 2, 110, 16, COL_ACCENT);

        ui_draw_keycap(44, 140, "ENTER", "save");
        ui_draw_keycap(126, 140, ",", "e(10^x)");
        ui_draw_keycap(214, 140, "CLR", "back");
        hal_swap_buffers();

        KeyCode k = hal_wait_key();
        if (k == KEY_CLEAR || k == KEY_QUIT) return false;
        if (k == KEY_ENTER) {
            if (len > 0) *val = strtod(buf, NULL);
            return true;
        }
        if (k == KEY_DEL) {
            first_key = false;
            if (len > 0) buf[--len] = '\0';
            continue;
        }
        char ch = 0;
        if (k >= KEY_0 && k <= KEY_9) ch = (char)('0' + (k - KEY_0));
        else if (k == KEY_DOT) ch = '.';
        else if (k == KEY_NEG || k == KEY_MINUS) ch = '-';
        else if (k == KEY_EE) ch = 'e';

        if (ch != 0) {
            if (first_key) { len = 0; buf[0] = '\0'; first_key = false; }
            if (len < 20) { buf[len++] = ch; buf[len] = '\0'; }
        }
    }
}

static const char * const SPECIES_POOL[] = {
    "--", "A", "B", "C", "D", "I1", "I2", "Cat",
    "NO", "NO2", "NO3", "Cl2", "Cl", "NOCl", "NOCl2",
    "O3", "O2", "O", "CO", "CO2", "H2", "I2", "I", "HI", "H2O2", "OH"
};
#define NUM_SPECIES_POOL ((int)(sizeof(SPECIES_POOL) / sizeof(SPECIES_POOL[0])))

static int find_pool_idx(const char *name) {
    for (int i = 0; i < NUM_SPECIES_POOL; i++) {
        if (strcmp(SPECIES_POOL[i], name) == 0) return i;
    }
    return 1;
}

static void ui_edit_mech_step(MechStep *st, int step_num) {
    int r_id[2] = { 0, 0 }, r_co[2] = { 1, 1 };
    int p_id[2] = { 0, 0 }, p_co[2] = { 1, 1 };
    if (st->n_react >= 1) { r_id[0] = find_pool_idx(st->react[0].formula); r_co[0] = st->react[0].coeff; }
    if (st->n_react >= 2) { r_id[1] = find_pool_idx(st->react[1].formula); r_co[1] = st->react[1].coeff; }
    if (st->n_prod >= 1)  { p_id[0] = find_pool_idx(st->prod[0].formula);  p_co[0] = st->prod[0].coeff; }
    if (st->n_prod >= 2)  { p_id[1] = find_pool_idx(st->prod[1].formula);  p_co[1] = st->prod[1].coeff; }

    int row = 0;
    for (;;) {
        gfx_fill_rrect(20, 34, 280, 174, 4, COL_WHITE);
        gfx_draw_rrect(20, 34, 280, 174, 4, COL_ACCENT);
        gfx_fill_rect(21, 35, 278, 22, COL_BLUE_BG);
        char hdr[64]; snprintf(hdr, sizeof(hdr), "Edit Step %d  (L/R species, +/- coeff)", step_num + 1);
        gfx_draw_text(28, 39, &FONT_UI_11B, hdr, RAMP_ACCENT_BLUE);

        const char *rlab[5] = { "Reactant 1", "Reactant 2", "Product 1", "Product 2", "Step Speed" };
        for (int i = 0; i < 5; i++) {
            int y = 62 + i * 24;
            bool sel = (row == i);
            if (sel) gfx_fill_rrect(28, y, 264, 20, 3, COL_BLUE_BG);
            gfx_draw_text(36, y + 3, &FONT_UI_11B, rlab[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);

            char val[28];
            if (i < 2) {
                if (r_id[i] == 0) strcpy(val, "-- (none)");
                else snprintf(val, sizeof(val), "%d %s", r_co[i], SPECIES_POOL[r_id[i]]);
            } else if (i < 4) {
                int j = i - 2;
                if (p_id[j] == 0) strcpy(val, "-- (none)");
                else snprintf(val, sizeof(val), "%d %s", p_co[j], SPECIES_POOL[p_id[j]]);
            } else {
                strcpy(val, st->speed == STEP_SLOW ? "SLOW (RDS)" : (st->speed == STEP_FAST_EQ ? "FAST EQ (" SYM_EQUIL ")" : "FAST"));
            }
            gfx_draw_text_right(284, y + 3, &FONT_UI_11B, val, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }

        ui_draw_keycap(28, 186, "L/R", "pick");
        ui_draw_keycap(98, 186, "+/-", "coeff");
        ui_draw_keycap(180, 186, "ENTER", "save");
        hal_swap_buffers();

        KeyCode k = hal_wait_key();
        if (k == KEY_CLEAR || k == KEY_QUIT) return;
        if (k == KEY_UP && row > 0) row--;
        if (k == KEY_DOWN && row < 4) row++;
        if (k == KEY_LEFT || k == KEY_RIGHT) {
            int d = (k == KEY_RIGHT) ? 1 : -1;
            if (row < 2) r_id[row] = (r_id[row] + NUM_SPECIES_POOL + d) % NUM_SPECIES_POOL;
            else if (row < 4) p_id[row - 2] = (p_id[row - 2] + NUM_SPECIES_POOL + d) % NUM_SPECIES_POOL;
            else st->speed = (StepSpeed)(((int)st->speed + 3 + d) % 3);
        }
        if (k == KEY_PLUS || k == KEY_MINUS) {
            int d = (k == KEY_PLUS) ? 1 : -1;
            if (row < 2) { r_co[row] += d; if (r_co[row] < 1) r_co[row] = 1; if (r_co[row] > 3) r_co[row] = 3; }
            else if (row < 4) { int j = row - 2; p_co[j] += d; if (p_co[j] < 1) p_co[j] = 1; if (p_co[j] > 3) p_co[j] = 3; }
        }
        if (k == KEY_ENTER) {
            st->n_react = 0;
            for (int i = 0; i < 2; i++) {
                if (r_id[i] > 0) {
                    strcpy(st->react[st->n_react].formula, SPECIES_POOL[r_id[i]]);
                    st->react[st->n_react].coeff = r_co[i];
                    st->n_react++;
                }
            }
            st->n_prod = 0;
            for (int i = 0; i < 2; i++) {
                if (p_id[i] > 0) {
                    strcpy(st->prod[st->n_prod].formula, SPECIES_POOL[p_id[i]]);
                    st->prod[st->n_prod].coeff = p_co[i];
                    st->n_prod++;
                }
            }
            return;
        }
    }
}

void app_load_mech_preset(AppState *app, int idx) {
    app->mech_preset = idx % 5;
    MechState *m = &app->mech;
    memset(m, 0, sizeof(*m));

    if (app->mech_preset == 0) {
        m->num_steps = 2;
        m->steps[0].speed = STEP_FAST_EQ;
        m->steps[0].n_react = 2;
        strcpy(m->steps[0].react[0].formula, "NO");  m->steps[0].react[0].coeff = 1;
        strcpy(m->steps[0].react[1].formula, "Cl2"); m->steps[0].react[1].coeff = 1;
        m->steps[0].n_prod = 1;
        strcpy(m->steps[0].prod[0].formula, "NOCl2"); m->steps[0].prod[0].coeff = 1;

        m->steps[1].speed = STEP_SLOW;
        m->steps[1].n_react = 2;
        strcpy(m->steps[1].react[0].formula, "NOCl2"); m->steps[1].react[0].coeff = 1;
        strcpy(m->steps[1].react[1].formula, "NO");    m->steps[1].react[1].coeff = 1;
        m->steps[1].n_prod = 1;
        strcpy(m->steps[1].prod[0].formula, "NOCl"); m->steps[1].prod[0].coeff = 2;
    } else if (app->mech_preset == 1) {
        m->num_steps = 2;
        m->steps[0].speed = STEP_FAST_EQ;
        m->steps[0].n_react = 1;
        strcpy(m->steps[0].react[0].formula, "O3"); m->steps[0].react[0].coeff = 1;
        m->steps[0].n_prod = 2;
        strcpy(m->steps[0].prod[0].formula, "O2"); m->steps[0].prod[0].coeff = 1;
        strcpy(m->steps[0].prod[1].formula, "O");  m->steps[0].prod[1].coeff = 1;

        m->steps[1].speed = STEP_SLOW;
        m->steps[1].n_react = 2;
        strcpy(m->steps[1].react[0].formula, "O3"); m->steps[1].react[0].coeff = 1;
        strcpy(m->steps[1].react[1].formula, "O");  m->steps[1].react[1].coeff = 1;
        m->steps[1].n_prod = 1;
        strcpy(m->steps[1].prod[0].formula, "O2"); m->steps[1].prod[0].coeff = 2;
    } else if (app->mech_preset == 2) {
        m->num_steps = 2;
        m->steps[0].speed = STEP_SLOW;
        m->steps[0].n_react = 1;
        strcpy(m->steps[0].react[0].formula, "NO2"); m->steps[0].react[0].coeff = 2;
        m->steps[0].n_prod = 2;
        strcpy(m->steps[0].prod[0].formula, "NO3"); m->steps[0].prod[0].coeff = 1;
        strcpy(m->steps[0].prod[1].formula, "NO");  m->steps[0].prod[1].coeff = 1;

        m->steps[1].speed = STEP_FAST;
        m->steps[1].n_react = 2;
        strcpy(m->steps[1].react[0].formula, "NO3"); m->steps[1].react[0].coeff = 1;
        strcpy(m->steps[1].react[1].formula, "CO");  m->steps[1].react[1].coeff = 1;
        m->steps[1].n_prod = 2;
        strcpy(m->steps[1].prod[0].formula, "NO2"); m->steps[1].prod[0].coeff = 1;
        strcpy(m->steps[1].prod[1].formula, "CO2"); m->steps[1].prod[1].coeff = 1;
    } else if (app->mech_preset == 3) {
        m->num_steps = 2;
        m->steps[0].speed = STEP_SLOW;
        m->steps[0].n_react = 2;
        strcpy(m->steps[0].react[0].formula, "O3"); m->steps[0].react[0].coeff = 1;
        strcpy(m->steps[0].react[1].formula, "Cl"); m->steps[0].react[1].coeff = 1;
        m->steps[0].n_prod = 2;
        strcpy(m->steps[0].prod[0].formula, "O2");  m->steps[0].prod[0].coeff = 1;
        strcpy(m->steps[0].prod[1].formula, "ClO"); m->steps[0].prod[1].coeff = 1;

        m->steps[1].speed = STEP_FAST;
        m->steps[1].n_react = 2;
        strcpy(m->steps[1].react[0].formula, "ClO"); m->steps[1].react[0].coeff = 1;
        strcpy(m->steps[1].react[1].formula, "O");   m->steps[1].react[1].coeff = 1;
        m->steps[1].n_prod = 2;
        strcpy(m->steps[1].prod[0].formula, "O2"); m->steps[1].prod[0].coeff = 1;
        strcpy(m->steps[1].prod[1].formula, "Cl"); m->steps[1].prod[1].coeff = 1;
    } else {
        m->num_steps = 3;
        m->steps[0].speed = STEP_FAST_EQ;
        m->steps[0].n_react = 1;
        strcpy(m->steps[0].react[0].formula, "Cl2"); m->steps[0].react[0].coeff = 1;
        m->steps[0].n_prod = 1;
        strcpy(m->steps[0].prod[0].formula, "Cl"); m->steps[0].prod[0].coeff = 2;

        m->steps[1].speed = STEP_FAST_EQ;
        m->steps[1].n_react = 2;
        strcpy(m->steps[1].react[0].formula, "Cl"); m->steps[1].react[0].coeff = 1;
        strcpy(m->steps[1].react[1].formula, "CO"); m->steps[1].react[1].coeff = 1;
        m->steps[1].n_prod = 1;
        strcpy(m->steps[1].prod[0].formula, "COCl"); m->steps[1].prod[0].coeff = 1;

        m->steps[2].speed = STEP_SLOW;
        m->steps[2].n_react = 2;
        strcpy(m->steps[2].react[0].formula, "COCl"); m->steps[2].react[0].coeff = 1;
        strcpy(m->steps[2].react[1].formula, "Cl2");  m->steps[2].react[1].coeff = 1;
        m->steps[2].n_prod = 2;
        strcpy(m->steps[2].prod[0].formula, "COCl2"); m->steps[2].prod[0].coeff = 1;
        strcpy(m->steps[2].prod[1].formula, "Cl");    m->steps[2].prod[1].coeff = 1;
    }
    kinetics_analyze_mech(m);
}

void app_init_defaults(AppState *app) {
    memset(app, 0, sizeof(*app));
    app->screen = SCR_HOME;
    app->k_unit_order = 2.0;
    app->pseudo_kp = 0.12;
    app->pseudo_excess = 3.0;
    app->pseudo_order = 2.0;

    app->rates.c1 = 0.0100; app->rates.c2 = 0.0079;
    app->rates.t1 = 0.0;    app->rates.t2 = 50.0;
    app->rates.coeff = 2.0; app->rates.is_reactant = true;
    kinetics_rate_calc(&app->rates);

    app->stoich.coeffs[0] = 1; app->stoich.is_product[0] = false;
    app->stoich.coeffs[1] = 3; app->stoich.is_product[1] = false;
    app->stoich.coeffs[2] = 2; app->stoich.is_product[2] = true;
    app->stoich.coeffs[3] = 0; app->stoich.is_product[3] = true;
    app->stoich.known_idx = 1; app->stoich.known_rate = 0.060;
    kinetics_stoich_rates(&app->stoich);

    app->init_rates.num_reactants = 2;
    app->init_rates.num_rows = 4;
    double c[4][2] = {{0.100, 0.100}, {0.100, 0.300}, {0.200, 0.100}, {0.100, 0.200}};
    double r[4]    = {1.53e-4, 4.59e-4, 6.12e-4, 3.06e-4};
    for (int i = 0; i < 4; i++) {
        app->init_rates.conc[i][0] = c[i][0];
        app->init_rates.conc[i][1] = c[i][1];
        app->init_rates.conc[i][2] = 0.100;
        app->init_rates.rate[i] = r[i];
    }
    app->init_rates.pred_conc[0] = 0.300;
    app->init_rates.pred_conc[1] = 0.600;
    app->init_rates.pred_conc[2] = 0.100;
    kinetics_solve_initial_rates(&app->init_rates);

    app->int_table.num_rows = 4;
    double tt[4] = {0, 2, 4, 6};
    double cc[4] = {1.00, 0.50, 0.3333, 0.25};
    for (int i = 0; i < 4; i++) { app->int_table.t[i] = tt[i]; app->int_table.c[i] = cc[i]; }
    kinetics_regress_order(&app->int_table);

    app->int_eq.order = 1;
    app->int_eq.unknown = 1;
    app->int_eq.A0 = 0.160;
    app->int_eq.At = 0.0303;
    app->int_eq.k  = 0.416;
    app->int_eq.t  = 4.0;
    kinetics_integrated_solve(&app->int_eq);

    app_load_mech_preset(app, 0);

    app->arrh2.unknown = 1;
    app->arrh2.k1 = 3.52e-7;
    app->arrh2.T1 = 555.0;
    app->arrh2.T2 = 645.0;
    app->arrh2.Ea_kj = 186.0;
    kinetics_arrhenius_2pt(&app->arrh2);

    app->arrh_tbl.num_rows = 3;
    app->arrh_tbl.T[0] = 338; app->arrh_tbl.k[0] = 4.9e-3;
    app->arrh_tbl.T[1] = 318; app->arrh_tbl.k[1] = 5.0e-4;
    app->arrh_tbl.T[2] = 298; app->arrh_tbl.k[2] = 3.5e-5;
    app->arrh_tbl.eval_T = 308;
    kinetics_arrhenius_regress(&app->arrh_tbl);

    app->cat.Ea_uncat_kj = 125.0;
    app->cat.Ea_cat_kj   = 85.0;
    app->cat.delta_E_kj  = -216.0;
    app->cat.T_kelvin    = 298.0;
    kinetics_catalyst_calc(&app->cat);
}

static void render_home(const AppState *app) {
    static const char * const TITLES[6] = {
        "Rates",        "Init. Rates", "Integrated",
        "Mechanisms",   "Arrhenius",   "Ref & Graphs"
    };
    static const char * const SUBS[6] = {
        "Avg, Stoich, k", "Order Table", "Plots & t1/2",
        "Custom Steps",   "Ea, T & Cat", "5 Test Guides"
    };

    ui_draw_topbar("PlexChem", "CH. 12 KINETICS");

    for (int i = 0; i < 6; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 10 + col * 102;
        int y = 28 + row * 82;
        int w = 96, h = 74;
        bool sel = (app->home_sel == i);

        gfx_fill_rrect(x, y, w, h, 4, sel ? COL_BLUE_BG : COL_WHITE);
        gfx_draw_rrect(x, y, w, h, 4, sel ? COL_ACCENT : COL_BORDER);
        if (sel) gfx_draw_rrect(x + 1, y + 1, w - 2, h - 2, 3, COL_ACCENT);

        char nb[2] = { (char)('1' + i), '\0' };
        gfx_fill_rrect(x + 6, y + 6, 15, 14, 2, sel ? COL_ACCENT : COL_GRAY_BG);
        gfx_draw_text_center(x + 13, y + 6, &FONT_UI_11B, nb,
                             sel ? RAMP_WHITE_ACCENT : RAMP_MUTED_WHITE);

        gfx_draw_icon(x + (w - 18) / 2 + 4, y + 10, (uint8_t)i,
                      sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);

        gfx_draw_text_center(x + w / 2, y + 34, &FONT_UI_13B, TITLES[i],
                             sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
        gfx_draw_text_center(x + w / 2, y + 53, &FONT_UI_11, SUBS[i],
                             sel ? RAMP_INK_BLUE : RAMP_MUTED_WHITE);
    }

    ui_draw_keycap(10, 196, "ENTER", "open");
    ui_draw_keycap(106, 196, "1-6", "quick");
    ui_draw_keycap(196, 196, "CLEAR", "quit");
    ui_draw_bottom_tabs(-1);
}

static void render_rates(const AppState *app) {
    const char *mnames[3] = { "RATES " SYM_DOT " AVG", "RATES " SYM_DOT " STOICH", "RATES " SYM_DOT " k UNITS" };
    ui_draw_topbar("PlexChem", mnames[app->sub_mode]);

    if (app->sub_mode == 0) {
        gfx_fill_rrect(10, 26, 300, 56, 4, COL_WHITE);
        gfx_draw_rrect(10, 26, 300, 56, 4, COL_BORDER);
        math_draw_center(160, 33,
                         app->rates.is_reactant
                             ? "~Rate~ = " SYM_MINUS "\\f{" SYM_DELTA "[A]}{~a~" SYM_DELTA "~t~}"
                             : "~Rate~ = +\\f{" SYM_DELTA "[C]}{~c~" SYM_DELTA "~t~}",
                         14, RAMP_MUTED_WHITE, COL_MUTED);
        gfx_hline(24, 54, 272, COL_BORDER);
        char rbuf[48], eqbuf[80];
        fmt_sci_math(app->rates.overall_rate, rbuf, sizeof(rbuf), 3);
        snprintf(eqbuf, sizeof(eqbuf), "~Rate~ = %s \\f{mol}{L" SYM_DOT "s}", rbuf);
        math_draw_center(160, 60, eqbuf, 14, RAMP_INK_WHITE, COL_INK);

        const char *labels[5] = { "[A]1 (M)", "[A]2 (M)", "t1 (s)", "t2 (s)", "Coeff a" };
        double vals[5] = { app->rates.c1, app->rates.c2, app->rates.t1, app->rates.t2, app->rates.coeff };
        for (int i = 0; i < 5; i++) {
            int y = 89 + i * 20;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(10, y, 300, 18, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 2, &FONT_UI_11B, labels[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            char vb[32]; fmt_compact(vals[i], vb, sizeof(vb));
            gfx_draw_text_right(302, y + 2, &FONT_UI_11, vb, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }
        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(92, 196, "MODE", "stoich/k");
        ui_draw_keycap(204, 196, "APPS", app->rates.is_reactant ? "React" : "Prod");
    } else if (app->sub_mode == 1) {
        char hdr[64];
        snprintf(hdr, sizeof(hdr), "%.0fA + %.0fB " SYM_ARROW " %.0fC",
                 app->stoich.coeffs[0], app->stoich.coeffs[1], app->stoich.coeffs[2]);
        math_draw_center(160, 27, hdr, 14, RAMP_INK_WHITE, COL_INK);

        gfx_fill_rrect(10, 49, 300, 34, 4, COL_BLUE_BG);
        gfx_draw_text(18, 54, &FONT_UI_11B, "Overall Reaction Rate", RAMP_INK_BLUE);
        char rb[40]; fmt_sci_math(app->stoich.overall_rate, rb, sizeof(rb), 3);
        math_draw_right(302, 56, rb, 14, RAMP_ACCENT_BLUE, COL_ACCENT);
        gfx_draw_text(18, 68, &FONT_UI_11, "mol/(L" SYM_DOT "s)  [sign: - consumed, + formed]", RAMP_MUTED_BLUE);

        const char *sp[3] = { "Species A (Reactant)", "Species B (Reactant)", "Species C (Product)" };
        for (int i = 0; i < 3; i++) {
            int y = 90 + i * 32;
            bool sel = (app->cursor_r == i);
            gfx_fill_rrect(10, y, 300, 28, 3, sel ? COL_BLUE_BG : COL_WHITE);
            gfx_draw_rrect(10, y, 300, 28, 3, sel ? COL_ACCENT : COL_BORDER);
            gfx_draw_text(18, y + 3, &FONT_UI_11B, sp[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            char cb[32]; snprintf(cb, sizeof(cb), "coeff = %.0f", app->stoich.coeffs[i]);
            gfx_draw_text(18, y + 15, &FONT_UI_11, cb, RAMP_MUTED_WHITE);
            char sb[40]; fmt_sci_math(app->stoich.rates[i], sb, sizeof(sb), 3);
            math_draw_right(302, y + 6, sb, 14, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE, COL_INK);
        }
        ui_draw_keycap(10, 196, "ENTER", "coeff");
        ui_draw_keycap(96, 196, "APPS", "set rate");
        ui_draw_keycap(206, 196, "MODE", "k units");
    } else {
        gfx_fill_rrect(10, 28, 300, 82, 4, COL_BLUE_BG);
        char oline[48]; snprintf(oline, sizeof(oline), "Overall Order  ~n~ = %.2g", app->k_unit_order);
        math_draw_center(160, 36, oline, 14, RAMP_ACCENT_BLUE, COL_ACCENT);
        char ku[48], kfull[80];
        kinetics_format_k_units(app->k_unit_order, "s", ku, sizeof(ku));
        snprintf(kfull, sizeof(kfull), "~k~ units = %s", ku);
        math_draw_center(160, 64, kfull, 20, RAMP_INK_BLUE, COL_INK);

        gfx_fill_rrect(10, 118, 300, 70, 4, COL_WHITE);
        gfx_draw_rrect(10, 118, 300, 70, 4, COL_BORDER);
        gfx_draw_text(18, 124, &FONT_UI_11B, "Rule:  k = M^{1-n} / time", RAMP_INK_WHITE);
        gfx_draw_text(18, 141, &FONT_UI_11, "n=0: mol/(L" SYM_DOT "s)   n=1: s^-1   n=2: L/(mol" SYM_DOT "s)", RAMP_MUTED_WHITE);
        gfx_draw_text(18, 157, &FONT_UI_11, "n=3: L^2/(mol^2" SYM_DOT "s)  n=1.5: L^0.5/(mol^0.5" SYM_DOT "s)", RAMP_MUTED_WHITE);
        gfx_draw_text(18, 172, &FONT_UI_11, "Press LEFT/RIGHT (+/-0.5) or ENTER for any n", RAMP_ACCENT_WHITE);

        ui_draw_keycap(10, 196, "L/R", "+/-0.5");
        ui_draw_keycap(96, 196, "ENTER", "custom n");
        ui_draw_keycap(214, 196, "MODE", "avg");
    }
    ui_draw_bottom_tabs(0);
}

static void render_init_rates(const AppState *app) {
    const InitialRatesState *ir = &app->init_rates;
    if (app->sub_mode == 0) {
        char st[32];
        snprintf(st, sizeof(st), "%dR " SYM_TIMES " %d ROWS", ir->num_reactants, ir->num_rows);
        ui_draw_topbar("Initial Rates", st);

        char p1[24], p2[24];
        snprintf(p1, sizeof(p1), "React: %d", ir->num_reactants);
        snprintf(p2, sizeof(p2), "Rows: %d", ir->num_rows);
        ui_draw_pill(10, 25, p1, COL_BLUE_BG, RAMP_ACCENT_BLUE);
        ui_draw_pill(90, 25, p2, COL_GRAY_BG, RAMP_INK_GRAY);
        ui_draw_pill(168, 25, "APPS: Solve & Steps", COL_GREEN_BG, RAMP_GREEN_BG);

        int cols = ir->num_reactants + 1;
        int cw = 266 / cols;
        int y0 = 46;
        gfx_fill_rect(10, y0, 300, 18, COL_CHROME);
        gfx_draw_rect(10, y0, 300, 18, COL_BORDER);
        gfx_draw_text(16, y0 + 2, &FONT_UI_11B, "#", RAMP_MUTED_CHROME);
        const char *chdr[3] = { "[A]", "[B]", "[C]" };
        for (int c = 0; c < ir->num_reactants; c++) {
            gfx_draw_text_center(44 + c * cw + cw / 2, y0 + 2, &FONT_UI_11B, chdr[c], RAMP_INK_CHROME);
        }
        gfx_draw_text_center(44 + ir->num_reactants * cw + cw / 2, y0 + 2, &FONT_UI_11B, "Rate", RAMP_INK_CHROME);

        int vis = ir->num_rows < 6 ? ir->num_rows : 6;
        int start_r = (app->cursor_r >= vis) ? (app->cursor_r - vis + 1) : 0;
        for (int vi = 0; vi < vis; vi++) {
            int r = start_r + vi;
            int ry = y0 + 18 + vi * 20;
            gfx_draw_rect(10, ry, 300, 20, COL_BORDER);
            char rb[16]; snprintf(rb, sizeof(rb), "%d", r + 1);
            gfx_draw_text(16, ry + 3, &FONT_UI_11, rb, RAMP_MUTED_WHITE);

            for (int c = 0; c < cols; c++) {
                int cx = 44 + c * cw;
                bool sel = (app->cursor_r == r && app->cursor_c == c);
                if (sel) {
                    gfx_fill_rect(cx + 1, ry + 1, cw - 2, 18, COL_BLUE_BG);
                    gfx_draw_rect(cx, ry, cw, 20, COL_ACCENT);
                }
                double v = (c < ir->num_reactants) ? ir->conc[r][c] : ir->rate[r];
                char vb[16]; fmt_compact(v, vb, sizeof(vb));
                gfx_draw_text_center(cx + cw / 2, ry + 3, &FONT_UI_11, vb,
                                     sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            }
        }
        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(92, 196, "+/-", "rows");
        ui_draw_keycap(166, 196, "MODE", "react#");
    } else {
        char st[24]; snprintf(st, sizeof(st), "STEPS %d / 3", app->step_idx + 1);
        ui_draw_topbar("PlexChem", st);

        char rl[96] = "~Rate~ = ~k~";
        const char *rn[3] = { "A", "B", "C" };
        for (int j = 0; j < ir->num_reactants; j++) {
            double o = ir->order[j];
            if (fabs(o) < 1e-6) continue;
            char t[28];
            if (fabs(o - 1.0) < 1e-6) snprintf(t, sizeof(t), "[%s]", rn[j]);
            else if (fabs(o - floor(o + 0.5)) < 1e-6) {
                int io = (int)floor(o + 0.5);
                if (io < 0) snprintf(t, sizeof(t), "[%s]^{" SYM_MINUS "%d}", rn[j], -io);
                else snprintf(t, sizeof(t), "[%s]^{%d}", rn[j], io);
            } else snprintf(t, sizeof(t), "[%s]^{%.1f}", rn[j], o);
            strncat(rl, t, sizeof(rl) - strlen(rl) - 1);
        }
        math_draw_center(160, 25, rl, 20, RAMP_INK_WHITE, COL_INK);

        int cur_y = 54;
        for (int s = 0; s < 3; s++) {
            bool act = (app->step_idx == s);
            if (act) {
                gfx_fill_rrect(8, cur_y, 304, 66, 4, COL_BLUE_BG);
                char tag[12]; snprintf(tag, sizeof(tag), "STEP %d", s + 1);
                gfx_draw_text_right(304, cur_y + 5, &FONT_UI_11B, tag, RAMP_ACCENT_BLUE);

                if (s == 0) {
                    gfx_draw_text(16, cur_y + 5, &FONT_UI_11B, "Find reactant orders (log ratio)", RAMP_INK_BLUE);
                    char sub[96];
                    snprintf(sub, sizeof(sub), "Compare trials (%d,%d) " SYM_ARROW " Overall = %.2g",
                             ir->pair_r1[0] + 1, ir->pair_r2[0] + 1, ir->overall_order);
                    gfx_draw_text(16, cur_y + 19, &FONT_UI_11, sub, RAMP_MUTED_BLUE);
                    char om[80];
                    if (ir->num_reactants == 1)
                        snprintf(om, sizeof(om), "~n~_{A} = %.2g   (~raw~ %.2f)", ir->order[0], ir->raw_order[0]);
                    else if (ir->num_reactants == 2)
                        snprintf(om, sizeof(om), "~n~_{A} = %.2g,  ~m~_{B} = %.2g,  ~n~+~m~ = %.2g",
                                 ir->order[0], ir->order[1], ir->overall_order);
                    else
                        snprintf(om, sizeof(om), "~n~=%.2g, ~m~=%.2g, ~p~=%.2g (Total %.2g)",
                                 ir->order[0], ir->order[1], ir->order[2], ir->overall_order);
                    math_draw_center(160, cur_y + 40, om, 14, RAMP_INK_BLUE, COL_INK);
                } else if (s == 1) {
                    gfx_draw_text(16, cur_y + 5, &FONT_UI_11B, "Solve rate constant k (mean)", RAMP_INK_BLUE);
                    char sub[80];
                    snprintf(sub, sizeof(sub), "Trial 1 k=%.3g, Trial 2 k=%.3g", ir->k_trial[0], ir->k_trial[1]);
                    gfx_draw_text(16, cur_y + 19, &FONT_UI_11, sub, RAMP_MUTED_BLUE);
                    char kb[40], ku[40], kl[90];
                    fmt_sci_math(ir->k_mean, kb, sizeof(kb), 3);
                    kinetics_format_k_units(ir->overall_order, "s", ku, sizeof(ku));
                    snprintf(kl, sizeof(kl), "~k~ = %s %s", kb, ku);
                    math_draw_center(160, cur_y + 39, kl, 14, RAMP_INK_BLUE, COL_INK);
                } else {
                    gfx_draw_text(16, cur_y + 5, &FONT_UI_11B, "Predict rate (ENTER to edit [ ])", RAMP_INK_BLUE);
                    char sub[80];
                    if (ir->num_reactants == 1) snprintf(sub, sizeof(sub), "At [A]=%.3g M", ir->pred_conc[0]);
                    else if (ir->num_reactants == 2) snprintf(sub, sizeof(sub), "At [A]=%.3g, [B]=%.3g M", ir->pred_conc[0], ir->pred_conc[1]);
                    else snprintf(sub, sizeof(sub), "At [A]=%.2g, [B]=%.2g, [C]=%.2g", ir->pred_conc[0], ir->pred_conc[1], ir->pred_conc[2]);
                    gfx_draw_text(16, cur_y + 19, &FONT_UI_11, sub, RAMP_MUTED_BLUE);
                    char pb[40], pl[72];
                    fmt_sci_math(ir->pred_rate, pb, sizeof(pb), 3);
                    snprintf(pl, sizeof(pl), "~Rate~ = %s \\f{mol}{L" SYM_DOT "s}", pb);
                    math_draw_center(160, cur_y + 39, pl, 14, RAMP_ACCENT_BLUE, COL_ACCENT);
                }
                cur_y += 70;
            } else {
                char nb[4]; snprintf(nb, sizeof(nb), "%d", s + 1);
                gfx_draw_text(12, cur_y + 6, &FONT_UI_11, nb, RAMP_MUTED_WHITE);
                if (s == 0) {
                    gfx_draw_text(24, cur_y + 6, &FONT_UI_11B, "Reactant orders", RAMP_INK_WHITE);
                    char ov[32]; snprintf(ov, sizeof(ov), "Overall = %.2g", ir->overall_order);
                    gfx_draw_text_right(304, cur_y + 6, &FONT_UI_11, ov, RAMP_MUTED_WHITE);
                } else if (s == 1) {
                    gfx_draw_text(24, cur_y + 6, &FONT_UI_11B, "Mean rate constant", RAMP_INK_WHITE);
                    char kb[40]; fmt_sci_math(ir->k_mean, kb, sizeof(kb), 3);
                    math_draw_right(304, cur_y + 5, kb, 14, RAMP_INK_WHITE, COL_INK);
                } else {
                    gfx_draw_text(24, cur_y + 6, &FONT_UI_11B, "Predicted rate", RAMP_INK_WHITE);
                    char pb[40]; fmt_sci_math(ir->pred_rate, pb, sizeof(pb), 3);
                    math_draw_right(304, cur_y + 5, pb, 14, RAMP_INK_WHITE, COL_INK);
                }
                gfx_hline(14, cur_y + 26, 292, COL_BORDER);
                cur_y += 28;
            }
        }
        ui_draw_keycap(10, 196, "UP/DN", "steps");
        ui_draw_keycap(104, 196, "ENTER", "predict");
        ui_draw_keycap(206, 196, "CLEAR", "table");
    }
    ui_draw_bottom_tabs(1);
}

static void draw_mini_plot(int x, int y, int w, int h, const double *tx, const double *vy,
                           int n, const LinReg *reg, const char *title, bool winner) {
    gfx_fill_rrect(x, y, w, h, 3, winner ? COL_GREEN_BG : COL_WHITE);
    gfx_draw_rrect(x, y, w, h, 3, winner ? COL_GREEN : COL_BORDER);
    gfx_draw_text_center(x + w / 2, y + 3, &FONT_UI_11B, title,
                         winner ? RAMP_GREEN_BG : RAMP_INK_WHITE);

    int px = x + 8, py = y + 19, pw = w - 16, ph = h - 38;
    gfx_vline(px, py, ph, COL_MUTED);
    gfx_hline(px, py + ph, pw, COL_MUTED);

    double xmin = tx[0], xmax = tx[0], ymin = vy[0], ymax = vy[0];
    for (int i = 1; i < n; i++) {
        if (tx[i] < xmin) xmin = tx[i];
        if (tx[i] > xmax) xmax = tx[i];
        if (vy[i] < ymin) ymin = vy[i];
        if (vy[i] > ymax) ymax = vy[i];
    }
    if (xmax - xmin < 1e-12) xmax = xmin + 1.0;
    if (ymax - ymin < 1e-12) ymax = ymin + 1.0;
    double pad_y = (ymax - ymin) * 0.12;
    ymin -= pad_y; ymax += pad_y;

    double ly0 = reg->slope * xmin + reg->intercept;
    double ly1 = reg->slope * xmax + reg->intercept;
    int gx0 = px + 2, gx1 = px + pw - 2;
    int gy0 = py + ph - (int)((ly0 - ymin) / (ymax - ymin) * (double)(ph - 2));
    int gy1 = py + ph - (int)((ly1 - ymin) / (ymax - ymin) * (double)(ph - 2));
    if (gy0 < py) gy0 = py;
    if (gy0 > py + ph) gy0 = py + ph;
    if (gy1 < py) gy1 = py;
    if (gy1 > py + ph) gy1 = py + ph;
    gfx_line(gx0, gy0, gx1, gy1, winner ? COL_GREEN : COL_ACCENT);

    for (int i = 0; i < n; i++) {
        int dx = px + 2 + (int)((tx[i] - xmin) / (xmax - xmin) * (double)(pw - 4));
        int dy = py + ph - (int)((vy[i] - ymin) / (ymax - ymin) * (double)(ph - 2));
        gfx_fill_circle(dx, dy, 2, COL_INK);
    }

    char r2s[24];
    snprintf(r2s, sizeof(r2s), "R2=%.4f", reg->r2);
    gfx_draw_text_center(x + w / 2, y + h - 15, &FONT_UI_11, r2s,
                         winner ? RAMP_GREEN_BG : RAMP_MUTED_WHITE);
}

static void render_integrated(const AppState *app) {
    if (app->sub_mode == 0) {
        const IntegratedTableState *it = &app->int_table;
        ui_draw_topbar("Integrated Law", "3-PLOT LINEARITY");

        double y0[MAX_INT_ROWS], y1[MAX_INT_ROWS], y2[MAX_INT_ROWS];
        for (int i = 0; i < it->num_rows; i++) {
            y0[i] = it->c[i]; y1[i] = log(it->c[i]); y2[i] = 1.0 / it->c[i];
        }
        draw_mini_plot(8,   26, 98, 102, it->t, y0, it->num_rows, &it->reg[0], "[A] vs t",   it->best_order == 0);
        draw_mini_plot(111, 26, 98, 102, it->t, y1, it->num_rows, &it->reg[1], "ln[A] vs t", it->best_order == 1);
        draw_mini_plot(214, 26, 98, 102, it->t, y2, it->num_rows, &it->reg[2], "1/[A] vs t", it->best_order == 2);

        gfx_fill_rrect(8, 134, 304, 56, 4, COL_BLUE_BG);
        const char *onames[3] = { "ZERO ORDER (n = 0)", "FIRST ORDER (n = 1)", "SECOND ORDER (n = 2)" };
        gfx_draw_text(16, 140, &FONT_UI_11B, onames[it->best_order], RAMP_ACCENT_BLUE);
        char kb[40], ku[40], kl[90];
        fmt_sci_math(it->k, kb, sizeof(kb), 3);
        kinetics_format_k_units((double)it->best_order, "s", ku, sizeof(ku));
        snprintf(kl, sizeof(kl), "~k~ = %s %s", kb, ku);
        math_draw(16, 158, kl, 14, RAMP_INK_BLUE, COL_INK);

        char slp[48]; snprintf(slp, sizeof(slp), "slope = %s~k~", it->best_order == 2 ? "+" : SYM_MINUS);
        math_draw_right(302, 140, slp, 14, RAMP_MUTED_BLUE, COL_MUTED);
        ui_draw_keycap(10, 196, "APPS", "edit data");
        ui_draw_keycap(118, 196, "MODE", "eq/pseudo");
    } else if (app->sub_mode == 1) {
        const IntegratedEqState *eq = &app->int_eq;
        char st[24]; snprintf(st, sizeof(st), "ORDER %d " SYM_DOT " SOLVE", eq->order);
        ui_draw_topbar("Integrated Law", st);

        const char *fstr = (eq->order == 0) ? "[A]_{~t~} = " SYM_MINUS "~kt~ + [A]_{0}"
                         : (eq->order == 1) ? "ln\\f{[A]_{0}}{[A]_{~t~}} = ~kt~"
                         : (eq->order == 2) ? "\\f{1}{[A]_{~t~}} " SYM_MINUS " \\f{1}{[A]_{0}} = ~kt~"
                                            : "\\f{1}{2[A]_{~t~}^{2}} " SYM_MINUS " \\f{1}{2[A]_{0}^{2}} = ~kt~";
        gfx_fill_rrect(10, 26, 300, 42, 4, COL_WHITE);
        gfx_draw_rrect(10, 26, 300, 42, 4, COL_BORDER);
        math_draw_center(160, 38, fstr, 14, RAMP_INK_WHITE, COL_INK);

        const char *flab[4] = { "[A]0 (initial)", "[A]t (at time t)", "Rate const k", "Time t" };
        double fval[4] = { eq->A0, eq->At, eq->k, eq->t };
        for (int i = 0; i < 4; i++) {
            int y = 74 + i * 20;
            bool sel = (app->cursor_r == i);
            bool unk = (eq->unknown == i);
            if (sel) gfx_fill_rrect(10, y, 300, 18, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 2, &FONT_UI_11B, flab[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            if (unk) ui_draw_pill(145, y + 1, "SOLVED", COL_GREEN_BG, RAMP_GREEN_BG);
            char vb[40]; fmt_sci_math(fval[i], vb, sizeof(vb), 3);
            math_draw_right(302, y + 1, vb, 14, unk ? RAMP_GREEN_WHITE : RAMP_INK_WHITE, COL_INK);
        }

        gfx_fill_rrect(10, 158, 300, 32, 3, COL_GRAY_BG);
        char hl[96];
        snprintf(hl, sizeof(hl), "t1/2: 1st=%.3g  2nd=%.3g  3rd=%.3g (%.1f%%)",
                 eq->t_half_1, eq->t_half_2, eq->t_half_3, eq->pct_remaining);
        gfx_draw_text_center(160, 168, &FONT_UI_11, hl, RAMP_INK_GRAY);

        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(86, 196, "APPS", "solve");
        ui_draw_keycap(166, 196, "L/R", "order");
        ui_draw_keycap(236, 196, "MODE", "next");
    } else if (app->sub_mode == 2) {
        const IntegratedTableState *it = &app->int_table;
        char st[24]; snprintf(st, sizeof(st), "%d DATA ROWS", it->num_rows);
        ui_draw_topbar("Integrated Data", st);

        int y0 = 26;
        gfx_fill_rect(10, y0, 300, 18, COL_CHROME);
        gfx_draw_rect(10, y0, 300, 18, COL_BORDER);
        gfx_draw_text(22, y0 + 2, &FONT_UI_11B, "Time t", RAMP_INK_CHROME);
        gfx_draw_text(98, y0 + 2, &FONT_UI_11B, "[A]", RAMP_INK_CHROME);
        gfx_draw_text(172, y0 + 2, &FONT_UI_11B, "ln[A]", RAMP_MUTED_CHROME);
        gfx_draw_text(244, y0 + 2, &FONT_UI_11B, "1/[A]", RAMP_MUTED_CHROME);

        int vis = it->num_rows < 7 ? it->num_rows : 7;
        for (int r = 0; r < vis; r++) {
            int ry = y0 + 18 + r * 20;
            gfx_draw_rect(10, ry, 300, 20, COL_BORDER);
            char b0[16], b1[16], b2[16], b3[16];
            fmt_compact(it->t[r], b0, sizeof(b0));
            fmt_compact(it->c[r], b1, sizeof(b1));
            fmt_compact(log(it->c[r]), b2, sizeof(b2));
            fmt_compact(1.0 / it->c[r], b3, sizeof(b3));
            if (app->cursor_r == r && app->cursor_c == 0) gfx_fill_rect(11, ry + 1, 74, 18, COL_BLUE_BG);
            if (app->cursor_r == r && app->cursor_c == 1) gfx_fill_rect(86, ry + 1, 74, 18, COL_BLUE_BG);
            gfx_draw_text(22, ry + 3, &FONT_UI_11, b0, RAMP_INK_WHITE);
            gfx_draw_text(98, ry + 3, &FONT_UI_11, b1, RAMP_INK_WHITE);
            gfx_draw_text(172, ry + 3, &FONT_UI_11, b2, RAMP_MUTED_WHITE);
            gfx_draw_text(244, ry + 3, &FONT_UI_11, b3, RAMP_MUTED_WHITE);
        }
        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(92, 196, "+/-", "rows");
        ui_draw_keycap(178, 196, "MODE", "plots");
    } else {
        ui_draw_topbar("Pseudo-Order", "k' " SYM_ARROW " TRUE k");
        double k_true = kinetics_pseudo_k(app->pseudo_kp, app->pseudo_excess, app->pseudo_order);
        gfx_fill_rrect(10, 28, 300, 56, 4, COL_BLUE_BG);
        math_draw_center(160, 36, "~k~ = \\f{~k~" SYM_PRIME "}{[B]_{0}^{~m~}}", 14, RAMP_MUTED_BLUE, COL_MUTED);
        char kb[40], kl[64];
        fmt_sci_math(k_true, kb, sizeof(kb), 3);
        snprintf(kl, sizeof(kl), "True ~k~ = %s", kb);
        math_draw_center(160, 62, kl, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        const char *pl[3] = { "Pseudo k' (-slope)", "Excess [B]0 (M)", "Order m of B" };
        double pv[3] = { app->pseudo_kp, app->pseudo_excess, app->pseudo_order };
        for (int i = 0; i < 3; i++) {
            int y = 96 + i * 26;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(10, y, 300, 22, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 4, &FONT_UI_11B, pl[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            char vb[24]; fmt_compact(pv[i], vb, sizeof(vb));
            gfx_draw_text_right(302, y + 4, &FONT_UI_11B, vb, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }
        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(106, 196, "MODE", "plots");
    }
    ui_draw_bottom_tabs(2);
}

static void render_mech(const AppState *app) {
    const MechState *m = &app->mech;
    char st[28]; snprintf(st, sizeof(st), "%d STEPS " SYM_DOT " CUSTOM", m->num_steps);
    ui_draw_topbar("Mechanisms", st);

    gfx_fill_rrect(10, 25, 300, 74, 4, COL_WHITE);
    gfx_draw_rrect(10, 25, 300, 74, 4, COL_BORDER);
    for (int s = 0; s < m->num_steps; s++) {
        const MechStep *st_p = &m->steps[s];
        int y = 29 + s * 22;
        bool sel = (app->cursor_r == s);
        if (sel) gfx_fill_rrect(13, y - 1, 294, 20, 3, COL_BLUE_BG);

        char line[80] = "";
        for (int i = 0; i < st_p->n_react; i++) {
            if (i > 0) strcat(line, " + ");
            char t[20];
            if (st_p->react[i].coeff > 1) snprintf(t, sizeof(t), "%d%s", st_p->react[i].coeff, st_p->react[i].formula);
            else snprintf(t, sizeof(t), "%s", st_p->react[i].formula);
            strcat(line, t);
        }
        strcat(line, st_p->speed == STEP_FAST_EQ ? " " SYM_EQUIL " " : " " SYM_ARROW " ");
        for (int i = 0; i < st_p->n_prod; i++) {
            if (i > 0) strcat(line, " + ");
            char t[20];
            if (st_p->prod[i].coeff > 1) snprintf(t, sizeof(t), "%d%s", st_p->prod[i].coeff, st_p->prod[i].formula);
            else snprintf(t, sizeof(t), "%s", st_p->prod[i].formula);
            strcat(line, t);
        }
        gfx_draw_text(18, y + 2, &FONT_UI_11B, line, sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
        const char *tag = (st_p->speed == STEP_SLOW) ? "SLOW" : (st_p->speed == STEP_FAST_EQ ? "FAST EQ" : "FAST");
        uint8_t bg = (st_p->speed == STEP_SLOW) ? COL_RED_BG : COL_GRAY_BG;
        uint8_t rp = (st_p->speed == STEP_SLOW) ? RAMP_RED_BG : RAMP_MUTED_WHITE;
        ui_draw_pill(248, y, tag, bg, rp);
    }

    gfx_fill_rrect(10, 102, 300, 61, 4, COL_BLUE_BG);
    gfx_draw_text(18, 105, &FONT_UI_11B, "Overall:", RAMP_MUTED_BLUE);
    math_draw(72, 104, m->overall_eq, 14, RAMP_INK_BLUE, COL_INK);

    gfx_draw_text(18, 128, &FONT_UI_11B, "Derived:", RAMP_ACCENT_BLUE);
    math_draw(72, 127, m->rate_law_math, 14, RAMP_INK_BLUE, COL_INK);
    if (m->elim_step_math[0]) {
        gfx_draw_text(18, 148, &FONT_UI_11, m->elim_step_math, RAMP_MUTED_BLUE);
    } else {
        char ml[48];
        snprintf(ml, sizeof(ml), "RDS = Step %d (%s)", m->slow_step_idx + 1, m->molecularity[m->slow_step_idx]);
        gfx_draw_text(18, 148, &FONT_UI_11, ml, RAMP_MUTED_BLUE);
    }

    int bx = 10;
    for (int i = 0; i < m->n_species; i++) {
        if (m->is_intermediate[i]) {
            char b[28]; snprintf(b, sizeof(b), "Int: %s", m->species_name[i]);
            ui_draw_pill(bx, 168, b, COL_AMBER_BG, RAMP_AMBER_BG);
            bx += gfx_text_width(&FONT_UI_11B, b) + 14;
        }
        if (m->is_catalyst[i]) {
            char b[28]; snprintf(b, sizeof(b), "Cat: %s", m->species_name[i]);
            ui_draw_pill(bx, 168, b, COL_GREEN_BG, RAMP_GREEN_BG);
            bx += gfx_text_width(&FONT_UI_11B, b) + 14;
        }
    }

    ui_draw_keycap(10, 196, "ENTER", "edit step");
    ui_draw_keycap(116, 196, "L/R", "speed");
    ui_draw_keycap(196, 196, "+/-", "2/3 stp");
    ui_draw_bottom_tabs(3);
}

static void render_arrhenius(const AppState *app) {
    if (app->sub_mode == 0) {
        const ArrhTwoPtState *a = &app->arrh2;
        ui_draw_topbar("Arrhenius", app->arrh_celsius ? "2-PT (" SYM_DEG "C)" : "2-PT (K)");

        gfx_fill_rrect(10, 26, 300, 42, 4, COL_WHITE);
        gfx_draw_rrect(10, 26, 300, 42, 4, COL_BORDER);
        math_draw_center(160, 38,
                         "ln\\f{~k~_{2}}{~k~_{1}} = \\f{~E~_{a}}{~R~}(\\f{1}{~T~_{1}} " SYM_MINUS " \\f{1}{~T~_{2}})",
                         14, RAMP_INK_WHITE, COL_INK);

        const char *labs[5] = { "Ea (kJ/mol)", "k2", app->arrh_celsius ? "T2 (C)" : "T2 (K)",
                                "k1", app->arrh_celsius ? "T1 (C)" : "T1 (K)" };
        double vals[5] = { a->Ea_kj, a->k2, app->arrh_celsius ? (a->T2 - 273.15) : a->T2,
                           a->k1, app->arrh_celsius ? (a->T1 - 273.15) : a->T1 };
        for (int i = 0; i < 5; i++) {
            int y = 74 + i * 22;
            bool sel = (app->cursor_r == i);
            bool unk = (a->unknown == i);
            if (sel) gfx_fill_rrect(10, y, 300, 20, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 3, &FONT_UI_11B, labs[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            if (unk) ui_draw_pill(116, y + 2, "SOLVED", COL_GREEN_BG, RAMP_GREEN_BG);
            char vb[40]; fmt_sci_math(vals[i], vb, sizeof(vb), 3);
            math_draw_right(302, y + 2, vb, 14, unk ? RAMP_GREEN_WHITE : RAMP_INK_WHITE, COL_INK);
        }
        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(84, 196, "APPS", "solve");
        ui_draw_keycap(158, 196, "L/R", "C/K");
        ui_draw_keycap(222, 196, "MODE", "plot");
    } else if (app->sub_mode == 1) {
        const ArrhTableState *at = &app->arrh_tbl;
        ui_draw_topbar("Arrhenius Plot", "ln k vs 1/T");

        double ix[MAX_ARRH_ROWS], ly[MAX_ARRH_ROWS];
        for (int i = 0; i < at->num_rows; i++) { ix[i] = 1.0 / at->T[i]; ly[i] = log(at->k[i]); }
        draw_mini_plot(10, 26, 146, 114, ix, ly, at->num_rows, &at->reg, "ln(k) vs 1/T", true);

        gfx_fill_rrect(164, 26, 146, 114, 4, COL_BLUE_BG);
        gfx_draw_text(172, 32, &FONT_UI_11B, "Linear Fit Results", RAMP_ACCENT_BLUE);
        char e1[48], a1[48], k1[48];
        snprintf(e1, sizeof(e1), "~E~_{a} = %.1f kJ", at->Ea_kj);
        math_draw(172, 52, e1, 14, RAMP_INK_BLUE, COL_INK);
        fmt_sci_math(at->A_factor, a1, sizeof(a1), 2);
        char al[56]; snprintf(al, sizeof(al), "~A~ = %s", a1);
        math_draw(172, 76, al, 14, RAMP_INK_BLUE, COL_INK);
        fmt_sci_math(at->eval_k, k1, sizeof(k1), 2);
        char kl[96]; snprintf(kl, sizeof(kl), "~k~(%.0fK) = %s", at->eval_T, k1);
        math_draw(172, 100, kl, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        gfx_fill_rrect(10, 146, 300, 44, 3, COL_GRAY_BG);
        char tr[96];
        snprintf(tr, sizeof(tr), "Pts: %d  (T1=%.0fK, k1=%.2e)  slope=-Ea/R",
                 at->num_rows, at->T[0], at->k[0]);
        gfx_draw_text_center(160, 152, &FONT_UI_11, tr, RAMP_INK_GRAY);
        gfx_draw_text_center(160, 168, &FONT_UI_11, "ENTER: edit (T,k) pts   APPS: eval new T", RAMP_MUTED_WHITE);

        ui_draw_keycap(10, 196, "ENTER", "edit pts");
        ui_draw_keycap(112, 196, "APPS", "eval T");
        ui_draw_keycap(206, 196, "MODE", "energy");
    } else {
        const CatalystState *c = &app->cat;
        ui_draw_topbar("Energy Profile", c->delta_E_kj < 0 ? "EXOTHERMIC" : "ENDOTHERMIC");

        int px = 14, py = 28, pw = 292, ph = 112;
        gfx_fill_rrect(px, py, pw, ph, 4, COL_WHITE);
        gfx_draw_rrect(px, py, pw, ph, 4, COL_BORDER);

        gfx_vline(px + 20, py + 10, ph - 26, COL_INK);
        gfx_hline(px + 20, py + ph - 16, pw - 32, COL_INK);
        gfx_draw_text(px + 6, py + 12, &FONT_UI_11B, "E", RAMP_MUTED_WHITE);
        gfx_draw_text_right(px + pw - 14, py + ph - 14, &FONT_UI_11, "Rxn Progress", RAMP_MUTED_WHITE);

        int y_r = (c->delta_E_kj < 0) ? (py + 62) : (py + 82);
        int y_p = (c->delta_E_kj < 0) ? (py + 88) : (py + 58);
        int y_pk_un  = py + 18;
        int y_pk_cat = py + 40;

        int prev_x = px + 28, prev_yu = y_r, prev_yc = y_r;
        for (int i = 1; i <= 40; i++) {
            double u = (double)i / 40.0;
            int gx = px + 28 + (int)(u * 244.0);
            double base = (double)y_r + (double)(y_p - y_r) / (1.0 + exp(-12.0 * (u - 0.5)));
            double hump = exp(-28.0 * (u - 0.5) * (u - 0.5));
            int yu = (int)(base - hump * ((double)y_r - (double)y_pk_un));
            int yc = (int)(base - hump * ((double)y_r - (double)y_pk_cat));
            gfx_line(prev_x, prev_yu, gx, yu, COL_RED);
            if ((i & 1) == 0) gfx_line(prev_x, prev_yc, gx, yc, COL_GREEN);
            prev_x = gx; prev_yu = yu; prev_yc = yc;
        }
        gfx_dashed_line(px + 28, y_r, px + 266, y_r, COL_BORDER);
        gfx_draw_text(px + 30, y_r - 13, &FONT_UI_11B, "R", RAMP_INK_WHITE);
        gfx_draw_text(px + 258, y_p - 13, &FONT_UI_11B, "P", RAMP_INK_WHITE);
        gfx_draw_text(px + 158, y_pk_un - 2, &FONT_UI_11B, "Uncat", RAMP_RED_WHITE);
        gfx_draw_text(px + 158, y_pk_cat - 2, &FONT_UI_11B, "Cat", RAMP_GREEN_WHITE);

        gfx_fill_rrect(14, 146, 292, 44, 4, COL_BLUE_BG);
        char l1[80], l2[80], sb[36];
        snprintf(l1, sizeof(l1), "~E~_{a,fwd}=%.0f  ~E~_{a,rev}=%.0f  " SYM_DELTA "~E~=%.0f kJ",
                 c->Ea_uncat_kj, c->Ea_rev_kj, c->delta_E_kj);
        fmt_sci_math(c->speedup_ratio, sb, sizeof(sb), 3);
        snprintf(l2, sizeof(l2), "~k~_{cat}/~k~_{un} = %s  (at %.0f K)", sb, c->T_kelvin);
        math_draw_center(160, 150, l1, 14, RAMP_INK_BLUE, COL_INK);
        math_draw_center(160, 169, l2, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        ui_draw_keycap(10, 196, "ENTER", "edit Ea/dE");
        ui_draw_keycap(126, 196, "MODE", "2-pt solver");
    }
    ui_draw_bottom_tabs(4);
}

static void render_summary(const AppState *app) {
    char pg[24]; snprintf(pg, sizeof(pg), "GUIDE %d / 5", app->ref_page + 1);
    ui_draw_topbar("Ref & Graphs", pg);

    if (app->ref_page == 0) {
        int x0 = 6, y0 = 26;
        gfx_fill_rect(x0, y0, 308, 18, COL_CHROME);
        gfx_draw_rect(x0, y0, 308, 18, COL_BORDER);
        gfx_draw_text(12, y0 + 2, &FONT_UI_11B, "Ord", RAMP_INK_CHROME);
        gfx_draw_text(44, y0 + 2, &FONT_UI_11B, "Integrated Law", RAMP_INK_CHROME);
        gfx_draw_text(168, y0 + 2, &FONT_UI_11B, "Plot (m)", RAMP_INK_CHROME);
        gfx_draw_text(238, y0 + 2, &FONT_UI_11B, "Half-Life", RAMP_INK_CHROME);

        const char *ord[3]  = { "0", "1", "2" };
        const char *eqs[3]  = { "[A] = " SYM_MINUS "~kt~ + [A]_{0}",
                                "ln[A] = " SYM_MINUS "~kt~ + ln[A]_{0}",
                                "\\f{1}{[A]} = ~kt~ + \\f{1}{[A]_{0}}" };
        const char *plts[3] = { "[A] (" SYM_MINUS "~k~)", "ln[A] (" SYM_MINUS "~k~)", "1/[A] (+~k~)" };
        const char *hls[3]  = { "\\f{[A]_{0}}{2~k~}", "\\f{0.693}{~k~}", "\\f{1}{~k~[A]_{0}}" };

        for (int i = 0; i < 3; i++) {
            int ry = y0 + 18 + i * 48;
            gfx_fill_rect(x0, ry, 308, 48, (i == 1) ? COL_BLUE_BG : COL_WHITE);
            gfx_draw_rect(x0, ry, 308, 48, COL_BORDER);
            uint8_t rmp = (i == 1) ? RAMP_INK_BLUE : RAMP_INK_WHITE;
            gfx_draw_text(16, ry + 16, &FONT_UI_13B, ord[i], rmp);
            math_draw(42, ry + 15, eqs[i], 14, rmp, COL_INK);
            math_draw(166, ry + 15, plts[i], 10, rmp, COL_INK);
            math_draw(242, ry + 15, hls[i], 14, rmp, COL_INK);
        }
    } else if (app->ref_page == 1) {
        int px = 14, py = 28, pw = 292, ph = 114;
        gfx_fill_rrect(px, py, pw, ph, 4, COL_WHITE);
        gfx_draw_rrect(px, py, pw, ph, 4, COL_BORDER);
        gfx_vline(px + 22, py + 10, ph - 26, COL_INK);
        gfx_hline(px + 22, py + ph - 16, pw - 34, COL_INK);
        gfx_draw_text(px + 4, py + 12, &FONT_UI_11B, "N", RAMP_MUTED_WHITE);
        gfx_draw_text_right(px + pw - 14, py + ph - 14, &FONT_UI_11, "Kinetic Energy", RAMP_MUTED_WHITE);

        int base_y = py + ph - 16;
        int ea_x = px + 186;
        gfx_dashed_line(ea_x, py + 12, ea_x, base_y, COL_INK);
        gfx_draw_text(ea_x + 3, py + 12, &FONT_UI_11B, "Ea", RAMP_INK_WHITE);

        int p1x = px + 22, p1y = base_y, p2x = px + 22, p2y = base_y;
        for (int i = 1; i <= 50; i++) {
            double u = (double)i / 50.0;
            int gx = px + 22 + (int)(u * 246.0);
            double f1 = 78.0 * pow(u / 0.28, 1.5) * exp(-1.5 * (u / 0.28 - 1.0));
            double f2 = 50.0 * pow(u / 0.44, 1.5) * exp(-1.5 * (u / 0.44 - 1.0));
            int y1 = base_y - (int)f1; if (y1 < py + 12) y1 = py + 12;
            int y2 = base_y - (int)f2; if (y2 < py + 12) y2 = py + 12;
            gfx_line(p1x, p1y, gx, y1, COL_ACCENT);
            gfx_line(p2x, p2y, gx, y2, COL_RED);
            if (gx >= ea_x) {
                gfx_vline(gx, y2, base_y - y2, COL_RED_BG);
                gfx_pixel(gx, y1, COL_ACCENT);
                gfx_pixel(gx, y2, COL_RED);
            }
            p1x = gx; p1y = y1; p2x = gx; p2y = y2;
        }
        gfx_draw_text(px + 72, py + 24, &FONT_UI_11B, "T1", RAMP_ACCENT_WHITE);
        gfx_draw_text(px + 130, py + 52, &FONT_UI_11B, "T2 > T1", RAMP_RED_WHITE);

        gfx_fill_rrect(14, 146, 292, 44, 4, COL_BLUE_BG);
        gfx_draw_text(22, 151, &FONT_UI_11B, "Higher T " SYM_ARROW " curve shifts right & flattens", RAMP_ACCENT_BLUE);
        gfx_draw_text(22, 168, &FONT_UI_11, "More collisions exceed Ea " SYM_ARROW " larger k & faster rate", RAMP_INK_BLUE);
    } else if (app->ref_page == 2) {
        int px = 14, py = 28, pw = 292, ph = 114;
        gfx_fill_rrect(px, py, pw, ph, 4, COL_WHITE);
        gfx_draw_rrect(px, py, pw, ph, 4, COL_BORDER);
        gfx_vline(px + 20, py + 10, ph - 26, COL_INK);
        gfx_hline(px + 20, py + ph - 16, pw - 32, COL_INK);
        gfx_draw_text(px + 6, py + 12, &FONT_UI_11B, "E", RAMP_MUTED_WHITE);
        gfx_draw_text_right(px + pw - 14, py + ph - 14, &FONT_UI_11, "Reaction Coordinate", RAMP_MUTED_WHITE);

        int prev_x = px + 26, prev_y = py + 72;
        for (int i = 1; i <= 50; i++) {
            double u = (double)i / 50.0;
            int gx = px + 26 + (int)(u * 246.0);
            double h1 = 26.0 * exp(-42.0 * (u - 0.30) * (u - 0.30));
            double h2 = 54.0 * exp(-36.0 * (u - 0.68) * (u - 0.68));
            double drift = 16.0 * u;
            int gy = py + 72 + (int)drift - (int)(h1 + h2);
            gfx_line(prev_x, prev_y, gx, gy, COL_ACCENT);
            prev_x = gx; prev_y = gy;
        }
        gfx_draw_text(px + 28, py + 58, &FONT_UI_11B, "R", RAMP_INK_WHITE);
        gfx_draw_text(px + 114, py + 62, &FONT_UI_11B, "Int", RAMP_AMBER_BG);
        gfx_draw_text(px + 168, py + 14, &FONT_UI_11B, "Slow (RDS)", RAMP_RED_WHITE);
        gfx_draw_text(px + 258, py + 74, &FONT_UI_11B, "P", RAMP_INK_WHITE);

        gfx_fill_rrect(14, 146, 292, 44, 4, COL_BLUE_BG);
        gfx_draw_text(22, 151, &FONT_UI_11B, "2 humps = 2-step mechanism  (valley = Interm.)", RAMP_ACCENT_BLUE);
        gfx_draw_text(22, 168, &FONT_UI_11, "Slowest step (RDS) always has the LARGEST Ea", RAMP_INK_BLUE);
    } else if (app->ref_page == 3) {
        gfx_fill_rrect(10, 28, 300, 162, 4, COL_WHITE);
        gfx_draw_rrect(10, 28, 300, 162, 4, COL_BORDER);
        gfx_draw_text(18, 34, &FONT_UI_11B, "How to Identify Order on the Exam:", RAMP_ACCENT_WHITE);
        gfx_draw_text(18, 54, &FONT_UI_11B, "Zero Order (n = 0):", RAMP_INK_WHITE);
        gfx_draw_text(18, 68, &FONT_UI_11, "  [A] vs t is straight (m = -k). t1/2 HALVES each time.", RAMP_MUTED_WHITE);
        gfx_draw_text(18, 88, &FONT_UI_11B, "First Order (n = 1):", RAMP_INK_WHITE);
        gfx_draw_text(18, 102, &FONT_UI_11, "  ln[A] vs t is straight (m = -k). t1/2 is CONSTANT.", RAMP_MUTED_WHITE);
        gfx_draw_text(18, 122, &FONT_UI_11B, "Second Order (n = 2):", RAMP_INK_WHITE);
        gfx_draw_text(18, 136, &FONT_UI_11, "  1/[A] vs t is straight (m = +k). t1/2 DOUBLES.", RAMP_MUTED_WHITE);
        gfx_fill_rrect(16, 156, 288, 28, 3, COL_BLUE_BG);
        gfx_draw_text(22, 163, &FONT_UI_11B, "Catalyst lowers Ea -> ln k vs 1/T slope LESS neg.", RAMP_ACCENT_BLUE);
    } else {
        gfx_fill_rrect(10, 28, 300, 162, 4, COL_WHITE);
        gfx_draw_rrect(10, 28, 300, 162, 4, COL_BORDER);
        gfx_draw_text(18, 34, &FONT_UI_11B, "Which Tab to Use on the Test:", RAMP_ACCENT_WHITE);
        gfx_draw_text(18, 54, &FONT_UI_11, "Y= Rates:   d[A]/dt, stoich ratios, units of k", RAMP_INK_WHITE);
        gfx_draw_text(18, 74, &FONT_UI_11, "WIN Init.R: Table of [A],[B] & Rate -> orders & k", RAMP_INK_WHITE);
        gfx_draw_text(18, 94, &FONT_UI_11, "ZM Integ:   Table of t & [A], half-life, % left, k'", RAMP_INK_WHITE);
        gfx_draw_text(18, 114, &FONT_UI_11, "TR Mech:    Custom 2/3-step mech -> Rate Law", RAMP_INK_WHITE);
        gfx_draw_text(18, 134, &FONT_UI_11, "GR Arrh:    k1/k2 & T1/T2, Ea, catalysts, dE", RAMP_INK_WHITE);
        gfx_fill_rrect(16, 156, 288, 28, 3, COL_GREEN_BG);
        gfx_draw_text(22, 163, &FONT_UI_11B, "Tip: R = 8.3145 J/(mol" SYM_DOT "K), K = C + 273.15", RAMP_GREEN_BG);
    }

    ui_draw_keycap(10, 196, "L/R", "next guide");
    ui_draw_keycap(136, 196, "CLEAR", "home");
    ui_draw_bottom_tabs(-1);
}

void app_render(const AppState *app) {
    gfx_clear(COL_WHITE);
    switch (app->screen) {
        case SCR_HOME:       render_home(app); break;
        case SCR_RATES:      render_rates(app); break;
        case SCR_INIT_RATES: render_init_rates(app); break;
        case SCR_INTEGRATED: render_integrated(app); break;
        case SCR_MECH:       render_mech(app); break;
        case SCR_ARRHENIUS:  render_arrhenius(app); break;
        case SCR_SUMMARY:    render_summary(app); break;
    }
    hal_swap_buffers();
}

bool app_handle_key(AppState *app, int k_in) {
    KeyCode k = (KeyCode)k_in;
    if (k == KEY_QUIT) return false;

    if (k == KEY_F1) {
        if (app->screen == SCR_RATES) app->sub_mode = (app->sub_mode + 1) % 3;
        else { app->screen = SCR_RATES; app->sub_mode = 0; }
        app->cursor_r = 0; return true;
    }
    if (k == KEY_F2) {
        if (app->screen == SCR_INIT_RATES) app->sub_mode ^= 1;
        else { app->screen = SCR_INIT_RATES; app->sub_mode = 0; }
        app->cursor_r = 0; app->cursor_c = 0; return true;
    }
    if (k == KEY_F3) {
        if (app->screen == SCR_INTEGRATED) app->sub_mode = (app->sub_mode + 1) % 4;
        else { app->screen = SCR_INTEGRATED; app->sub_mode = 0; }
        app->cursor_r = 0; return true;
    }
    if (k == KEY_F4) {
        app->screen = SCR_MECH; app->cursor_r = 0; return true;
    }
    if (k == KEY_F5) {
        if (app->screen == SCR_ARRHENIUS) app->sub_mode = (app->sub_mode + 1) % 3;
        else { app->screen = SCR_ARRHENIUS; app->sub_mode = 0; }
        app->cursor_r = 0; return true;
    }

    if (app->screen == SCR_HOME) {
        if (k == KEY_CLEAR) return false;
        if (k == KEY_LEFT  && app->home_sel % 3 > 0) app->home_sel--;
        if (k == KEY_RIGHT && app->home_sel % 3 < 2) app->home_sel++;
        if (k == KEY_UP    && app->home_sel >= 3) app->home_sel -= 3;
        if (k == KEY_DOWN  && app->home_sel < 3)  app->home_sel += 3;
        if (k >= KEY_1 && k <= KEY_6) { app->home_sel = (int)(k - KEY_1); k = KEY_ENTER; }
        if (k == KEY_ENTER) {
            app->screen = (ScreenId)(app->home_sel + 1);
            app->sub_mode = 0; app->cursor_r = 0; app->cursor_c = 0;
        }
        return true;
    }

    if (k == KEY_CLEAR) {
        if (app->screen == SCR_INIT_RATES && app->sub_mode == 1) { app->sub_mode = 0; return true; }
        app->screen = SCR_HOME;
        return true;
    }

    if (app->screen == SCR_RATES) {
        if (k == KEY_MODE) { app->sub_mode = (app->sub_mode + 1) % 3; app->cursor_r = 0; }
        else if (app->sub_mode == 0) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 4) app->cursor_r++;
            if (k == KEY_APPS) { app->rates.is_reactant = !app->rates.is_reactant; kinetics_rate_calc(&app->rates); }
            if (k == KEY_ENTER) {
                double *ptr[5] = { &app->rates.c1, &app->rates.c2, &app->rates.t1, &app->rates.t2, &app->rates.coeff };
                const char *pr[5] = { "Edit [A]1 (M)", "Edit [A]2 (M)", "Edit t1 (s)", "Edit t2 (s)", "Edit Coeff a" };
                if (ui_edit_double(pr[app->cursor_r], ptr[app->cursor_r])) kinetics_rate_calc(&app->rates);
            }
        } else if (app->sub_mode == 1) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 2) app->cursor_r++;
            if (k == KEY_ENTER) {
                if (ui_edit_double("Edit Coefficient", &app->stoich.coeffs[app->cursor_r]))
                    kinetics_stoich_rates(&app->stoich);
            }
            if (k == KEY_APPS) {
                app->stoich.known_idx = app->cursor_r;
                if (ui_edit_double("Known Species Rate", &app->stoich.known_rate))
                    kinetics_stoich_rates(&app->stoich);
            }
        } else {
            if (k == KEY_LEFT)  app->k_unit_order -= 0.5;
            if (k == KEY_RIGHT) app->k_unit_order += 0.5;
            if (k == KEY_ENTER) ui_edit_double("Overall Order n", &app->k_unit_order);
        }
    } else if (app->screen == SCR_INIT_RATES) {
        InitialRatesState *ir = &app->init_rates;
        if (app->sub_mode == 0) {
            int cols = ir->num_reactants + 1;
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < ir->num_rows - 1) app->cursor_r++;
            if (k == KEY_LEFT && app->cursor_c > 0) app->cursor_c--;
            if (k == KEY_RIGHT && app->cursor_c < cols - 1) app->cursor_c++;
            if (k == KEY_PLUS && ir->num_rows < MAX_IR_ROWS) {
                int nr = ir->num_rows++;
                for (int j = 0; j < MAX_IR_REACT; j++) ir->conc[nr][j] = ir->conc[nr - 1][j];
                ir->rate[nr] = ir->rate[nr - 1];
            }
            if (k == KEY_MINUS && ir->num_rows > 2) {
                ir->num_rows--;
                if (app->cursor_r >= ir->num_rows) app->cursor_r = ir->num_rows - 1;
            }
            if (k == KEY_MODE) {
                ir->num_reactants = (ir->num_reactants % 3) + 1;
                if (app->cursor_c > ir->num_reactants) app->cursor_c = ir->num_reactants;
            }
            if (k == KEY_ENTER) {
                double *target = (app->cursor_c < ir->num_reactants)
                                 ? &ir->conc[app->cursor_r][app->cursor_c]
                                 : &ir->rate[app->cursor_r];
                ui_edit_double("Edit Table Cell", target);
            }
            if (k == KEY_APPS) {
                kinetics_solve_initial_rates(ir);
                app->sub_mode = 1;
                app->step_idx = 0;
            }
        } else {
            if (k == KEY_UP && app->step_idx > 0) app->step_idx--;
            if (k == KEY_DOWN && app->step_idx < 2) app->step_idx++;
            if (k == KEY_ENTER) {
                const char *pr[3] = { "Predict at [A] (M)", "Predict at [B] (M)", "Predict at [C] (M)" };
                for (int j = 0; j < ir->num_reactants; j++) {
                    if (!ui_edit_double(pr[j], &ir->pred_conc[j])) break;
                }
                kinetics_solve_initial_rates(ir);
                app->step_idx = 2;
            }
        }
    } else if (app->screen == SCR_INTEGRATED) {
        if (k == KEY_MODE) { app->sub_mode = (app->sub_mode + 1) % 4; app->cursor_r = 0; }
        else if (app->sub_mode == 0 && k == KEY_APPS) { app->sub_mode = 2; app->cursor_r = 0; app->cursor_c = 0; }
        else if (app->sub_mode == 1) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 3) app->cursor_r++;
            if (k == KEY_LEFT)  { app->int_eq.order = (app->int_eq.order + 3) % 4; kinetics_integrated_solve(&app->int_eq); }
            if (k == KEY_RIGHT) { app->int_eq.order = (app->int_eq.order + 1) % 4; kinetics_integrated_solve(&app->int_eq); }
            if (k == KEY_APPS)  { app->int_eq.unknown = app->cursor_r; kinetics_integrated_solve(&app->int_eq); }
            if (k == KEY_ENTER) {
                double *ptrs[4] = { &app->int_eq.A0, &app->int_eq.At, &app->int_eq.k, &app->int_eq.t };
                if (ui_edit_double("Edit Value", ptrs[app->cursor_r])) kinetics_integrated_solve(&app->int_eq);
            }
        } else if (app->sub_mode == 2) {
            IntegratedTableState *it = &app->int_table;
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < it->num_rows - 1) app->cursor_r++;
            if (k == KEY_LEFT) app->cursor_c = 0;
            if (k == KEY_RIGHT) app->cursor_c = 1;
            if (k == KEY_PLUS && it->num_rows < MAX_INT_ROWS) {
                int r = it->num_rows++;
                it->t[r] = it->t[r - 1] + 2.0;
                it->c[r] = it->c[r - 1] * 0.5;
                kinetics_regress_order(it);
            }
            if (k == KEY_MINUS && it->num_rows > 3) {
                it->num_rows--;
                if (app->cursor_r >= it->num_rows) app->cursor_r = it->num_rows - 1;
                kinetics_regress_order(it);
            }
            if (k == KEY_ENTER) {
                double *tptr = (app->cursor_c == 0) ? &it->t[app->cursor_r] : &it->c[app->cursor_r];
                if (ui_edit_double(app->cursor_c == 0 ? "Edit Time t" : "Edit [A]", tptr))
                    kinetics_regress_order(it);
            }
        } else if (app->sub_mode == 3) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 2) app->cursor_r++;
            if (k == KEY_ENTER) {
                double *ptrs[3] = { &app->pseudo_kp, &app->pseudo_excess, &app->pseudo_order };
                ui_edit_double("Edit Pseudo Val", ptrs[app->cursor_r]);
            }
        }
    } else if (app->screen == SCR_MECH) {
        if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
        if (k == KEY_DOWN && app->cursor_r < app->mech.num_steps - 1) app->cursor_r++;
        if (k == KEY_LEFT || k == KEY_RIGHT) {
            int d = (k == KEY_RIGHT) ? 1 : -1;
            MechStep *st = &app->mech.steps[app->cursor_r];
            st->speed = (StepSpeed)(((int)st->speed + 3 + d) % 3);
            kinetics_analyze_mech(&app->mech);
        }
        if (k == KEY_PLUS && app->mech.num_steps < 3) {
            app->mech.num_steps = 3;
            MechStep *st = &app->mech.steps[2];
            st->speed = STEP_FAST; st->n_react = 1; st->n_prod = 1;
            strcpy(st->react[0].formula, "I1"); st->react[0].coeff = 1;
            strcpy(st->prod[0].formula, "C");   st->prod[0].coeff = 1;
            kinetics_analyze_mech(&app->mech);
        }
        if (k == KEY_MINUS && app->mech.num_steps > 2) {
            app->mech.num_steps = 2;
            if (app->cursor_r >= 2) app->cursor_r = 1;
            kinetics_analyze_mech(&app->mech);
        }
        if (k == KEY_ENTER) {
            ui_edit_mech_step(&app->mech.steps[app->cursor_r], app->cursor_r);
            kinetics_analyze_mech(&app->mech);
        }
        if (k == KEY_MODE) {
            app_load_mech_preset(app, (app->mech_preset + 1) % 5);
            app->cursor_r = 0;
        }
    } else if (app->screen == SCR_ARRHENIUS) {
        if (k == KEY_MODE) { app->sub_mode = (app->sub_mode + 1) % 3; app->cursor_r = 0; }
        else if (app->sub_mode == 0) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 4) app->cursor_r++;
            if (k == KEY_LEFT || k == KEY_RIGHT) app->arrh_celsius = !app->arrh_celsius;
            if (k == KEY_APPS) { app->arrh2.unknown = app->cursor_r; kinetics_arrhenius_2pt(&app->arrh2); }
            if (k == KEY_ENTER) {
                if (app->cursor_r == 2 || app->cursor_r == 4) {
                    double *tk = (app->cursor_r == 2) ? &app->arrh2.T2 : &app->arrh2.T1;
                    double disp = app->arrh_celsius ? (*tk - 273.15) : *tk;
                    if (ui_edit_double(app->arrh_celsius ? "Temp (deg C)" : "Temp (K)", &disp)) {
                        *tk = app->arrh_celsius ? (disp + 273.15) : disp;
                        kinetics_arrhenius_2pt(&app->arrh2);
                    }
                } else {
                    double *ptrs[5] = { &app->arrh2.Ea_kj, &app->arrh2.k2, &app->arrh2.T2, &app->arrh2.k1, &app->arrh2.T1 };
                    if (ui_edit_double("Edit Arrhenius Val", ptrs[app->cursor_r])) kinetics_arrhenius_2pt(&app->arrh2);
                }
            }
        } else if (app->sub_mode == 1) {
            ArrhTableState *at = &app->arrh_tbl;
            if (k == KEY_APPS) {
                if (ui_edit_double("Evaluate k at T (K)", &at->eval_T)) kinetics_arrhenius_regress(at);
            }
            if (k == KEY_ENTER) {
                for (int i = 0; i < at->num_rows; i++) {
                    char pt[24], pk[24];
                    snprintf(pt, sizeof(pt), "Pt %d Temp (K)", i + 1);
                    snprintf(pk, sizeof(pk), "Pt %d Rate k", i + 1);
                    if (!ui_edit_double(pt, &at->T[i])) break;
                    if (!ui_edit_double(pk, &at->k[i])) break;
                }
                kinetics_arrhenius_regress(at);
            }
        } else {
            if (k == KEY_ENTER) {
                if (ui_edit_double("Uncatalyzed Ea (kJ)", &app->cat.Ea_uncat_kj)) {
                    ui_edit_double("Catalyzed Ea (kJ)", &app->cat.Ea_cat_kj);
                    ui_edit_double("Delta E (kJ, -exo)", &app->cat.delta_E_kj);
                    ui_edit_double("Temp T (K)", &app->cat.T_kelvin);
                    kinetics_catalyst_calc(&app->cat);
                }
            }
        }
    } else if (app->screen == SCR_SUMMARY) {
        if (k == KEY_LEFT)  app->ref_page = (app->ref_page + 4) % 5;
        if (k == KEY_RIGHT) app->ref_page = (app->ref_page + 1) % 5;
    }
    return true;
}
