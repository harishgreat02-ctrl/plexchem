#include "ui_app.h"
#include "theory_data.h"
#include "hal.h"
#include "gfx.h"
#include "math_render.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * Species Pool for Custom Mechanism Builder (Zero Popup — Inline Editing!)
 * ============================================================================ */
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

/* ============================================================================
 * Inline Number Editing Helpers (ZERO Popups — Type Directly in Any Field!)
 * ============================================================================ */
static double *get_active_field_ptr(AppState *app, bool *is_celsius_temp) {
    *is_celsius_temp = false;
    if (app->screen == SCR_RATES) {
        if (app->sub_mode == 0) {
            double *p[5] = { &app->rates.c1, &app->rates.c2, &app->rates.t1, &app->rates.t2, &app->rates.coeff };
            return (app->cursor_r >= 0 && app->cursor_r < 5) ? p[app->cursor_r] : NULL;
        }
        if (app->sub_mode == 1) {
            if (app->cursor_r >= 0 && app->cursor_r < 3) {
                return (app->cursor_c == 0) ? &app->stoich.coeffs[app->cursor_r] : &app->stoich.known_rate;
            }
        }
        if (app->sub_mode == 2) return &app->k_unit_order;
    } else if (app->screen == SCR_INIT_RATES) {
        if (app->sub_mode == 0) {
            if (app->cursor_r < app->init_rates.num_rows) {
                return (app->cursor_c < app->init_rates.num_reactants)
                       ? &app->init_rates.conc[app->cursor_r][app->cursor_c]
                       : &app->init_rates.rate[app->cursor_r];
            }
        } else if (app->sub_mode == 1 && app->step_idx == 2) {
            if (app->cursor_c < app->init_rates.num_reactants)
                return &app->init_rates.pred_conc[app->cursor_c];
        }
    } else if (app->screen == SCR_INTEGRATED) {
        if (app->sub_mode == 1) {
            double *p[4] = { &app->int_eq.A0, &app->int_eq.At, &app->int_eq.k, &app->int_eq.t };
            return (app->cursor_r >= 0 && app->cursor_r < 4) ? p[app->cursor_r] : NULL;
        }
        if (app->sub_mode == 2 && app->cursor_r < app->int_table.num_rows) {
            return (app->cursor_c == 0) ? &app->int_table.t[app->cursor_r] : &app->int_table.c[app->cursor_r];
        }
        if (app->sub_mode == 3) {
            double *p[3] = { &app->pseudo_kp, &app->pseudo_excess, &app->pseudo_order };
            return (app->cursor_r >= 0 && app->cursor_r < 3) ? p[app->cursor_r] : NULL;
        }
    } else if (app->screen == SCR_ARRHENIUS) {
        if (app->sub_mode == 0) {
            if (app->cursor_r == 2 || app->cursor_r == 4) *is_celsius_temp = app->arrh_celsius;
            double *p[5] = { &app->arrh2.Ea_kj, &app->arrh2.k2, &app->arrh2.T2, &app->arrh2.k1, &app->arrh2.T1 };
            return (app->cursor_r >= 0 && app->cursor_r < 5) ? p[app->cursor_r] : NULL;
        }
        if (app->sub_mode == 1) {
            if (app->cursor_r < app->arrh_tbl.num_rows)
                return (app->cursor_c == 0) ? &app->arrh_tbl.T[app->cursor_r] : &app->arrh_tbl.k[app->cursor_r];
            return &app->arrh_tbl.eval_T;
        }
        if (app->sub_mode == 2) {
            double *p[4] = { &app->cat.Ea_uncat_kj, &app->cat.Ea_cat_kj, &app->cat.delta_E_kj, &app->cat.T_kelvin };
            return (app->cursor_r >= 0 && app->cursor_r < 4) ? p[app->cursor_r] : NULL;
        }
    }
    return NULL;
}

static void recalc_active_screen(AppState *app) {
    if (app->screen == SCR_RATES) {
        if (app->sub_mode == 0) kinetics_rate_calc(&app->rates);
        else if (app->sub_mode == 1) {
            if (app->cursor_c == 1) app->stoich.known_idx = app->cursor_r;
            kinetics_stoich_rates(&app->stoich);
        }
    } else if (app->screen == SCR_INIT_RATES) {
        kinetics_solve_initial_rates(&app->init_rates);
    } else if (app->screen == SCR_INTEGRATED) {
        if (app->sub_mode == 1) kinetics_integrated_solve(&app->int_eq);
        else if (app->sub_mode == 2) kinetics_regress_order(&app->int_table);
    } else if (app->screen == SCR_ARRHENIUS) {
        if (app->sub_mode == 0) kinetics_arrhenius_2pt(&app->arrh2);
        else if (app->sub_mode == 1) kinetics_arrhenius_regress(&app->arrh_tbl);
        else if (app->sub_mode == 2) kinetics_catalyst_calc(&app->cat);
    }
}

static void begin_inline_edit(AppState *app, char initial_ch) {
    bool is_c = false;
    double *ptr = get_active_field_ptr(app, &is_c);
    if (!ptr) return;
    app->editing = true;
    if (initial_ch != 0) {
        app->edit_buf[0] = initial_ch;
        app->edit_buf[1] = '\0';
        app->edit_len = 1;
        app->edit_first_key = false;
    } else {
        double v = is_c ? (*ptr - 273.15) : *ptr;
        fmt_compact(v, app->edit_buf, sizeof(app->edit_buf));
        app->edit_len = (int)strlen(app->edit_buf);
        app->edit_first_key = true;
    }
}

static void commit_inline_edit(AppState *app) {
    bool is_c = false;
    double *ptr = get_active_field_ptr(app, &is_c);
    if (ptr && app->edit_len > 0) {
        double v = strtod(app->edit_buf, NULL);
        *ptr = is_c ? (v + 273.15) : v;
        recalc_active_screen(app);
    }
    app->editing = false;
}

static char key_to_numchar(KeyCode k) {
    if (k >= KEY_0 && k <= KEY_9) return (char)('0' + (k - KEY_0));
    if (k == KEY_DOT) return '.';
    if (k == KEY_NEG) return '-';
    if (k == KEY_EE)  return 'e';
    return 0;
}

static void draw_inline_value(int right_x, int y, double val, bool is_editing,
                              const AppState *app, uint8_t normal_ramp) {
    if (is_editing) {
        int tw = gfx_text_width(&FONT_UI_11B, app->edit_buf);
        int bw = tw + 14;
        if (bw < 44) bw = 44;
        gfx_fill_rrect(right_x - bw, y - 1, bw, 16, 2, COL_WHITE);
        gfx_draw_rrect(right_x - bw, y - 1, bw, 16, 2, COL_ACCENT);
        gfx_draw_text(right_x - bw + 5, y, &FONT_UI_11B, app->edit_buf, RAMP_ACCENT_WHITE);
        gfx_vline(right_x - bw + 5 + tw + 1, y + 1, 12, COL_ACCENT);
    } else {
        char vb[24];
        fmt_compact(val, vb, sizeof(vb));
        gfx_draw_text_right(right_x, y, &FONT_UI_11B, vb, normal_ramp);
    }
}

/* ============================================================================
 * Default Initialization
 * ============================================================================ */
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
    app->theory_list = true;
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

/* ============================================================================
 * Screen 0: Brand-New Row-Based Home Menu (ZERO Icons, ZERO Tiles!)
 * ============================================================================ */
