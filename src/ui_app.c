#include "ui_app.h"
#include "hal.h"
#include "gfx.h"
#include "math_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * Interactive Number Input Modal (Zero malloc, supports scientific notation)
 * ============================================================================ */
static bool ui_edit_double(const char *prompt, double *val) {
    char buf[24];
    fmt_compact(*val, buf, sizeof(buf));
    int len = (int)strlen(buf);
    bool first_key = true;

    for (;;) {
        /* Draw clean PlexMath modal card over center of screen */
        gfx_fill_rrect(36, 78, 248, 84, 4, COL_WHITE);
        gfx_draw_rrect(36, 78, 248, 84, 4, COL_ACCENT);
        gfx_fill_rect(37, 79, 246, 22, COL_BLUE_BG);
        gfx_draw_text(46, 83, &FONT_UI_11B, prompt, RAMP_ACCENT_BLUE);

        /* Input field */
        gfx_fill_rrect(48, 108, 224, 24, 3, COL_GRAY_BG);
        gfx_draw_rrect(48, 108, 224, 24, 3, COL_BORDER);
        int tw = gfx_draw_text(56, 113, &FONT_UI_13B, buf, RAMP_INK_GRAY);
        gfx_vline(56 + tw + 2, 112, 16, COL_ACCENT);

        ui_draw_keycap(48, 140, "ENTER", "save");
        ui_draw_keycap(130, 140, ",", "e (10^x)");
        ui_draw_keycap(214, 140, "CLR", "cancel");
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

/* ============================================================================
 * Default Initialization (Preloaded with exact examples from uploaded notes!)
 * ============================================================================ */
void app_load_mech_preset(AppState *app, int idx) {
    app->mech_preset = idx % 5;
    MechState *m = &app->mech;
    memset(m, 0, sizeof(*m));

    if (app->mech_preset == 0) {
        /* 12.6 Ex. 1: NO + Cl2 <-> NOCl2 (fast eq), NOCl2 + NO -> 2NOCl (slow) */
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
        /* 12.6 Ex. 3: O3 <-> O2 + O (fast eq), O3 + O -> 2O2 (slow) => k[O3]^2/[O2] */
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
        /* 12.6 Intro: 2NO2 -> NO3 + NO (slow), NO3 + CO -> NO2 + CO2 (fast) */
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
        /* Zumdahl Q80: Cl-catalyzed ozone destruction: O3 + Cl -> O2 + ClO (slow), ClO + O -> O2 + Cl (fast) */
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
        /* Zumdahl Q115: Cl2 <-> 2Cl (fast eq), Cl + CO <-> COCl (fast eq), COCl + Cl2 -> COCl2 + Cl (slow) */
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

    /* 12.1 Table 12.1: 2 NO2 -> 2 NO + O2 (0..50s) */
    app->rates.c1 = 0.0100; app->rates.c2 = 0.0079;
    app->rates.t1 = 0.0;    app->rates.t2 = 50.0;
    app->rates.coeff = 2.0; app->rates.is_reactant = true;
    kinetics_rate_calc(&app->rates);

    /* Stoich example: N2 + 3 H2 -> 2 NH3 */
    app->stoich.coeffs[0] = 1; app->stoich.is_product[0] = false;
    app->stoich.coeffs[1] = 3; app->stoich.is_product[1] = false;
    app->stoich.coeffs[2] = 2; app->stoich.is_product[2] = true;
    app->stoich.coeffs[3] = 0; app->stoich.is_product[3] = true;
    app->stoich.known_idx = 1; app->stoich.known_rate = 0.060;
    kinetics_stoich_rates(&app->stoich);

    /* 12.3 Ex. 4: aA + bB -> products (2nd in A, 1st in B, k = 0.153) */
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

    /* 12.4 Ex. 3: HI decomposition (2nd order) */
    app->int_table.num_rows = 4;
    double tt[4] = {0, 2, 4, 6};
    double cc[4] = {1.00, 0.50, 0.3333, 0.25};
    for (int i = 0; i < 4; i++) { app->int_table.t[i] = tt[i]; app->int_table.c[i] = cc[i]; }
    kinetics_regress_order(&app->int_table);

    /* 12.4 Ex. 1: N2O5 1st order, k=0.416/min, [A]0=0.160, t=4.0 min */
    app->int_eq.order = 1;
    app->int_eq.unknown = 1; /* solve [A]t */
    app->int_eq.A0 = 0.160;
    app->int_eq.At = 0.0303;
    app->int_eq.k  = 0.416;
    app->int_eq.t  = 4.0;
    kinetics_integrated_solve(&app->int_eq);

    /* 12.6 Mechanism preset 0 */
    app_load_mech_preset(app, 0);

    /* 12.7 Arrhenius 2-pt (Zumdahl Q73: k1=3.52e-7, T1=555K, T2=645K, Ea=186 kJ/mol) */
    app->arrh2.unknown = 1; /* solve k2 */
    app->arrh2.k1 = 3.52e-7;
    app->arrh2.T1 = 555.0;
    app->arrh2.T2 = 645.0;
    app->arrh2.Ea_kj = 186.0;
    kinetics_arrhenius_2pt(&app->arrh2);

    /* Arrhenius multi-pt table (Zumdahl Q71) */
    app->arrh_tbl.num_rows = 3;
    app->arrh_tbl.T[0] = 338; app->arrh_tbl.k[0] = 4.9e-3;
    app->arrh_tbl.T[1] = 318; app->arrh_tbl.k[1] = 5.0e-4;
    app->arrh_tbl.T[2] = 298; app->arrh_tbl.k[2] = 3.5e-5;
    app->arrh_tbl.eval_T = 308;
    kinetics_arrhenius_regress(&app->arrh_tbl);

    /* Catalyst & Energy Profile (Zumdahl Q69/Q79) */
    app->cat.Ea_uncat_kj = 125.0;
    app->cat.Ea_cat_kj   = 85.0;
    app->cat.delta_E_kj  = -216.0;
    app->cat.T_kelvin    = 298.0;
    kinetics_catalyst_calc(&app->cat);
}

/* ============================================================================
 * Screen 0: Home Icon Tile Menu (Minimalist 3x2 Grid + PlexMath Chrome)
 * ============================================================================ */
static void render_home(const AppState *app) {
    static const char * const TITLES[6] = {
        "Reaction Rates", "Initial Rates", "Integrated",
        "Mechanisms",     "Arrhenius",     "Chart 12.6"
    };
    static const char * const SUBS[6] = {
        "Avg, Stoich & k", "Table 1-3R Solver", "Plots, k & t1/2",
        "Slow & Interm.",  "Ea, T & Catalyst",  "Order Quick Ref"
    };

    ui_draw_topbar("PlexChem", "AP CHEM " SYM_DOT " CH. 12");

    for (int i = 0; i < 6; i++) {
        int col = i % 3;
        int row = i / 3;
        int x = 10 + col * 102;
        int y = 30 + row * 80;
        int w = 96, h = 72;
        bool sel = (app->home_sel == i);

        gfx_fill_rrect(x, y, w, h, 4, sel ? COL_BLUE_BG : COL_WHITE);
        gfx_draw_rrect(x, y, w, h, 4, sel ? COL_ACCENT : COL_BORDER);
        if (sel) gfx_draw_rrect(x + 1, y + 1, w - 2, h - 2, 3, COL_ACCENT);

        /* Centered minimalist icon badge */
        gfx_draw_icon(x + (w - 18) / 2, y + 11, (uint8_t)i,
                      sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);

        gfx_draw_text_center(x + w / 2, y + 35, &FONT_UI_11B, TITLES[i],
                             sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
        gfx_draw_text_center(x + w / 2, y + 51, &FONT_UI_11, SUBS[i],
                             sel ? RAMP_MUTED_BLUE : RAMP_MUTED_WHITE);
    }

    ui_draw_keycap(12, 196, "ENTER", "open module");
    ui_draw_keycap(142, 196, "1-6", "jump");
    ui_draw_keycap(214, 196, "CLEAR", "exit");
    ui_draw_bottom_tabs(-1);
}

/* ============================================================================
 * Screen 1: Module 1 — Reaction Rates & Stoichiometry
 * ============================================================================ */
static void render_rates(const AppState *app) {
    ui_draw_topbar("PlexChem", app->sub_mode == 0 ? "RATES " SYM_DOT " AVG" : "RATES " SYM_DOT " STOICH");

    if (app->sub_mode == 0) {
        /* Hero formula box (PlexMath style) */
        gfx_fill_rrect(10, 27, 300, 56, 4, COL_WHITE);
        gfx_draw_rrect(10, 27, 300, 56, 4, COL_BORDER);
        math_draw_center(160, 34,
                         app->rates.is_reactant
                             ? "~Rate~ = " SYM_MINUS "\\f{" SYM_DELTA "[A]}{~a~" SYM_DELTA "~t~}"
                             : "~Rate~ = +\\f{" SYM_DELTA "[C]}{~c~" SYM_DELTA "~t~}",
                         14, RAMP_MUTED_WHITE, COL_MUTED);
        gfx_hline(24, 55, 272, COL_BORDER);
        char rbuf[48], eqbuf[80];
        fmt_sci_math(app->rates.overall_rate, rbuf, sizeof(rbuf), 3);
        snprintf(eqbuf, sizeof(eqbuf), "~Rate~ = %s \\f{mol}{L" SYM_DOT "s}", rbuf);
        math_draw_center(160, 61, eqbuf, 14, RAMP_INK_WHITE, COL_INK);

        /* Editable parameter rows */
        const char *labels[5] = { "[A]1 (M)", "[A]2 (M)", "t1 (s)", "t2 (s)", "Coeff a" };
        double vals[5] = { app->rates.c1, app->rates.c2, app->rates.t1, app->rates.t2, app->rates.coeff };
        for (int i = 0; i < 5; i++) {
            int y = 90 + i * 20;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(10, y, 300, 18, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 2, &FONT_UI_11B, labels[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            char vb[32]; fmt_compact(vals[i], vb, sizeof(vb));
            gfx_draw_text_right(302, y + 2, &FONT_UI_11, vb, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }
        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(96, 196, "MODE", "stoich");
        ui_draw_keycap(196, 196, "APPS", app->rates.is_reactant ? "React" : "Prod");
    } else {
        /* Stoichiometry Rate Converter: aA + bB -> cC + dD */
        char hdr[64];
        snprintf(hdr, sizeof(hdr), "%.0fA + %.0fB " SYM_ARROW " %.0fC",
                 app->stoich.coeffs[0], app->stoich.coeffs[1], app->stoich.coeffs[2]);
        math_draw_center(160, 28, hdr, 14, RAMP_INK_WHITE, COL_INK);

        /* Active Overall Rate Card */
        gfx_fill_rrect(10, 50, 300, 34, 4, COL_BLUE_BG);
        gfx_draw_text(18, 55, &FONT_UI_11B, "Overall Reaction Rate", RAMP_INK_BLUE);
        char rb[40]; fmt_sci_math(app->stoich.overall_rate, rb, sizeof(rb), 3);
        math_draw_right(302, 57, rb, 14, RAMP_ACCENT_BLUE, COL_ACCENT);
        gfx_draw_text(18, 69, &FONT_UI_11, "mol/(L" SYM_DOT "s)  [all rates positive overall]", RAMP_MUTED_BLUE);

        const char *sp[3] = { "Species A (Reactant)", "Species B (Reactant)", "Species C (Product)" };
        for (int i = 0; i < 3; i++) {
            int y = 92 + i * 32;
            bool sel = (app->cursor_r == i);
            gfx_fill_rrect(10, y, 300, 28, 3, sel ? COL_GRAY_BG : COL_WHITE);
            gfx_draw_rrect(10, y, 300, 28, 3, sel ? COL_ACCENT : COL_BORDER);
            gfx_draw_text(18, y + 3, &FONT_UI_11B, sp[i], RAMP_INK_WHITE);
            char cb[32]; snprintf(cb, sizeof(cb), "coeff = %.0f", app->stoich.coeffs[i]);
            gfx_draw_text(18, y + 15, &FONT_UI_11, cb, RAMP_MUTED_WHITE);
            char sb[40]; fmt_sci_math(app->stoich.rates[i], sb, sizeof(sb), 3);
            math_draw_right(302, y + 6, sb, 14, RAMP_INK_WHITE, COL_INK);
        }
        ui_draw_keycap(10, 196, "ENTER", "edit coeff");
        ui_draw_keycap(124, 196, "APPS", "set rate");
        ui_draw_keycap(228, 196, "MODE", "avg");
    }
    ui_draw_bottom_tabs(0);
}

/* ============================================================================
 * Screen 2: Module 2 — Method of Initial Rates (Configurable Table + PlexMath Steps!)
 * ============================================================================ */
static void render_init_rates(const AppState *app) {
    const InitialRatesState *ir = &app->init_rates;
    if (app->sub_mode == 0) {
        /* Configurable Data Table View */
        char st[32];
        snprintf(st, sizeof(st), "%dR " SYM_TIMES " %d ROWS", ir->num_reactants, ir->num_rows);
        ui_draw_topbar("Initial Rates", st);

        /* Config pills */
        char p1[24], p2[24];
        snprintf(p1, sizeof(p1), "Reactants: %d", ir->num_reactants);
        snprintf(p2, sizeof(p2), "Rows: %d", ir->num_rows);
        ui_draw_pill(10, 25, p1, COL_BLUE_BG, RAMP_ACCENT_BLUE);
        ui_draw_pill(116, 25, p2, COL_GRAY_BG, RAMP_INK_GRAY);
        ui_draw_pill(200, 25, "APPS: Solve", COL_GREEN_BG, RAMP_GREEN_BG);

        /* Table Header */
        int cols = ir->num_reactants + 1; /* [A], ([B]), ([C]), Rate */
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

        /* Table Rows (scrolls if > 6 rows) */
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
        ui_draw_keycap(10, 196, "ENTER", "edit cell");
        ui_draw_keycap(118, 196, "+/-", "rows");
        ui_draw_keycap(196, 196, "MODE", "react#");
    } else {
        /* PlexMath Signature Steps View (Matches image_15b142.png!) */
        char st[24]; snprintf(st, sizeof(st), "STEPS %d / 3", app->step_idx + 1);
        ui_draw_topbar("PlexChem", st);

        /* Hero Rate Law in large STIX/Termes Math Serif (NO '^' carets!) */
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
            } else {
                snprintf(t, sizeof(t), "[%s]^{%.1f}", rn[j], o);
            }
            strncat(rl, t, sizeof(rl) - strlen(rl) - 1);
        }
        math_draw_center(160, 27, rl, 20, RAMP_INK_WHITE, COL_INK);

        /* Step 1 Card (Active Blue Highlight like PlexMath!) */
        int sy = 56;
        bool s0 = (app->step_idx == 0);
        if (s0) gfx_fill_rrect(8, sy, 304, 62, 4, COL_BLUE_BG);
        gfx_draw_text(16, sy + 5, &FONT_UI_11B, "Determine reactant orders", s0 ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        gfx_draw_text_right(304, sy + 5, &FONT_UI_11B, "STEP 1", s0 ? RAMP_ACCENT_BLUE : RAMP_MUTED_WHITE);
        char sub1[96];
        if (ir->num_reactants == 2)
            snprintf(sub1, sizeof(sub1), "Trials (%d,%d) & (%d,%d) " SYM_ARROW " Overall = %.1g",
                     ir->pair_r1[0]+1, ir->pair_r2[0]+1, ir->pair_r1[1]+1, ir->pair_r2[1]+1, ir->overall_order);
        else
            snprintf(sub1, sizeof(sub1), "Log-linear exact fit " SYM_ARROW " Overall order = %.1g", ir->overall_order);
        gfx_draw_text(16, sy + 19, &FONT_UI_11, sub1, s0 ? RAMP_MUTED_BLUE : RAMP_MUTED_WHITE);

        char ord_math[80];
        if (ir->num_reactants == 1)
            snprintf(ord_math, sizeof(ord_math), "~n~_{A} = %.2g   (~raw~ %.2f)", ir->order[0], ir->raw_order[0]);
        else if (ir->num_reactants == 2)
            snprintf(ord_math, sizeof(ord_math), "~n~_{A} = %.2g,  ~m~_{B} = %.2g,  ~n~+~m~ = %.2g",
                     ir->order[0], ir->order[1], ir->overall_order);
        else
            snprintf(ord_math, sizeof(ord_math), "~n~=%.2g, ~m~=%.2g, ~p~=%.2g  (Total %.2g)",
                     ir->order[0], ir->order[1], ir->order[2], ir->overall_order);
        math_draw_center(160, sy + 38, ord_math, 14, s0 ? RAMP_INK_BLUE : RAMP_INK_WHITE, COL_INK);

        /* Step 2 Row: Rate Constant k & Units */
        int y2 = 124;
        bool s1 = (app->step_idx == 1);
        if (s1) gfx_fill_rrect(8, y2, 304, 34, 4, COL_BLUE_BG);
        gfx_draw_text(10, y2 + 9, &FONT_UI_11, "2", RAMP_MUTED_WHITE);
        gfx_draw_text(22, y2 + 9, &FONT_UI_11B, "Mean rate constant", s1 ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        char kb[40], ku[40], kline[90];
        fmt_sci_math(ir->k_mean, kb, sizeof(kb), 3);
        kinetics_format_k_units(ir->overall_order, "s", ku, sizeof(ku));
        snprintf(kline, sizeof(kline), "~k~ = %s %s", kb, ku);
        math_draw_right(306, y2 + 7, kline, 14, s1 ? RAMP_INK_BLUE : RAMP_INK_WHITE, COL_INK);
        gfx_hline(16, 160, 288, COL_BORDER);

        /* Step 3 Row: Predict Rate */
        int y3 = 163;
        bool s2 = (app->step_idx == 2);
        if (s2) gfx_fill_rrect(8, y3, 304, 28, 4, COL_BLUE_BG);
        gfx_draw_text(10, y3 + 6, &FONT_UI_11, "3", RAMP_MUTED_WHITE);
        gfx_draw_text(22, y3 + 6, &FONT_UI_11B, "Predicted Rate", s2 ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        char pb[40]; fmt_sci_math(ir->pred_rate, pb, sizeof(pb), 3);
        math_draw_right(306, y3 + 4, pb, 14, s2 ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE, COL_INK);

        ui_draw_keycap(10, 196, "CLEAR", "table");
        ui_draw_keycap(102, 196, "UP/DN", "steps");
    }
    ui_draw_bottom_tabs(1);
}

/* ============================================================================
 * Screen 3: Module 3 — Integrated Rate Laws, 3-Plot Linearization Graph & Half-Life
 * ============================================================================ */
static void draw_mini_plot(int x, int y, int w, int h, const double *tx, const double *vy,
                           int n, const LinReg *reg, const char *title, bool winner) {
    gfx_fill_rrect(x, y, w, h, 3, winner ? COL_GREEN_BG : COL_WHITE);
    gfx_draw_rrect(x, y, w, h, 3, winner ? COL_GREEN : COL_BORDER);
    gfx_draw_text_center(x + w / 2, y + 3, &FONT_UI_11B, title,
                         winner ? RAMP_GREEN_BG : RAMP_INK_WHITE);

    /* Plot area */
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

    /* Fitted regression line */
    double ly0 = reg->slope * xmin + reg->intercept;
    double ly1 = reg->slope * xmax + reg->intercept;
    int gx0 = px + 2;
    int gx1 = px + pw - 2;
    int gy0 = py + ph - (int)((ly0 - ymin) / (ymax - ymin) * (double)(ph - 2));
    int gy1 = py + ph - (int)((ly1 - ymin) / (ymax - ymin) * (double)(ph - 2));
    if (gy0 < py) gy0 = py;
    if (gy0 > py + ph) gy0 = py + ph;
    if (gy1 < py) gy1 = py;
    if (gy1 > py + ph) gy1 = py + ph;
    gfx_line(gx0, gy0, gx1, gy1, winner ? COL_GREEN : COL_ACCENT);

    /* Data points */
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
        /* 3-Plot Linearization Graph Comparison + Best Order & k */
        const IntegratedTableState *it = &app->int_table;
        ui_draw_topbar("Integrated Law", "3-PLOT LINEARITY");

        double y0[MAX_INT_ROWS], y1[MAX_INT_ROWS], y2[MAX_INT_ROWS];
        for (int i = 0; i < it->num_rows; i++) {
            y0[i] = it->c[i];
            y1[i] = log(it->c[i]);
            y2[i] = 1.0 / it->c[i];
        }
        draw_mini_plot(8,   26, 98, 102, it->t, y0, it->num_rows, &it->reg[0], "[A] vs t",   it->best_order == 0);
        draw_mini_plot(111, 26, 98, 102, it->t, y1, it->num_rows, &it->reg[1], "ln[A] vs t", it->best_order == 1);
        draw_mini_plot(214, 26, 98, 102, it->t, y2, it->num_rows, &it->reg[2], "1/[A] vs t", it->best_order == 2);

        /* Winner Summary Card */
        gfx_fill_rrect(8, 134, 304, 56, 4, COL_BLUE_BG);
        const char *onames[3] = { "ZERO ORDER (n = 0)", "FIRST ORDER (n = 1)", "SECOND ORDER (n = 2)" };
        gfx_draw_text(16, 140, &FONT_UI_11B, onames[it->best_order], RAMP_ACCENT_BLUE);
        char kb[40], ku[40], kl[90];
        fmt_sci_math(it->k, kb, sizeof(kb), 3);
        kinetics_format_k_units((double)it->best_order, "s", ku, sizeof(ku));
        snprintf(kl, sizeof(kl), "~k~ = %s %s", kb, ku);
        math_draw(16, 158, kl, 14, RAMP_INK_BLUE, COL_INK);

        char slp[48];
        snprintf(slp, sizeof(slp), "slope = %s~k~", it->best_order == 2 ? "+" : SYM_MINUS);
        math_draw_right(302, 140, slp, 14, RAMP_MUTED_BLUE, COL_MUTED);
        ui_draw_keycap(10, 196, "MODE", "eq solver");
        ui_draw_keycap(124, 196, "APPS", "edit data");
    } else if (app->sub_mode == 1) {
        /* Equation & Half-Life Solver */
        const IntegratedEqState *eq = &app->int_eq;
        char st[24]; snprintf(st, sizeof(st), "ORDER %d " SYM_DOT " SOLVE", eq->order);
        ui_draw_topbar("Integrated Law", st);

        /* Typeset formula banner */
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

        /* Half-life strip */
        gfx_fill_rrect(10, 158, 300, 32, 3, COL_GRAY_BG);
        char hl[96];
        snprintf(hl, sizeof(hl), "1st t1/2=%.3g   2nd=%.3g   3rd=%.3g  (%.1f%% left)",
                 eq->t_half_1, eq->t_half_2, eq->t_half_3, eq->pct_remaining);
        gfx_draw_text_center(160, 168, &FONT_UI_11, hl, RAMP_INK_GRAY);

        ui_draw_keycap(10, 196, "ENTER", "edit");
        ui_draw_keycap(86, 196, "APPS", "solve for");
        ui_draw_keycap(196, 196, "LEFT/RT", "order");
    } else {
        /* Editable (t, [A]) Data Table */
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
    }
    ui_draw_bottom_tabs(2);
}

/* ============================================================================
 * Screen 4: Module 4 — Reaction Mechanisms & Algebraic Intermediate Elimination
 * ============================================================================ */
static void render_mech(const AppState *app) {
    const MechState *m = &app->mech;
    char st[24]; snprintf(st, sizeof(st), "PRESET %d / 5", app->mech_preset + 1);
    ui_draw_topbar("Mechanisms", st);

    /* Elementary steps box */
    gfx_fill_rrect(10, 25, 300, 74, 4, COL_WHITE);
    gfx_draw_rrect(10, 25, 300, 74, 4, COL_BORDER);
    for (int s = 0; s < m->num_steps; s++) {
        const MechStep *st_p = &m->steps[s];
        int y = 30 + s * 22;
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
        gfx_draw_text(16, y + 2, &FONT_UI_11, line, RAMP_INK_WHITE);
        const char *tag = (st_p->speed == STEP_SLOW) ? "SLOW" : (st_p->speed == STEP_FAST_EQ ? "FAST EQ" : "FAST");
        uint8_t bg = (st_p->speed == STEP_SLOW) ? COL_RED_BG : COL_GRAY_BG;
        uint8_t rp = (st_p->speed == STEP_SLOW) ? RAMP_RED_BG : RAMP_MUTED_WHITE;
        ui_draw_pill(248, y, tag, bg, rp);
    }

    /* Derived Overall Equation + Rate Law (Active Blue Card) */
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

    /* Intermediates & Catalysts Badges */
    int bx = 10;
    for (int i = 0; i < m->n_species; i++) {
        if (m->is_intermediate[i]) {
            char b[28]; snprintf(b, sizeof(b), "Int: %s", m->species_name[i]);
            ui_draw_pill(bx, 168, b, COL_AMBER_BG, RAMP_AMBER_BG);
            bx += gfx_text_width(&FONT_UI_11B, b) + 16;
        }
        if (m->is_catalyst[i]) {
            char b[28]; snprintf(b, sizeof(b), "Cat: %s", m->species_name[i]);
            ui_draw_pill(bx, 168, b, COL_GREEN_BG, RAMP_GREEN_BG);
            bx += gfx_text_width(&FONT_UI_11B, b) + 16;
        }
    }

    ui_draw_keycap(10, 196, "LEFT/RT", "presets");
    ui_draw_keycap(124, 196, "ENTER", "flip slow");
    ui_draw_bottom_tabs(3);
}

/* ============================================================================
 * Screen 5: Module 5 — Arrhenius Solver & Energy / Catalyst Profile Graph
 * ============================================================================ */
static void render_arrhenius(const AppState *app) {
    if (app->sub_mode == 0) {
        /* Two-Point Arrhenius Equation Solver */
        const ArrhTwoPtState *a = &app->arrh2;
        ui_draw_topbar("Arrhenius", "2-POINT SOLVER");

        gfx_fill_rrect(10, 26, 300, 42, 4, COL_WHITE);
        gfx_draw_rrect(10, 26, 300, 42, 4, COL_BORDER);
        math_draw_center(160, 38,
                         "ln\\f{~k~_{2}}{~k~_{1}} = \\f{~E~_{a}}{~R~}(\\f{1}{~T~_{1}} " SYM_MINUS " \\f{1}{~T~_{2}})",
                         14, RAMP_INK_WHITE, COL_INK);

        const char *labs[5] = { "Ea (kJ/mol)", "k2", "T2 (K)", "k1", "T1 (K)" };
        double vals[5] = { a->Ea_kj, a->k2, a->T2, a->k1, a->T1 };
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
        ui_draw_keycap(86, 196, "APPS", "solve for");
        ui_draw_keycap(196, 196, "MODE", "energy plot");
    } else {
        /* Potential Energy Profile + Catalyst Speedup Graph */
        const CatalystState *c = &app->cat;
        ui_draw_topbar("Energy Profile", c->delta_E_kj < 0 ? "EXOTHERMIC" : "ENDOTHERMIC");

        /* Plot box */
        int px = 14, py = 28, pw = 292, ph = 112;
        gfx_fill_rrect(px, py, pw, ph, 4, COL_WHITE);
        gfx_draw_rrect(px, py, pw, ph, 4, COL_BORDER);

        /* Axes */
        gfx_vline(px + 20, py + 10, ph - 26, COL_INK);
        gfx_hline(px + 20, py + ph - 16, pw - 32, COL_INK);
        gfx_draw_text(px + 6, py + 12, &FONT_UI_11B, "E", RAMP_MUTED_WHITE);
        gfx_draw_text_right(px + pw - 14, py + ph - 14, &FONT_UI_11, "Rxn Progress", RAMP_MUTED_WHITE);

        /* Draw smooth uncatalyzed (red) and catalyzed (green dashed) reaction curves */
        int y_r = (c->delta_E_kj < 0) ? (py + 62) : (py + 82);
        int y_p = (c->delta_E_kj < 0) ? (py + 88) : (py + 58);
        int y_pk_un  = py + 18;
        int y_pk_cat = py + 40;

        int prev_x = px + 28, prev_yu = y_r, prev_yc = y_r;
        for (int i = 1; i <= 40; i++) {
            double u = (double)i / 40.0;
            int gx = px + 28 + (int)(u * 244.0);
            /* Smooth bell + sigmoid transition */
            double base = (double)y_r + (double)(y_p - y_r) / (1.0 + exp(-12.0 * (u - 0.5)));
            double hump = exp(-28.0 * (u - 0.5) * (u - 0.5));
            int yu = (int)(base - hump * ((double)y_r - (double)y_pk_un));
            int yc = (int)(base - hump * ((double)y_r - (double)y_pk_cat));
            gfx_line(prev_x, prev_yu, gx, yu, COL_RED);
            if ((i & 1) == 0) gfx_line(prev_x, prev_yc, gx, yc, COL_GREEN);
            prev_x = gx; prev_yu = yu; prev_yc = yc;
        }
        /* Reference dashed reactant line & labels */
        gfx_dashed_line(px + 28, y_r, px + 266, y_r, COL_BORDER);
        gfx_draw_text(px + 30, y_r - 13, &FONT_UI_11B, "R", RAMP_INK_WHITE);
        gfx_draw_text(px + 258, y_p - 13, &FONT_UI_11B, "P", RAMP_INK_WHITE);
        gfx_draw_text(px + 158, y_pk_un - 2, &FONT_UI_11B, "Uncat", RAMP_RED_WHITE);
        gfx_draw_text(px + 158, y_pk_cat - 2, &FONT_UI_11B, "Cat", RAMP_GREEN_WHITE);

        /* Quantitative Summary Card */
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

/* ============================================================================
 * Screen 6: Table 12.6 Quick Reference Summary Chart (True Math Typeset!)
 * ============================================================================ */
static void render_summary(const AppState *app) {
    (void)app;
    ui_draw_topbar("Summary Chart", "TABLE 12.6");

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

    ui_draw_keycap(10, 196, "CLEAR", "home menu");
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

/* ============================================================================
 * Unified Key Dispatcher (Returns false to cleanly exit back to TI-OS)
 * ============================================================================ */
bool app_handle_key(AppState *app, int k_in) {
    KeyCode k = (KeyCode)k_in;
    if (k == KEY_QUIT) return false;

    /* Global 5-tab softkeys (Y= .. GRAPH) jump instantly from anywhere! */
    if (k == KEY_F1) { app->screen = SCR_RATES;      app->sub_mode = 0; app->cursor_r = 0; return true; }
    if (k == KEY_F2) { app->screen = SCR_INIT_RATES; app->sub_mode = 0; app->cursor_r = 0; app->cursor_c = 0; return true; }
    if (k == KEY_F3) { app->screen = SCR_INTEGRATED; app->sub_mode = 0; app->cursor_r = 0; return true; }
    if (k == KEY_F4) { app->screen = SCR_MECH;       app->sub_mode = 0; return true; }
    if (k == KEY_F5) { app->screen = SCR_ARRHENIUS;  app->sub_mode = 0; app->cursor_r = 0; return true; }

    if (app->screen == SCR_HOME) {
        if (k == KEY_CLEAR) return false;
        if (k == KEY_LEFT  && app->home_sel % 3 > 0) app->home_sel--;
        if (k == KEY_RIGHT && app->home_sel % 3 < 2) app->home_sel++;
        if (k == KEY_UP    && app->home_sel >= 3) app->home_sel -= 3;
        if (k == KEY_DOWN  && app->home_sel < 3)  app->home_sel += 3;
        if (k >= KEY_1 && k <= KEY_6) {
            app->home_sel = (int)(k - KEY_1);
            k = KEY_ENTER;
        }
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
        if (k == KEY_MODE) { app->sub_mode ^= 1; app->cursor_r = 0; }
        else if (app->sub_mode == 0) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 4) app->cursor_r++;
            if (k == KEY_APPS) { app->rates.is_reactant = !app->rates.is_reactant; kinetics_rate_calc(&app->rates); }
            if (k == KEY_ENTER) {
                double *ptr[5] = { &app->rates.c1, &app->rates.c2, &app->rates.t1, &app->rates.t2, &app->rates.coeff };
                const char *pr[5] = { "Edit [A]1 (M)", "Edit [A]2 (M)", "Edit t1 (s)", "Edit t2 (s)", "Edit Coeff a" };
                if (ui_edit_double(pr[app->cursor_r], ptr[app->cursor_r])) kinetics_rate_calc(&app->rates);
            }
        } else {
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
        }
    } else if (app->screen == SCR_INTEGRATED) {
        if (k == KEY_MODE) { app->sub_mode = (app->sub_mode == 0) ? 1 : 0; app->cursor_r = 0; }
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
        }
    } else if (app->screen == SCR_MECH) {
        if (k == KEY_LEFT)  app_load_mech_preset(app, (app->mech_preset + 4) % 5);
        if (k == KEY_RIGHT) app_load_mech_preset(app, (app->mech_preset + 1) % 5);
        if (k == KEY_ENTER) {
            /* Rotate which step is Slow vs Fast Eq */
            int ns = app->mech.num_steps;
            int next_slow = (app->mech.slow_step_idx + 1) % ns;
            for (int s = 0; s < ns; s++) {
                if (s == next_slow) app->mech.steps[s].speed = STEP_SLOW;
                else if (s < next_slow) app->mech.steps[s].speed = STEP_FAST_EQ;
                else app->mech.steps[s].speed = STEP_FAST;
            }
            kinetics_analyze_mech(&app->mech);
        }
    } else if (app->screen == SCR_ARRHENIUS) {
        if (k == KEY_MODE) { app->sub_mode ^= 1; app->cursor_r = 0; }
        else if (app->sub_mode == 0) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 4) app->cursor_r++;
            if (k == KEY_APPS) { app->arrh2.unknown = app->cursor_r; kinetics_arrhenius_2pt(&app->arrh2); }
            if (k == KEY_ENTER) {
                double *ptrs[5] = { &app->arrh2.Ea_kj, &app->arrh2.k2, &app->arrh2.T2, &app->arrh2.k1, &app->arrh2.T1 };
                if (ui_edit_double("Edit Arrhenius Val", ptrs[app->cursor_r])) kinetics_arrhenius_2pt(&app->arrh2);
            }
        } else {
            if (k == KEY_ENTER) {
                if (ui_edit_double("Uncatalyzed Ea (kJ)", &app->cat.Ea_uncat_kj)) {
                    ui_edit_double("Catalyzed Ea (kJ)", &app->cat.Ea_cat_kj);
                    ui_edit_double("Delta E (kJ, -exo)", &app->cat.delta_E_kj);
                    kinetics_catalyst_calc(&app->cat);
                }
            }
        }
    }
    return true;
}