static void render_home(const AppState *app) {
    static const char * const TITLES[7] = {
        "Reaction Rates",
        "Initial Rates Table",
        "Integrated & Plots",
        "Mechanisms Builder",
        "Arrhenius & Energy",
        "Chart 12.6 & Graphs",
        "Theories & FRQ Guide"
    };
    static const char * const DESCS[7] = {
        "Avg, Stoich & k Units",
        "Orders, k & Steps",
        "Graphs, t1/2 & Pseudo",
        "Custom Steps & RDS",
        "2-Pt, ln(k) & Catalysts",
        "Master Ref & Curves",
        "12 Topics: Def/How/Ex"
    };

    ui_draw_topbar("PlexChem  Ch. 12", "PRESS 1-7 OR ENTER");

    for (int i = 0; i < 7; i++) {
        int y = 25 + i * 27;
        bool sel = (app->home_sel == i);
        if (sel) {
            gfx_fill_rrect(8, y, 304, 24, 3, COL_BLUE_BG);
            gfx_fill_rect(8, y + 3, 3, 18, COL_ACCENT);
        } else {
            gfx_hline(14, y + 25, 292, COL_GRID);
        }
        char nb[4]; snprintf(nb, sizeof(nb), "%d", i + 1);
        gfx_fill_rrect(16, y + 5, 16, 14, 2, sel ? COL_ACCENT : COL_GRAY_BG);
        gfx_draw_text_center(24, y + 5, &FONT_UI_11B, nb, sel ? RAMP_WHITE_ACCENT : RAMP_MUTED_WHITE);

        gfx_draw_text(38, y + 5, &FONT_UI_11B, TITLES[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
        gfx_draw_text_right(304, y + 5, &FONT_UI_11, DESCS[i], sel ? RAMP_INK_BLUE : RAMP_MUTED_WHITE);
    }

    ui_draw_softkeys("Open", "Rates", "Table", "Graphs", "Theory", 0);
}

/* ============================================================================
 * Screen 1: Reaction Rates (Avg | Stoich | k Units) — Inline Zero-Popup!
 * ============================================================================ */
static void render_rates(const AppState *app) {
    const char *hdr[3] = { "AVG RATE", "STOICHIOMETRY", "UNITS OF k" };
    ui_draw_topbar("Reaction Rates", hdr[app->sub_mode]);

    if (app->sub_mode == 0) {
        gfx_fill_rrect(10, 26, 300, 56, 4, COL_WHITE);
        gfx_draw_rrect(10, 26, 300, 56, 4, COL_BORDER);
        math_draw_center(160, 32,
                         app->rates.is_reactant
                             ? "~Rate~ = " SYM_MINUS "\\f{" SYM_DELTA "[A]}{~a~" SYM_DELTA "~t~}"
                             : "~Rate~ = +\\f{" SYM_DELTA "[C]}{~c~" SYM_DELTA "~t~}",
                         14, RAMP_MUTED_WHITE, COL_MUTED);
        gfx_hline(24, 54, 272, COL_BORDER);
        char rbuf[48], eqbuf[80];
        fmt_sci_math(app->rates.overall_rate, rbuf, sizeof(rbuf), 3);
        snprintf(eqbuf, sizeof(eqbuf), "~Rate~ = %s \\f{mol}{L" SYM_DOT "s}", rbuf);
        math_draw_center(160, 60, eqbuf, 14, RAMP_ACCENT_WHITE, COL_ACCENT);

        const char *labels[5] = { "[A]1 (initial M)", "[A]2 (final M)", "t1 (initial s)", "t2 (final s)", "Coeff a" };
        double vals[5] = { app->rates.c1, app->rates.c2, app->rates.t1, app->rates.t2, app->rates.coeff };
        for (int i = 0; i < 5; i++) {
            int y = 89 + i * 22;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(10, y, 300, 20, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 3, &FONT_UI_11B, labels[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            draw_inline_value(302, y + 3, vals[i], sel && app->editing, app,
                              sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }
        gfx_draw_text(12, 202, &FONT_UI_11, "Type number directly + ENTER  (no popups!)", RAMP_MUTED_WHITE);
        ui_draw_softkeys(app->rates.is_reactant ? "React" : "Prod", "Avg", "Stoich", "k Unit", "Home", 1);
    } else if (app->sub_mode == 1) {
        char eq[64];
        snprintf(eq, sizeof(eq), "%.0fA + %.0fB " SYM_ARROW " %.0fC",
                 app->stoich.coeffs[0], app->stoich.coeffs[1], app->stoich.coeffs[2]);
        math_draw_center(160, 27, eq, 14, RAMP_INK_WHITE, COL_INK);

        gfx_fill_rrect(10, 49, 300, 32, 4, COL_BLUE_BG);
        gfx_draw_text(18, 57, &FONT_UI_11B, "Overall Rate:", RAMP_INK_BLUE);
        char rb[40]; fmt_sci_math(app->stoich.overall_rate, rb, sizeof(rb), 3);
        math_draw_right(302, 56, rb, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        const char *sp[3] = { "A (React)", "B (React)", "C (Prod)" };
        gfx_draw_text(18, 86, &FONT_UI_11, "Species", RAMP_MUTED_WHITE);
        gfx_draw_text(126, 86, &FONT_UI_11, "Coeff", RAMP_MUTED_WHITE);
        gfx_draw_text_right(302, 86, &FONT_UI_11, "Rate (mol/L" SYM_DOT "s)", RAMP_MUTED_WHITE);
        gfx_hline(10, 100, 300, COL_BORDER);

        for (int i = 0; i < 3; i++) {
            int y = 105 + i * 28;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(10, y, 300, 24, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 5, &FONT_UI_11B, sp[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            draw_inline_value(162, y + 5, app->stoich.coeffs[i], sel && app->cursor_c == 0 && app->editing,
                              app, (sel && app->cursor_c == 0) ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            draw_inline_value(302, y + 5, app->stoich.rates[i], sel && app->cursor_c == 1 && app->editing,
                              app, (sel && app->cursor_c == 1) ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
        }
        gfx_draw_text(12, 198, &FONT_UI_11, "L/R: Coeff vs Rate   Type any value + ENTER", RAMP_MUTED_WHITE);
        ui_draw_softkeys("Calc", "Avg", "Stoich", "k Unit", "Home", 2);
    } else {
        gfx_fill_rrect(10, 28, 300, 82, 4, COL_BLUE_BG);
        char oline[48]; snprintf(oline, sizeof(oline), "Overall Order  ~n~ = %.2g", app->k_unit_order);
        math_draw_center(160, 36, oline, 14, RAMP_ACCENT_BLUE, COL_ACCENT);
        char ku[48], kfull[80];
        kinetics_format_k_units(app->k_unit_order, "s", ku, sizeof(ku));
        snprintf(kfull, sizeof(kfull), "~k~ = %s", ku);
        math_draw_center(160, 64, kfull, 20, RAMP_INK_BLUE, COL_INK);

        gfx_fill_rrect(10, 118, 300, 74, 4, COL_WHITE);
        gfx_draw_rrect(10, 118, 300, 74, 4, COL_BORDER);
        gfx_draw_text(18, 124, &FONT_UI_11B, "Type any order n (e.g. 0, 1, 2, 1.5, -1):", RAMP_INK_WHITE);
        draw_inline_value(298, 124, app->k_unit_order, app->editing, app, RAMP_ACCENT_WHITE);
        gfx_draw_text(18, 144, &FONT_UI_11, "0th: mol/(L" SYM_DOT "s)   1st: s^-1   2nd: L/(mol" SYM_DOT "s)", RAMP_MUTED_WHITE);
        gfx_draw_text(18, 160, &FONT_UI_11, "3rd: L^2/(mol^2" SYM_DOT "s)  L/R steps by +/-0.5", RAMP_MUTED_WHITE);
        gfx_draw_text(18, 175, &FONT_UI_11, "Formula:  k units = M^(1-n) / time", RAMP_ACCENT_WHITE);
        ui_draw_softkeys("n+0.5", "Avg", "Stoich", "k Unit", "Home", 3);
    }
}

/* ============================================================================
 * Screen 2: Initial Rates Table + Real Expanding Steps (Inline Zero-Popup!)
 * ============================================================================ */
static void render_init_rates(const AppState *app) {
    const InitialRatesState *ir = &app->init_rates;
    if (app->sub_mode == 0) {
        char st[32]; snprintf(st, sizeof(st), "%dR " SYM_TIMES " %d ROWS", ir->num_reactants, ir->num_rows);
        ui_draw_topbar("Initial Rates", st);

        int cols = ir->num_reactants + 1;
        int cw = 268 / cols;
        int y0 = 26;
        gfx_fill_rect(10, y0, 300, 18, COL_CHROME);
        gfx_draw_rect(10, y0, 300, 18, COL_BORDER);
        gfx_draw_text(16, y0 + 2, &FONT_UI_11B, "#", RAMP_MUTED_CHROME);
        const char *chdr[3] = { "[A]", "[B]", "[C]" };
        for (int c = 0; c < ir->num_reactants; c++)
            gfx_draw_text_center(42 + c * cw + cw / 2, y0 + 2, &FONT_UI_11B, chdr[c], RAMP_INK_CHROME);
        gfx_draw_text_center(42 + ir->num_reactants * cw + cw / 2, y0 + 2, &FONT_UI_11B, "Rate", RAMP_INK_CHROME);

        int vis = ir->num_rows < 6 ? ir->num_rows : 6;
        int start_r = (app->cursor_r >= vis) ? (app->cursor_r - vis + 1) : 0;
        for (int vi = 0; vi < vis; vi++) {
            int r = start_r + vi;
            int ry = y0 + 18 + vi * 22;
            gfx_draw_rect(10, ry, 300, 22, COL_BORDER);
            char rb[16]; snprintf(rb, sizeof(rb), "%d", r + 1);
            gfx_draw_text(16, ry + 4, &FONT_UI_11, rb, RAMP_MUTED_WHITE);

            for (int c = 0; c < cols; c++) {
                int cx = 42 + c * cw;
                bool sel = (app->cursor_r == r && app->cursor_c == c);
                if (sel) {
                    gfx_fill_rect(cx + 1, ry + 1, cw - 2, 20, app->editing ? COL_WHITE : COL_BLUE_BG);
                    gfx_draw_rect(cx, ry, cw, 22, COL_ACCENT);
                }
                if (sel && app->editing) {
                    int tw = gfx_draw_text(cx + 4, ry + 4, &FONT_UI_11B, app->edit_buf, RAMP_ACCENT_WHITE);
                    gfx_vline(cx + 4 + tw + 1, ry + 4, 13, COL_ACCENT);
                } else {
                    double v = (c < ir->num_reactants) ? ir->conc[r][c] : ir->rate[r];
                    char vb[16]; fmt_compact(v, vb, sizeof(vb));
                    gfx_draw_text_center(cx + cw / 2, ry + 4, &FONT_UI_11, vb,
                                         sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
                }
            }
        }
        /* Live solved summary strip right below table */
        gfx_fill_rrect(10, 180, 300, 34, 3, COL_BLUE_BG);
        char sline[112], kb[32];
        fmt_sci_math(ir->k_mean, kb, sizeof(kb), 3);
        if (ir->num_reactants == 1)
            snprintf(sline, sizeof(sline), "~n~=%.2g   ~k~ = %s", ir->order[0], kb);
        else if (ir->num_reactants == 2)
            snprintf(sline, sizeof(sline), "~n~=%.2g, ~m~=%.2g   ~k~ = %s", ir->order[0], ir->order[1], kb);
        else
            snprintf(sline, sizeof(sline), "~n~=%.2g, ~m~=%.2g, ~p~=%.2g   ~k~ = %s", ir->order[0], ir->order[1], ir->order[2], kb);
        math_draw_center(160, 189, sline, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        ui_draw_softkeys("Steps", "+Row", "-Row", "1/2/3R", "Home", -1);
    } else {
        char st[24]; snprintf(st, sizeof(st), "STEP %d / 3", app->step_idx + 1);
        ui_draw_topbar("Initial Rates Steps", st);

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
        math_draw_center(160, 26, rl, 20, RAMP_INK_WHITE, COL_INK);

        int cur_y = 56;
        for (int s = 0; s < 3; s++) {
            bool act = (app->step_idx == s);
            if (act) {
                gfx_fill_rrect(8, cur_y, 304, 72, 4, COL_BLUE_BG);
                char tag[12]; snprintf(tag, sizeof(tag), "STEP %d", s + 1);
                gfx_draw_text_right(304, cur_y + 5, &FONT_UI_11B, tag, RAMP_ACCENT_BLUE);
                if (s == 0) {
                    gfx_draw_text(16, cur_y + 5, &FONT_UI_11B, "Find reactant orders (log ratio)", RAMP_INK_BLUE);
                    char sub[96];
                    snprintf(sub, sizeof(sub), "Trials (%d,%d) " SYM_ARROW " Overall order = %.2g",
                             ir->pair_r1[0] + 1, ir->pair_r2[0] + 1, ir->overall_order);
                    gfx_draw_text(16, cur_y + 21, &FONT_UI_11, sub, RAMP_MUTED_BLUE);
                    char om[80];
                    if (ir->num_reactants == 1) snprintf(om, sizeof(om), "~n~_{A} = %.2g  (raw %.2f)", ir->order[0], ir->raw_order[0]);
                    else if (ir->num_reactants == 2) snprintf(om, sizeof(om), "~n~_{A} = %.2g,  ~m~_{B} = %.2g,  ~n~+~m~ = %.2g", ir->order[0], ir->order[1], ir->overall_order);
                    else snprintf(om, sizeof(om), "~n~=%.2g, ~m~=%.2g, ~p~=%.2g (Sum %.2g)", ir->order[0], ir->order[1], ir->order[2], ir->overall_order);
                    math_draw_center(160, cur_y + 44, om, 14, RAMP_INK_BLUE, COL_INK);
                } else if (s == 1) {
                    gfx_draw_text(16, cur_y + 5, &FONT_UI_11B, "Solve mean rate constant k", RAMP_INK_BLUE);
                    char sub[80];
                    snprintf(sub, sizeof(sub), "Trial 1 k=%.3g,  Trial 2 k=%.3g", ir->k_trial[0], ir->k_trial[1]);
                    gfx_draw_text(16, cur_y + 21, &FONT_UI_11, sub, RAMP_MUTED_BLUE);
                    char kb[40], ku[40], kl[90];
                    fmt_sci_math(ir->k_mean, kb, sizeof(kb), 3);
                    kinetics_format_k_units(ir->overall_order, "s", ku, sizeof(ku));
                    snprintf(kl, sizeof(kl), "~k~ = %s %s", kb, ku);
                    math_draw_center(160, cur_y + 42, kl, 14, RAMP_INK_BLUE, COL_INK);
                } else {
                    gfx_draw_text(16, cur_y + 5, &FONT_UI_11B, "Predict Rate (L/R pick, type #)", RAMP_INK_BLUE);
                    /* Inline editable prediction concentrations! */
                    for (int j = 0; j < ir->num_reactants; j++) {
                        int px = 16 + j * 96;
                        char lb[16]; snprintf(lb, sizeof(lb), "[%s]:", rn[j]);
                        gfx_draw_text(px, cur_y + 22, &FONT_UI_11, lb, (app->cursor_c == j) ? RAMP_ACCENT_BLUE : RAMP_MUTED_BLUE);
                        draw_inline_value(px + 86, cur_y + 22, ir->pred_conc[j],
                                          (app->cursor_c == j) && app->editing, app,
                                          (app->cursor_c == j) ? RAMP_ACCENT_BLUE : RAMP_INK_BLUE);
                    }
                    char pb[40], pl[72];
                    fmt_sci_math(ir->pred_rate, pb, sizeof(pb), 3);
                    snprintf(pl, sizeof(pl), "~Rate~ = %s \\f{mol}{L" SYM_DOT "s}", pb);
                    math_draw_center(160, cur_y + 45, pl, 14, RAMP_ACCENT_BLUE, COL_ACCENT);
                }
                cur_y += 76;
            } else {
                char nb[4]; snprintf(nb, sizeof(nb), "%d", s + 1);
                gfx_draw_text(12, cur_y + 7, &FONT_UI_11, nb, RAMP_MUTED_WHITE);
                if (s == 0) {
                    gfx_draw_text(24, cur_y + 7, &FONT_UI_11B, "Reactant orders", RAMP_INK_WHITE);
                    char ov[32]; snprintf(ov, sizeof(ov), "Overall = %.2g", ir->overall_order);
                    gfx_draw_text_right(304, cur_y + 7, &FONT_UI_11, ov, RAMP_MUTED_WHITE);
                } else if (s == 1) {
                    gfx_draw_text(24, cur_y + 7, &FONT_UI_11B, "Mean rate constant", RAMP_INK_WHITE);
                    char kb[40]; fmt_sci_math(ir->k_mean, kb, sizeof(kb), 3);
                    math_draw_right(304, cur_y + 6, kb, 14, RAMP_INK_WHITE, COL_INK);
                } else {
                    gfx_draw_text(24, cur_y + 7, &FONT_UI_11B, "Predicted rate", RAMP_INK_WHITE);
                    char pb[40]; fmt_sci_math(ir->pred_rate, pb, sizeof(pb), 3);
                    math_draw_right(304, cur_y + 6, pb, 14, RAMP_INK_WHITE, COL_INK);
                }
                gfx_hline(14, cur_y + 28, 292, COL_BORDER);
                cur_y += 30;
            }
        }
        ui_draw_softkeys("Table", "Prev", "Next", "Edit[]", "Home", -1);
    }
}

/* ============================================================================
 * Screen 3: Integrated Rate Laws (Plots | Eq Solver | Data Table | Pseudo)
 * ============================================================================ */
static void draw_mini_plot(int x, int y, int w, int h, const double *tx, const double *vy,
                           int n, const LinReg *reg, const char *title, bool winner) {
    gfx_fill_rrect(x, y, w, h, 3, winner ? COL_GREEN_BG : COL_WHITE);
    gfx_draw_rrect(x, y, w, h, 3, winner ? COL_GREEN : COL_BORDER);
    gfx_draw_text_center(x + w / 2, y + 3, &FONT_UI_11B, title, winner ? RAMP_GREEN_BG : RAMP_INK_WHITE);

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

    char r2s[24]; snprintf(r2s, sizeof(r2s), "R2=%.4f", reg->r2);
    gfx_draw_text_center(x + w / 2, y + h - 15, &FONT_UI_11, r2s, winner ? RAMP_GREEN_BG : RAMP_MUTED_WHITE);
}

static void render_integrated(const AppState *app) {
    if (app->sub_mode == 0) {
        const IntegratedTableState *it = &app->int_table;
        ui_draw_topbar("Integrated Law", "3-PLOT LINEARITY");

        double y0[MAX_INT_ROWS], y1[MAX_INT_ROWS], y2[MAX_INT_ROWS];
        for (int i = 0; i < it->num_rows; i++) {
            y0[i] = it->c[i]; y1[i] = log(it->c[i]); y2[i] = 1.0 / it->c[i];
        }
        draw_mini_plot(8,   26, 98, 108, it->t, y0, it->num_rows, &it->reg[0], "[A] vs t",   it->best_order == 0);
        draw_mini_plot(111, 26, 98, 108, it->t, y1, it->num_rows, &it->reg[1], "ln[A] vs t", it->best_order == 1);
        draw_mini_plot(214, 26, 98, 108, it->t, y2, it->num_rows, &it->reg[2], "1/[A] vs t", it->best_order == 2);

        gfx_fill_rrect(8, 140, 304, 68, 4, COL_BLUE_BG);
        const char *onames[3] = { "ZERO ORDER (n = 0)", "FIRST ORDER (n = 1)", "SECOND ORDER (n = 2)" };
        gfx_draw_text(16, 146, &FONT_UI_11B, onames[it->best_order], RAMP_ACCENT_BLUE);
        char kb[40], ku[40], kl[90];
        fmt_sci_math(it->k, kb, sizeof(kb), 3);
        kinetics_format_k_units((double)it->best_order, "s", ku, sizeof(ku));
        snprintf(kl, sizeof(kl), "~k~ = %s %s", kb, ku);
        math_draw(16, 168, kl, 14, RAMP_INK_BLUE, COL_INK);

        char slp[48]; snprintf(slp, sizeof(slp), "slope = %s~k~", it->best_order == 2 ? "+" : SYM_MINUS);
        math_draw_right(302, 146, slp, 14, RAMP_MUTED_BLUE, COL_MUTED);
        ui_draw_softkeys("Plots", "Eq.Slv", "Data", "Pseudo", "Home", 0);
    } else if (app->sub_mode == 1) {
        const IntegratedEqState *eq = &app->int_eq;
        char st[24]; snprintf(st, sizeof(st), "ORDER %d " SYM_DOT " SOLVER", eq->order);
        ui_draw_topbar("Integrated Eq.", st);

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
            int y = 74 + i * 22;
            bool sel = (app->cursor_r == i);
            bool unk = (eq->unknown == i);
            if (sel) gfx_fill_rrect(10, y, 300, 20, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 3, &FONT_UI_11B, flab[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            if (unk) ui_draw_pill(138, y + 2, "SOLVED", COL_GREEN_BG, RAMP_GREEN_BG);
            draw_inline_value(302, y + 3, fval[i], sel && app->editing, app,
                              unk ? RAMP_GREEN_WHITE : (sel ? RAMP_INK_BLUE : RAMP_INK_WHITE));
        }

        gfx_fill_rrect(10, 166, 300, 42, 3, COL_GRAY_BG);
        char hl[96];
        snprintf(hl, sizeof(hl), "t1/2: 1st=%.3g  2nd=%.3g  3rd=%.3g (%.1f%% left)",
                 eq->t_half_1, eq->t_half_2, eq->t_half_3, eq->pct_remaining);
        gfx_draw_text_center(160, 173, &FONT_UI_11B, hl, RAMP_INK_GRAY);
        gfx_draw_text_center(160, 190, &FONT_UI_11, "L/R: Order 0/1/2/3   APPS: Mark Unknown", RAMP_MUTED_WHITE);
        ui_draw_softkeys("Plots", "Eq.Slv", "Data", "Pseudo", "Home", 1);
    } else if (app->sub_mode == 2) {
        const IntegratedTableState *it = &app->int_table;
        char st[24]; snprintf(st, sizeof(st), "%d ROWS (+/-)", it->num_rows);
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
            int ry = y0 + 18 + r * 22;
            gfx_draw_rect(10, ry, 300, 22, COL_BORDER);
            bool s0 = (app->cursor_r == r && app->cursor_c == 0);
            bool s1 = (app->cursor_r == r && app->cursor_c == 1);
            if (s0) gfx_fill_rect(11, ry + 1, 74, 20, COL_BLUE_BG);
            if (s1) gfx_fill_rect(86, ry + 1, 74, 20, COL_BLUE_BG);

            if (s0 && app->editing) gfx_draw_text(18, ry + 4, &FONT_UI_11B, app->edit_buf, RAMP_ACCENT_BLUE);
            else { char b0[16]; fmt_compact(it->t[r], b0, sizeof(b0)); gfx_draw_text(22, ry + 4, &FONT_UI_11, b0, s0 ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE); }

            if (s1 && app->editing) gfx_draw_text(92, ry + 4, &FONT_UI_11B, app->edit_buf, RAMP_ACCENT_BLUE);
            else { char b1[16]; fmt_compact(it->c[r], b1, sizeof(b1)); gfx_draw_text(98, ry + 4, &FONT_UI_11, b1, s1 ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE); }

            char b2[16], b3[16];
            fmt_compact(log(it->c[r]), b2, sizeof(b2));
            fmt_compact(1.0 / it->c[r], b3, sizeof(b3));
            gfx_draw_text(172, ry + 4, &FONT_UI_11, b2, RAMP_MUTED_WHITE);
            gfx_draw_text(244, ry + 4, &FONT_UI_11, b3, RAMP_MUTED_WHITE);
        }
        ui_draw_softkeys("Plots", "Eq.Slv", "Data", "Pseudo", "Home", 2);
    } else {
        ui_draw_topbar("Pseudo-Order", "k' " SYM_ARROW " TRUE k");
        double k_true = kinetics_pseudo_k(app->pseudo_kp, app->pseudo_excess, app->pseudo_order);
        gfx_fill_rrect(10, 28, 300, 56, 4, COL_BLUE_BG);
        math_draw_center(160, 36, "~k~ = \\f{~k~" SYM_PRIME "}{[B]_{0}^{~m~}}", 14, RAMP_MUTED_BLUE, COL_MUTED);
        char kb[40], kl[64]; fmt_sci_math(k_true, kb, sizeof(kb), 3);
        snprintf(kl, sizeof(kl), "True ~k~ = %s", kb);
        math_draw_center(160, 62, kl, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        const char *pl[3] = { "Pseudo k' (-slope)", "Excess [B]0 (M)", "Order m of B" };
        double pv[3] = { app->pseudo_kp, app->pseudo_excess, app->pseudo_order };
        for (int i = 0; i < 3; i++) {
            int y = 96 + i * 28;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(10, y, 300, 24, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 5, &FONT_UI_11B, pl[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            draw_inline_value(302, y + 5, pv[i], sel && app->editing, app, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }
        ui_draw_softkeys("Plots", "Eq.Slv", "Data", "Pseudo", "Home", 3);
    }
}

/* ============================================================================
 * Screen 4: Custom Mechanisms Builder (ZERO Popups — Inline Step Editor!)
 * ============================================================================ */
static void render_mech(const AppState *app) {
    const MechState *m = &app->mech;
    if (app->mech_editing) {
        const MechStep *st = &m->steps[app->cursor_r];
        char hdr[48]; snprintf(hdr, sizeof(hdr), "EDIT STEP %d (NO POPUP)", app->cursor_r + 1);
        ui_draw_topbar("Mechanisms", hdr);

        const char *rlab[5] = { "Reactant 1", "Reactant 2", "Product 1", "Product 2", "Step Speed" };
        for (int i = 0; i < 5; i++) {
            int y = 32 + i * 30;
            bool sel = (app->mech_field == i);
            gfx_fill_rrect(10, y, 300, 26, 3, sel ? COL_BLUE_BG : COL_WHITE);
            gfx_draw_rrect(10, y, 300, 26, 3, sel ? COL_ACCENT : COL_BORDER);
            gfx_draw_text(18, y + 6, &FONT_UI_11B, rlab[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);

            char val[32];
            if (i == 0) {
                if (st->n_react >= 1) snprintf(val, sizeof(val), "%d %s", st->react[0].coeff, st->react[0].formula);
                else strcpy(val, "-- (none)");
            } else if (i == 1) {
                if (st->n_react >= 2) snprintf(val, sizeof(val), "%d %s", st->react[1].coeff, st->react[1].formula);
                else strcpy(val, "-- (none)");
            } else if (i == 2) {
                if (st->n_prod >= 1) snprintf(val, sizeof(val), "%d %s", st->prod[0].coeff, st->prod[0].formula);
                else strcpy(val, "-- (none)");
            } else if (i == 3) {
                if (st->n_prod >= 2) snprintf(val, sizeof(val), "%d %s", st->prod[1].coeff, st->prod[1].formula);
                else strcpy(val, "-- (none)");
            } else {
                strcpy(val, st->speed == STEP_SLOW ? "SLOW (RDS)" : (st->speed == STEP_FAST_EQ ? "FAST EQ (" SYM_EQUIL ")" : "FAST"));
            }
            gfx_draw_text_right(302, y + 6, &FONT_UI_11B, val, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }
        gfx_draw_text(12, 192, &FONT_UI_11, "L/R: Pick Species   +/-: Coeff (1-3)   ENTER: Done", RAMP_MUTED_WHITE);
        ui_draw_softkeys("Done", "PrevSp", "NextSp", "Coeff+", "Home", 0);
        return;
    }

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

    gfx_fill_rrect(10, 102, 300, 62, 4, COL_BLUE_BG);
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
            ui_draw_pill(bx, 170, b, COL_AMBER_BG, RAMP_AMBER_BG);
            bx += gfx_text_width(&FONT_UI_11B, b) + 14;
        }
        if (m->is_catalyst[i]) {
            char b[28]; snprintf(b, sizeof(b), "Cat: %s", m->species_name[i]);
            ui_draw_pill(bx, 170, b, COL_GREEN_BG, RAMP_GREEN_BG);
            bx += gfx_text_width(&FONT_UI_11B, b) + 14;
        }
    }
    gfx_draw_text(12, 196, &FONT_UI_11, "UP/DN: pick step  ENTER: edit species  L/R: speed", RAMP_MUTED_WHITE);
    ui_draw_softkeys("Edit", "Speed", "2/3Stp", "Examp", "Home", -1);
}

/* ============================================================================
 * Screen 5: Arrhenius (2-Pt | ln k vs 1/T Plot | Energy Profile) — Inline!
 * ============================================================================ */
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
            int y = 74 + i * 23;
            bool sel = (app->cursor_r == i);
            bool unk = (a->unknown == i);
            if (sel) gfx_fill_rrect(10, y, 300, 20, 3, COL_BLUE_BG);
            gfx_draw_text(18, y + 3, &FONT_UI_11B, labs[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            if (unk) ui_draw_pill(116, y + 2, "SOLVED", COL_GREEN_BG, RAMP_GREEN_BG);
            draw_inline_value(302, y + 3, vals[i], sel && app->editing, app,
                              unk ? RAMP_GREEN_WHITE : (sel ? RAMP_INK_BLUE : RAMP_INK_WHITE));
        }
        gfx_draw_text(12, 196, &FONT_UI_11, "Type # + ENTER   APPS: mark unknown to solve", RAMP_MUTED_WHITE);
        ui_draw_softkeys("2-Pt", "Plot", "Energy", app->arrh_celsius ? "-> K" : "-> C", "Home", 0);
    } else if (app->sub_mode == 1) {
        const ArrhTableState *at = &app->arrh_tbl;
        ui_draw_topbar("Arrhenius Plot", "ln k vs 1/T");

        double ix[MAX_ARRH_ROWS], ly[MAX_ARRH_ROWS];
        for (int i = 0; i < at->num_rows; i++) { ix[i] = 1.0 / at->T[i]; ly[i] = log(at->k[i]); }
        draw_mini_plot(10, 26, 138, 108, ix, ly, at->num_rows, &at->reg, "ln(k) vs 1/T", true);

        gfx_fill_rrect(156, 26, 154, 108, 4, COL_BLUE_BG);
        gfx_draw_text(164, 31, &FONT_UI_11B, "Linear Fit Results", RAMP_ACCENT_BLUE);
        char e1[48], a1[48], k1[48];
        snprintf(e1, sizeof(e1), "~E~_{a} = %.1f kJ", at->Ea_kj);
        math_draw(164, 50, e1, 14, RAMP_INK_BLUE, COL_INK);
        fmt_sci_math(at->A_factor, a1, sizeof(a1), 2);
        char al[56]; snprintf(al, sizeof(al), "~A~ = %s", a1);
        math_draw(164, 72, al, 14, RAMP_INK_BLUE, COL_INK);
        fmt_sci_math(at->eval_k, k1, sizeof(k1), 2);
        char kl[96]; snprintf(kl, sizeof(kl), "~k~(%.0fK)=%s", at->eval_T, k1);
        math_draw(164, 96, kl, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        /* Inline editable (T, k) rows below plot */
        for (int r = 0; r < 3; r++) {
            int y = 140 + r * 20;
            bool s0 = (app->cursor_r == r && app->cursor_c == 0);
            bool s1 = (app->cursor_r == r && app->cursor_c == 1);
            char pl[16]; snprintf(pl, sizeof(pl), "Pt %d  T:", r + 1);
            gfx_draw_text(14, y + 3, &FONT_UI_11, pl, (app->cursor_r == r) ? RAMP_ACCENT_WHITE : RAMP_MUTED_WHITE);
            draw_inline_value(136, y + 3, at->T[r], s0 && app->editing, app, s0 ? RAMP_ACCENT_WHITE : RAMP_INK_WHITE);
            gfx_draw_text(154, y + 3, &FONT_UI_11, "k:", (app->cursor_r == r) ? RAMP_ACCENT_WHITE : RAMP_MUTED_WHITE);
            draw_inline_value(302, y + 3, at->k[r], s1 && app->editing, app, s1 ? RAMP_ACCENT_WHITE : RAMP_INK_WHITE);
        }
        ui_draw_softkeys("2-Pt", "Plot", "Energy", "EvalT", "Home", 1);
    } else {
        const CatalystState *c = &app->cat;
        ui_draw_topbar("Energy Profile", c->delta_E_kj < 0 ? "EXOTHERMIC" : "ENDOTHERMIC");

        int px = 12, py = 26, pw = 296, ph = 94;
        gfx_fill_rrect(px, py, pw, ph, 4, COL_WHITE);
        gfx_draw_rrect(px, py, pw, ph, 4, COL_BORDER);
        gfx_vline(px + 20, py + 8, ph - 22, COL_INK);
        gfx_hline(px + 20, py + ph - 14, pw - 32, COL_INK);
        gfx_draw_text(px + 6, py + 10, &FONT_UI_11B, "E", RAMP_MUTED_WHITE);
        gfx_draw_text_right(px + pw - 14, py + ph - 13, &FONT_UI_11, "Rxn Progress", RAMP_MUTED_WHITE);

        int y_r = (c->delta_E_kj < 0) ? (py + 52) : (py + 68);
        int y_p = (c->delta_E_kj < 0) ? (py + 72) : (py + 46);
        int y_pk_un = py + 14, y_pk_cat = py + 32;

        int prev_x = px + 28, prev_yu = y_r, prev_yc = y_r;
        for (int i = 1; i <= 40; i++) {
            double u = (double)i / 40.0;
            int gx = px + 28 + (int)(u * 246.0);
            double base = (double)y_r + (double)(y_p - y_r) / (1.0 + exp(-12.0 * (u - 0.5)));
            double hump = exp(-28.0 * (u - 0.5) * (u - 0.5));
            int yu = (int)(base - hump * ((double)y_r - (double)y_pk_un));
            int yc = (int)(base - hump * ((double)y_r - (double)y_pk_cat));
            gfx_line(prev_x, prev_yu, gx, yu, COL_RED);
            if ((i & 1) == 0) gfx_line(prev_x, prev_yc, gx, yc, COL_GREEN);
            prev_x = gx; prev_yu = yu; prev_yc = yc;
        }
        gfx_dashed_line(px + 28, y_r, px + 266, y_r, COL_BORDER);
        gfx_draw_text(px + 30, y_r - 12, &FONT_UI_11B, "R", RAMP_INK_WHITE);
        gfx_draw_text(px + 258, y_p - 12, &FONT_UI_11B, "P", RAMP_INK_WHITE);
        gfx_draw_text(px + 158, y_pk_un - 2, &FONT_UI_11B, "Uncat", RAMP_RED_WHITE);
        gfx_draw_text(px + 158, y_pk_cat - 2, &FONT_UI_11B, "Cat", RAMP_GREEN_WHITE);

        /* Inline editable Ea / dE / T + live speedup & Ea,rev! */
        const char *clab[4] = { "Ea(un)", "Ea(cat)", "Delta E", "Temp K" };
        double cval[4] = { c->Ea_uncat_kj, c->Ea_cat_kj, c->delta_E_kj, c->T_kelvin };
        for (int i = 0; i < 4; i++) {
            int col = i % 2, row = i / 2;
            int bx = 12 + col * 150, by = 125 + row * 22;
            bool sel = (app->cursor_r == i);
            if (sel) gfx_fill_rrect(bx, by, 146, 20, 3, COL_BLUE_BG);
            gfx_draw_text(bx + 6, by + 3, &FONT_UI_11B, clab[i], sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            draw_inline_value(bx + 140, by + 3, cval[i], sel && app->editing, app, sel ? RAMP_INK_BLUE : RAMP_INK_WHITE);
        }

        gfx_fill_rrect(12, 172, 296, 38, 4, COL_BLUE_BG);
        char sb[36], l1[96];
        fmt_sci_math(c->speedup_ratio, sb, sizeof(sb), 3);
        snprintf(l1, sizeof(l1), "~E~_{a,rev}=%.0f kJ    ~k~_{cat}/~k~_{un}=%s", c->Ea_rev_kj, sb);
        math_draw_center(160, 182, l1, 14, RAMP_ACCENT_BLUE, COL_ACCENT);

        ui_draw_softkeys("2-Pt", "Plot", "Energy", "Reset", "Home", 2);
    }
}

/* ============================================================================
 * Screen 6: FIXED Chart 12.6 & Visual Reference Graphs (ZERO Overlap!)
 * ============================================================================ */
static void render_ref_graphs(const AppState *app) {
    const char *pnames[4] = { "TABLE 12.6", "MAXWELL-BOLTZ.", "MULTI-STEP RDS", "PROOF PLOTS" };
    ui_draw_topbar("Chart & Graphs", pnames[app->ref_page]);

    if (app->ref_page == 0) {
        /* FIXED Table 12.6: 3 spacious horizontal order cards -> ZERO overlap! */
        const char *hdrs[3] = { "ZERO ORDER (n = 0)", "FIRST ORDER (n = 1)", "SECOND ORDER (n = 2)" };
        const char *eqs[3]  = { "[A]_{~t~} = " SYM_MINUS "~kt~ + [A]_{0}",
                                "ln[A]_{~t~} = " SYM_MINUS "~kt~ + ln[A]_{0}",
                                "1/[A]_{~t~} = ~kt~ + 1/[A]_{0}" };
        const char *hls[3]  = { "~t~_{1/2} = [A]_{0} / 2~k~",
                                "~t~_{1/2} = 0.693 / ~k~",
                                "~t~_{1/2} = 1 / (~k~[A]_{0})" };
        const char *meta[3] = { "Plot: [A] vs t (m=-k)    k: mol/(L" SYM_DOT "s)",
                                "Plot: ln[A] vs t (m=-k)  k: s^-1 (const t1/2)",
                                "Plot: 1/[A] vs t (m=+k)  k: L/(mol" SYM_DOT "s)" };

        for (int i = 0; i < 3; i++) {
            int y = 26 + i * 62;
            uint8_t bg = (i == 1) ? COL_BLUE_BG : COL_WHITE;
            uint8_t rm = (i == 1) ? RAMP_INK_BLUE : RAMP_INK_WHITE;
            uint8_t am = (i == 1) ? RAMP_ACCENT_BLUE : RAMP_ACCENT_WHITE;
            uint8_t mm = (i == 1) ? RAMP_MUTED_BLUE : RAMP_MUTED_WHITE;
            gfx_fill_rrect(8, y, 304, 57, 4, bg);
            gfx_draw_rrect(8, y, 304, 57, 4, (i == 1) ? COL_ACCENT : COL_BORDER);
            gfx_draw_text(16, y + 4, &FONT_UI_11B, hdrs[i], am);
            math_draw_right(304, y + 3, hls[i], 14, rm, COL_INK);
            math_draw(16, y + 21, eqs[i], 14, rm, COL_INK);
            gfx_draw_text(16, y + 40, &FONT_UI_11, meta[i], mm);
        }
    } else if (app->ref_page == 1) {
        /* Maxwell-Boltzmann Energy Distribution */
        int px = 14, py = 28, pw = 292, ph = 114;
        gfx_fill_rrect(px, py, pw, ph, 4, COL_WHITE);
        gfx_draw_rrect(px, py, pw, ph, 4, COL_BORDER);
        gfx_vline(px + 22, py + 10, ph - 26, COL_INK);
        gfx_hline(px + 22, py + ph - 16, pw - 34, COL_INK);
        gfx_draw_text(px + 4, py + 12, &FONT_UI_11B, "N", RAMP_MUTED_WHITE);
        gfx_draw_text_right(px + pw - 14, py + ph - 14, &FONT_UI_11, "Kinetic Energy", RAMP_MUTED_WHITE);

        int base_y = py + ph - 16, ea_x = px + 186;
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
                gfx_pixel(gx, y1, COL_ACCENT); gfx_pixel(gx, y2, COL_RED);
            }
            p1x = gx; p1y = y1; p2x = gx; p2y = y2;
        }
        gfx_draw_text(px + 72, py + 24, &FONT_UI_11B, "T1", RAMP_ACCENT_WHITE);
        gfx_draw_text(px + 130, py + 52, &FONT_UI_11B, "T2 > T1", RAMP_RED_WHITE);

        gfx_fill_rrect(14, 148, 292, 60, 4, COL_BLUE_BG);
        gfx_draw_text(22, 154, &FONT_UI_11B, "Higher T " SYM_ARROW " shifts right & flattens peak", RAMP_ACCENT_BLUE);
        gfx_draw_text(22, 171, &FONT_UI_11, "Ea stays FIXED (only a catalyst lowers Ea!).", RAMP_INK_BLUE);
        gfx_draw_text(22, 187, &FONT_UI_11, "Shaded area >= Ea grows exponentially -> larger k.", RAMP_MUTED_BLUE);
    } else if (app->ref_page == 2) {
        /* Multi-Step Energy Profile (RDS = tallest hump) */
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
            int gy = py + 72 + (int)(16.0 * u) - (int)(h1 + h2);
            gfx_line(prev_x, prev_y, gx, gy, COL_ACCENT);
            prev_x = gx; prev_y = gy;
        }
        gfx_draw_text(px + 28, py + 58, &FONT_UI_11B, "R", RAMP_INK_WHITE);
        gfx_draw_text(px + 114, py + 62, &FONT_UI_11B, "Int", RAMP_AMBER_BG);
        gfx_draw_text(px + 168, py + 14, &FONT_UI_11B, "Slow (RDS)", RAMP_RED_WHITE);
        gfx_draw_text(px + 258, py + 74, &FONT_UI_11B, "P", RAMP_INK_WHITE);

        gfx_fill_rrect(14, 148, 292, 60, 4, COL_BLUE_BG);
        gfx_draw_text(22, 154, &FONT_UI_11B, "2 humps = 2-step mechanism (valley = Interm.)", RAMP_ACCENT_BLUE);
        gfx_draw_text(22, 171, &FONT_UI_11, "Slow step (RDS) ALWAYS has the largest Ea.", RAMP_INK_BLUE);
        gfx_draw_text(22, 187, &FONT_UI_11, "Delta E = E(P) - E(R) does NOT affect rate!", RAMP_MUTED_BLUE);
    } else {
        /* 3 Proof Plots Visual Reference */
        double tx[4] = { 0, 1, 2, 3 };
        double d0[4] = { 4, 3, 2, 1 };
        double d1[4] = { 4, 3, 2, 1 };
        double d2[4] = { 1, 2, 3, 4 };
        LinReg r0 = { -1, 4, 1.0 }, r1 = { -1, 4, 1.0 }, r2 = { 1, 1, 1.0 };
        draw_mini_plot(8,   28, 98, 108, tx, d0, 4, &r0, "0th: [A] vs t",   true);
        draw_mini_plot(111, 28, 98, 108, tx, d1, 4, &r1, "1st: ln[A]",      true);
        draw_mini_plot(214, 28, 98, 108, tx, d2, 4, &r2, "2nd: 1/[A]",      true);

        gfx_fill_rrect(8, 142, 304, 66, 4, COL_BLUE_BG);
        gfx_draw_text(16, 148, &FONT_UI_11B, "Straight-Line Proof Rules & Half-Life:", RAMP_ACCENT_BLUE);
        gfx_draw_text(16, 165, &FONT_UI_11, "0th: m = -k, t1/2 HALVES    1st: m = -k, t1/2 CONST", RAMP_INK_BLUE);
        gfx_draw_text(16, 182, &FONT_UI_11, "2nd: m = +k, t1/2 DOUBLES   Arrh: ln k vs 1/T (-Ea/R)", RAMP_INK_BLUE);
    }
    ui_draw_softkeys("Table", "Maxw.", "Energy", "Plots", "Home", app->ref_page);
}

/* ============================================================================
 * Screen 7: 12-Topic AP Chem Theories & FRQ Encyclopedia (Def | HowTo | Ex.)
 * ============================================================================ */
static void render_theory(const AppState *app) {
    if (app->theory_list) {
        char st[24]; snprintf(st, sizeof(st), "%d / %d TOPICS", app->theory_idx + 1, NUM_THEORIES);
        ui_draw_topbar("Theories & FRQ", st);

        int vis = 7;
        int start = (app->theory_idx >= vis) ? (app->theory_idx - vis + 1) : 0;
        for (int vi = 0; vi < vis && (start + vi) < NUM_THEORIES; vi++) {
            int idx = start + vi;
            int y = 25 + vi * 27;
            bool sel = (idx == app->theory_idx);
            if (sel) {
                gfx_fill_rrect(8, y, 304, 24, 3, COL_BLUE_BG);
                gfx_fill_rect(8, y + 3, 3, 18, COL_ACCENT);
            } else {
                gfx_hline(14, y + 25, 292, COL_GRID);
            }
            char nb[8]; snprintf(nb, sizeof(nb), "%d", idx + 1);
            gfx_fill_rrect(16, y + 5, 18, 14, 2, sel ? COL_ACCENT : COL_GRAY_BG);
            gfx_draw_text_center(25, y + 5, &FONT_UI_11B, nb, sel ? RAMP_WHITE_ACCENT : RAMP_MUTED_WHITE);
            gfx_draw_text(40, y + 5, &FONT_UI_11B, THEORY_DB[idx].title, sel ? RAMP_ACCENT_BLUE : RAMP_INK_WHITE);
            gfx_draw_text_right(304, y + 5, &FONT_UI_11, THEORY_DB[idx].section, sel ? RAMP_INK_BLUE : RAMP_MUTED_WHITE);
        }
        ui_draw_softkeys("Read", "Def", "HowTo", "Ex.", "Home", 0);
        return;
    }

    const TheoryTopic *t = &THEORY_DB[app->theory_idx];
    char hdr[32]; snprintf(hdr, sizeof(hdr), "%s  (%d/%d)", t->section, app->theory_idx + 1, NUM_THEORIES);
    ui_draw_topbar(t->title, hdr);

    /* 3 clean pill headers */
    const char *tabs[3] = { "1. Textbook Def", "2. How to Use", "3. Example" };
    for (int i = 0; i < 3; i++) {
        int bx = 10 + i * 102;
        bool act = (app->theory_tab == i);
        gfx_fill_rrect(bx, 26, 96, 20, 3, act ? COL_ACCENT : COL_GRAY_BG);
        gfx_draw_text_center(bx + 48, 29, &FONT_UI_11B, tabs[i], act ? RAMP_WHITE_ACCENT : RAMP_MUTED_WHITE);
    }

    /* Spacious content card */
    gfx_fill_rrect(10, 52, 300, 154, 4, (app->theory_tab == 2) ? COL_BLUE_BG : COL_WHITE);
    gfx_draw_rrect(10, 52, 300, 154, 4, (app->theory_tab == 2) ? COL_ACCENT : COL_BORDER);
    const char * const *lines = (app->theory_tab == 0) ? t->def_lines
                              : (app->theory_tab == 1) ? t->how_lines : t->ex_lines;
    uint8_t rmp = (app->theory_tab == 2) ? RAMP_INK_BLUE : RAMP_INK_WHITE;
    for (int i = 0; i < 6; i++) {
        if (lines[i] && lines[i][0]) {
            gfx_draw_text(18, 60 + i * 23, &FONT_UI_11, lines[i], rmp);
        }
    }

    ui_draw_softkeys("List", "Def", "HowTo", "Ex.", "Next", app->theory_tab + 1);
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
        case SCR_REF_GRAPHS: render_ref_graphs(app); break;
        case SCR_THEORY:     render_theory(app); break;
    }
    hal_swap_buffers();
}

/* ============================================================================
 * Unified Zero-Popup Responsive Key Dispatcher
 * ============================================================================ */
bool app_handle_key(AppState *app, int k_in) {
    KeyCode k = (KeyCode)k_in;
    if (k == KEY_QUIT) return false;

    /* 1) Handle active inline text editing first (zero popups!) */
    if (app->editing) {
        if (k == KEY_CLEAR) { app->editing = false; return true; }
        if (k == KEY_ENTER) { commit_inline_edit(app); return true; }
        if (k == KEY_DEL) {
            app->edit_first_key = false;
            if (app->edit_len > 0) app->edit_buf[--app->edit_len] = '\0';
            return true;
        }
        char ch = key_to_numchar(k);
        if (k == KEY_MINUS) ch = '-';
        if (ch != 0) {
            if (app->edit_first_key) { app->edit_len = 0; app->edit_buf[0] = '\0'; app->edit_first_key = false; }
            if (app->edit_len < 20) { app->edit_buf[app->edit_len++] = ch; app->edit_buf[app->edit_len] = '\0'; }
            return true;
        }
        /* Arrow key commits current edit and moves immediately! */
        if (k == KEY_UP || k == KEY_DOWN || k == KEY_LEFT || k == KEY_RIGHT) {
            commit_inline_edit(app);
        } else {
            return true;
        }
    }

    /* 2) Direct digit typing on any editable screen starts inline edit immediately! */
    if (app->screen != SCR_HOME && app->screen != SCR_MECH &&
        app->screen != SCR_REF_GRAPHS && app->screen != SCR_THEORY) {
        char ch = key_to_numchar(k);
        if (ch != 0) {
            bool dummy = false;
            if (get_active_field_ptr(app, &dummy) != NULL) {
                begin_inline_edit(app, ch);
                return true;
            }
        }
    }

    /* 3) Home Menu (Row List 1-7) */
    if (app->screen == SCR_HOME) {
        if (k == KEY_CLEAR) return false;
        if (k == KEY_UP   && app->home_sel > 0) app->home_sel--;
        if (k == KEY_DOWN && app->home_sel < 6) app->home_sel++;
        if (k >= KEY_1 && k <= KEY_7) { app->home_sel = (int)(k - KEY_1); k = KEY_ENTER; }
        if (k == KEY_F1 || k == KEY_ENTER) {
            app->screen = (ScreenId)(app->home_sel + 1);
            app->sub_mode = 0; app->cursor_r = 0; app->cursor_c = 0;
            app->mech_editing = false; app->theory_list = true;
            return true;
        }
        if (k == KEY_F2) { app->screen = SCR_RATES; app->sub_mode = 0; app->cursor_r = 0; return true; }
        if (k == KEY_F3) { app->screen = SCR_INIT_RATES; app->sub_mode = 0; app->cursor_r = 0; app->cursor_c = 0; return true; }
        if (k == KEY_F4) { app->screen = SCR_REF_GRAPHS; app->ref_page = 0; return true; }
        if (k == KEY_F5) { app->screen = SCR_THEORY; app->theory_list = true; return true; }
        return true;
    }

    /* 4) Screen-specific contextual softkeys & navigation */
    if (app->screen == SCR_RATES) {
        if (k == KEY_F5 || k == KEY_CLEAR) { app->screen = SCR_HOME; return true; }
        if (k == KEY_F2) { app->sub_mode = 0; app->cursor_r = 0; return true; }
        if (k == KEY_F3) { app->sub_mode = 1; app->cursor_r = 0; app->cursor_c = 0; return true; }
        if (k == KEY_F4) { app->sub_mode = 2; app->cursor_r = 0; return true; }
        if (app->sub_mode == 0) {
            if (k == KEY_F1) { app->rates.is_reactant = !app->rates.is_reactant; kinetics_rate_calc(&app->rates); }
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 4) app->cursor_r++;
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else if (app->sub_mode == 1) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 2) app->cursor_r++;
            if (k == KEY_LEFT) app->cursor_c = 0;
            if (k == KEY_RIGHT) app->cursor_c = 1;
            if (k == KEY_F1) kinetics_stoich_rates(&app->stoich);
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else {
            if (k == KEY_LEFT)  app->k_unit_order -= 0.5;
            if (k == KEY_RIGHT || k == KEY_F1) app->k_unit_order += 0.5;
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        }
    } else if (app->screen == SCR_INIT_RATES) {
        InitialRatesState *ir = &app->init_rates;
        if (k == KEY_F5) { app->screen = SCR_HOME; return true; }
        if (app->sub_mode == 0) {
            if (k == KEY_CLEAR) { app->screen = SCR_HOME; return true; }
            int cols = ir->num_reactants + 1;
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < ir->num_rows - 1) app->cursor_r++;
            if (k == KEY_LEFT && app->cursor_c > 0) app->cursor_c--;
            if (k == KEY_RIGHT && app->cursor_c < cols - 1) app->cursor_c++;
            if (k == KEY_F1 || k == KEY_APPS) { kinetics_solve_initial_rates(ir); app->sub_mode = 1; app->step_idx = 0; app->cursor_c = 0; }
            if ((k == KEY_F2 || k == KEY_PLUS) && ir->num_rows < MAX_IR_ROWS) {
                int nr = ir->num_rows++;
                for (int j = 0; j < MAX_IR_REACT; j++) ir->conc[nr][j] = ir->conc[nr - 1][j];
                ir->rate[nr] = ir->rate[nr - 1];
                kinetics_solve_initial_rates(ir);
            }
            if ((k == KEY_F3 || k == KEY_MINUS) && ir->num_rows > 2) {
                ir->num_rows--;
                if (app->cursor_r >= ir->num_rows) app->cursor_r = ir->num_rows - 1;
                kinetics_solve_initial_rates(ir);
            }
            if (k == KEY_F4 || k == KEY_MODE) {
                ir->num_reactants = (ir->num_reactants % 3) + 1;
                if (app->cursor_c > ir->num_reactants) app->cursor_c = ir->num_reactants;
                kinetics_solve_initial_rates(ir);
            }
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else {
            if (k == KEY_CLEAR || k == KEY_F1) { app->sub_mode = 0; return true; }
            if ((k == KEY_UP || k == KEY_F2) && app->step_idx > 0) app->step_idx--;
            if ((k == KEY_DOWN || k == KEY_F3) && app->step_idx < 2) app->step_idx++;
            if (app->step_idx == 2) {
                if (k == KEY_LEFT && app->cursor_c > 0) app->cursor_c--;
                if (k == KEY_RIGHT && app->cursor_c < ir->num_reactants - 1) app->cursor_c++;
                if (k == KEY_ENTER || k == KEY_F4) begin_inline_edit(app, 0);
            } else if (k == KEY_F4) {
                app->step_idx = 2; app->cursor_c = 0; begin_inline_edit(app, 0);
            }
        }
    } else if (app->screen == SCR_INTEGRATED) {
        if (k == KEY_F5 || k == KEY_CLEAR) { app->screen = SCR_HOME; return true; }
        if (k == KEY_F1) { app->sub_mode = 0; app->cursor_r = 0; return true; }
        if (k == KEY_F2) { app->sub_mode = 1; app->cursor_r = 0; return true; }
        if (k == KEY_F3) { app->sub_mode = 2; app->cursor_r = 0; app->cursor_c = 0; return true; }
        if (k == KEY_F4) { app->sub_mode = 3; app->cursor_r = 0; return true; }
        if (app->sub_mode == 1) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 3) app->cursor_r++;
            if (k == KEY_LEFT)  { app->int_eq.order = (app->int_eq.order + 3) % 4; kinetics_integrated_solve(&app->int_eq); }
            if (k == KEY_RIGHT) { app->int_eq.order = (app->int_eq.order + 1) % 4; kinetics_integrated_solve(&app->int_eq); }
            if (k == KEY_APPS)  { app->int_eq.unknown = app->cursor_r; kinetics_integrated_solve(&app->int_eq); }
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else if (app->sub_mode == 2) {
            IntegratedTableState *it = &app->int_table;
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < it->num_rows - 1) app->cursor_r++;
            if (k == KEY_LEFT) app->cursor_c = 0;
            if (k == KEY_RIGHT) app->cursor_c = 1;
            if (k == KEY_PLUS && it->num_rows < MAX_INT_ROWS) {
                int r = it->num_rows++;
                it->t[r] = it->t[r - 1] + 2.0; it->c[r] = it->c[r - 1] * 0.5;
                kinetics_regress_order(it);
            }
            if (k == KEY_MINUS && it->num_rows > 3) {
                it->num_rows--;
                if (app->cursor_r >= it->num_rows) app->cursor_r = it->num_rows - 1;
                kinetics_regress_order(it);
            }
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else if (app->sub_mode == 3) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 2) app->cursor_r++;
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        }
    } else if (app->screen == SCR_MECH) {
        if (k == KEY_F5) { app->mech_editing = false; app->screen = SCR_HOME; return true; }
        if (app->mech_editing) {
            MechStep *st = &app->mech.steps[app->cursor_r];
            if (k == KEY_CLEAR || k == KEY_ENTER || k == KEY_F1) {
                app->mech_editing = false; kinetics_analyze_mech(&app->mech); return true;
            }
            if (k == KEY_UP && app->mech_field > 0) app->mech_field--;
            if (k == KEY_DOWN && app->mech_field < 4) app->mech_field++;
            if (k == KEY_LEFT || k == KEY_RIGHT || k == KEY_F2 || k == KEY_F3) {
                int d = (k == KEY_RIGHT || k == KEY_F3) ? 1 : -1;
                if (app->mech_field == 4) {
                    st->speed = (StepSpeed)(((int)st->speed + 3 + d) % 3);
                } else {
                    int is_p = (app->mech_field >= 2);
                    int slot = app->mech_field % 2;
                    int *cnt = is_p ? &st->n_prod : &st->n_react;
                    MechTerm *arr = is_p ? st->prod : st->react;
                    int cur_id = (slot < *cnt) ? find_pool_idx(arr[slot].formula) : 0;
                    int nid = (cur_id + NUM_SPECIES_POOL + d) % NUM_SPECIES_POOL;
                    if (nid == 0) {
                        if (slot == 0 && *cnt == 2) { arr[0] = arr[1]; *cnt = 1; }
                        else if (slot < *cnt) *cnt = slot;
                    } else {
                        if (slot >= *cnt) { *cnt = slot + 1; arr[slot].coeff = 1; }
                        strcpy(arr[slot].formula, SPECIES_POOL[nid]);
                    }
                }
                kinetics_analyze_mech(&app->mech);
            }
            if (k == KEY_PLUS || k == KEY_MINUS || k == KEY_F4) {
                int d = (k == KEY_MINUS) ? -1 : 1;
                if (app->mech_field < 4) {
                    int is_p = (app->mech_field >= 2);
                    int slot = app->mech_field % 2;
                    int cnt = is_p ? st->n_prod : st->n_react;
                    MechTerm *arr = is_p ? st->prod : st->react;
                    if (slot < cnt) {
                        arr[slot].coeff = ((arr[slot].coeff - 1 + 3 + d) % 3) + 1;
                        kinetics_analyze_mech(&app->mech);
                    }
                }
            }
            return true;
        }
        if (k == KEY_CLEAR) { app->screen = SCR_HOME; return true; }
        if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
        if (k == KEY_DOWN && app->cursor_r < app->mech.num_steps - 1) app->cursor_r++;
        if (k == KEY_ENTER || k == KEY_F1) { app->mech_editing = true; app->mech_field = 0; }
        if (k == KEY_LEFT || k == KEY_RIGHT || k == KEY_F2) {
            int d = (k == KEY_LEFT) ? -1 : 1;
            MechStep *st = &app->mech.steps[app->cursor_r];
            st->speed = (StepSpeed)(((int)st->speed + 3 + d) % 3);
            kinetics_analyze_mech(&app->mech);
        }
        if (k == KEY_F3 || k == KEY_PLUS || k == KEY_MINUS) {
            if (app->mech.num_steps == 2 && k != KEY_MINUS) {
                app->mech.num_steps = 3;
                MechStep *st = &app->mech.steps[2];
                st->speed = STEP_FAST; st->n_react = 1; st->n_prod = 1;
                strcpy(st->react[0].formula, "I1"); st->react[0].coeff = 1;
                strcpy(st->prod[0].formula, "C");   st->prod[0].coeff = 1;
            } else if (app->mech.num_steps == 3 && k != KEY_PLUS) {
                app->mech.num_steps = 2;
                if (app->cursor_r >= 2) app->cursor_r = 1;
            }
            kinetics_analyze_mech(&app->mech);
        }
        if (k == KEY_F4 || k == KEY_MODE) { app_load_mech_preset(app, (app->mech_preset + 1) % 5); app->cursor_r = 0; }
    } else if (app->screen == SCR_ARRHENIUS) {
        if (k == KEY_F5 || k == KEY_CLEAR) { app->screen = SCR_HOME; return true; }
        if (k == KEY_F1) { app->sub_mode = 0; app->cursor_r = 0; return true; }
        if (k == KEY_F2) { app->sub_mode = 1; app->cursor_r = 0; app->cursor_c = 0; return true; }
        if (k == KEY_F3) { app->sub_mode = 2; app->cursor_r = 0; return true; }
        if (app->sub_mode == 0) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 4) app->cursor_r++;
            if (k == KEY_F4 || k == KEY_LEFT || k == KEY_RIGHT) app->arrh_celsius = !app->arrh_celsius;
            if (k == KEY_APPS) { app->arrh2.unknown = app->cursor_r; kinetics_arrhenius_2pt(&app->arrh2); }
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else if (app->sub_mode == 1) {
            if (k == KEY_UP && app->cursor_r > 0) app->cursor_r--;
            if (k == KEY_DOWN && app->cursor_r < 2) app->cursor_r++;
            if (k == KEY_LEFT) app->cursor_c = 0;
            if (k == KEY_RIGHT) app->cursor_c = 1;
            if (k == KEY_F4) { app->cursor_r = 3; begin_inline_edit(app, 0); }
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        } else {
            if (k == KEY_UP && app->cursor_r >= 2) app->cursor_r -= 2;
            if (k == KEY_DOWN && app->cursor_r < 2) app->cursor_r += 2;
            if (k == KEY_LEFT && (app->cursor_r % 2) == 1) app->cursor_r--;
            if (k == KEY_RIGHT && (app->cursor_r % 2) == 0) app->cursor_r++;
            if (k == KEY_F4) {
                app->cat.Ea_uncat_kj = 125.0; app->cat.Ea_cat_kj = 85.0;
                app->cat.delta_E_kj = -216.0; app->cat.T_kelvin = 298.0;
                kinetics_catalyst_calc(&app->cat);
            }
            if (k == KEY_ENTER) begin_inline_edit(app, 0);
        }
    } else if (app->screen == SCR_REF_GRAPHS) {
        if (k == KEY_F5 || k == KEY_CLEAR) { app->screen = SCR_HOME; return true; }
        if (k == KEY_F1) app->ref_page = 0;
        if (k == KEY_F2) app->ref_page = 1;
        if (k == KEY_F3) app->ref_page = 2;
        if (k == KEY_F4) app->ref_page = 3;
        if (k == KEY_LEFT)  app->ref_page = (app->ref_page + 3) % 4;
        if (k == KEY_RIGHT) app->ref_page = (app->ref_page + 1) % 4;
    } else if (app->screen == SCR_THEORY) {
        if (app->theory_list) {
            if (k == KEY_CLEAR || k == KEY_F5) { app->screen = SCR_HOME; return true; }
            if (k == KEY_UP && app->theory_idx > 0) app->theory_idx--;
            if (k == KEY_DOWN && app->theory_idx < NUM_THEORIES - 1) app->theory_idx++;
            if (k == KEY_ENTER || k == KEY_F1) { app->theory_list = false; app->theory_tab = 0; }
            if (k == KEY_F2) { app->theory_list = false; app->theory_tab = 0; }
            if (k == KEY_F3) { app->theory_list = false; app->theory_tab = 1; }
            if (k == KEY_F4) { app->theory_list = false; app->theory_tab = 2; }
        } else {
            if (k == KEY_CLEAR || k == KEY_F1) { app->theory_list = true; return true; }
            if (k == KEY_F2) app->theory_tab = 0;
            if (k == KEY_F3) app->theory_tab = 1;
            if (k == KEY_F4) app->theory_tab = 2;
            if (k == KEY_LEFT)  app->theory_tab = (app->theory_tab + 2) % 3;
            if (k == KEY_RIGHT) app->theory_tab = (app->theory_tab + 1) % 3;
            if (k == KEY_UP && app->theory_idx > 0) { app->theory_idx--; app->theory_tab = 0; }
            if (k == KEY_DOWN || k == KEY_F5) { app->theory_idx = (app->theory_idx + 1) % NUM_THEORIES; app->theory_tab = 0; }
        }
    }
    return true;
}
