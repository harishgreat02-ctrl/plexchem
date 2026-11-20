#include "kinetics.h"
#include "fonts_baked.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Helper: Linear Regression (y = slope * x + intercept, R^2)
 * ============================================================================ */
static bool linreg_fit(const double *x, const double *y, int n, LinReg *out) {
    if (n < 2) return false;
    double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0;
    for (int i = 0; i < n; i++) {
        sx += x[i];
        sy += y[i];
        sxx += x[i] * x[i];
        syy += y[i] * y[i];
        sxy += x[i] * y[i];
    }
    double denom = (double)n * sxx - sx * sx;
    if (fabs(denom) < 1e-30) return false;
    out->slope = ((double)n * sxy - sx * sy) / denom;
    out->intercept = (sy - out->slope * sx) / (double)n;

    double my = sy / (double)n;
    double ss_tot = 0, ss_res = 0;
    for (int i = 0; i < n; i++) {
        double yh = out->slope * x[i] + out->intercept;
        double d1 = y[i] - my;
        double d2 = y[i] - yh;
        ss_tot += d1 * d1;
        ss_res += d2 * d2;
    }
    if (ss_tot < 1e-30) out->r2 = 1.0;
    else out->r2 = 1.0 - ss_res / ss_tot;
    return true;
}

/* ============================================================================
 * Module 1: Reaction Rates & Stoichiometry
 * ============================================================================ */
void kinetics_rate_calc(RateCalcState *s) {
    double dt = s->t2 - s->t1;
    if (fabs(dt) < 1e-30 || s->coeff <= 0.0) {
        s->species_rate = 0.0;
        s->overall_rate = 0.0;
        return;
    }
    s->species_rate = (s->c2 - s->c1) / dt;
    s->overall_rate = (s->is_reactant ? -s->species_rate : s->species_rate) / s->coeff;
}

void kinetics_stoich_rates(StoichRateState *s) {
    int k = s->known_idx;
    if (k < 0 || k > 3 || s->coeffs[k] <= 0.0) return;
    s->overall_rate = fabs(s->known_rate) / s->coeffs[k];
    for (int i = 0; i < 4; i++) {
        if (s->coeffs[i] <= 0.0) { s->rates[i] = 0.0; continue; }
        double mag = s->overall_rate * s->coeffs[i];
        s->rates[i] = s->is_product[i] ? mag : -mag;
    }
}

void kinetics_format_k_units(double n, const char *tu, char *out, size_t sz) {
    /* Rate = M/tu = k * M^n => k has units M^(1-n) * tu^-1 */
    double p = 1.0 - n;
    if (fabs(p) < 1e-6) {
        snprintf(out, sz, "%s^{" SYM_MINUS "1}", tu);
    } else if (fabs(p - 1.0) < 1e-6) {
        snprintf(out, sz, "\\f{mol}{L" SYM_DOT "%s}", tu);
    } else if (fabs(p + 1.0) < 1e-6) {
        snprintf(out, sz, "\\f{L}{mol" SYM_DOT "%s}", tu);
    } else if (p < 0.0) {
        double ap = -p;
        if (fabs(ap - floor(ap + 0.5)) < 1e-6) {
            int ip = (int)floor(ap + 0.5);
            snprintf(out, sz, "\\f{L^{%d}}{mol^{%d}" SYM_DOT "%s}", ip, ip, tu);
        } else {
            snprintf(out, sz, "\\f{L^{%.1f}}{mol^{%.1f}" SYM_DOT "%s}", ap, ap, tu);
        }
    } else {
        if (fabs(p - floor(p + 0.5)) < 1e-6) {
            int ip = (int)floor(p + 0.5);
            snprintf(out, sz, "\\f{mol^{%d}}{L^{%d}" SYM_DOT "%s}", ip, ip, tu);
        } else {
            snprintf(out, sz, "\\f{mol^{%.1f}}{L^{%.1f}" SYM_DOT "%s}", p, p, tu);
        }
    }
}

/* ============================================================================
 * Module 2: Method of Initial Rates (Log-Linear Least Squares + Pair Finder)
 * ============================================================================ */
static bool solve_linear_system(double A[4][4], double b[4], int m, double x[4]) {
    for (int col = 0; col < m; col++) {
        int piv = col;
        double maxv = fabs(A[col][col]);
        for (int r = col + 1; r < m; r++) {
            if (fabs(A[r][col]) > maxv) { maxv = fabs(A[r][col]); piv = r; }
        }
        if (maxv < 1e-12) return false;
        if (piv != col) {
            for (int c = col; c < m; c++) {
                double tmp = A[col][c]; A[col][c] = A[piv][c]; A[piv][c] = tmp;
            }
            double tmp = b[col]; b[col] = b[piv]; b[piv] = tmp;
        }
        for (int r = col + 1; r < m; r++) {
            double f = A[r][col] / A[col][col];
            for (int c = col; c < m; c++) A[r][c] -= f * A[col][c];
            b[r] -= f * b[col];
        }
    }
    for (int i = m - 1; i >= 0; i--) {
        double sum = b[i];
        for (int c = i + 1; c < m; c++) sum -= A[i][c] * x[c];
        x[i] = sum / A[i][i];
    }
    return true;
}

static double snap_order(double v) {
    /* Snap cleanly to nearest 0.5 if within 0.08 (handles experimental noise) */
    double half = floor(v * 2.0 + 0.5) / 2.0;
    if (fabs(v - half) < 0.08) return half;
    return v;
}

bool kinetics_solve_initial_rates(InitialRatesState *s) {
    int nr = s->num_reactants;
    int N = s->num_rows;
    s->solved = false;
    if (nr < 1 || nr > MAX_IR_REACT || N < nr + 1 || N > MAX_IR_ROWS) return false;

    int m = nr + 1;
    double XtX[4][4];
    double Xty[4];
    memset(XtX, 0, sizeof(XtX));
    memset(Xty, 0, sizeof(Xty));

    for (int i = 0; i < N; i++) {
        if (s->rate[i] <= 0.0) return false;
        double row[4];
        row[0] = 1.0;
        for (int j = 0; j < nr; j++) {
            if (s->conc[i][j] <= 0.0) return false;
            row[j + 1] = log(s->conc[i][j]);
        }
        double y = log(s->rate[i]);
        for (int a = 0; a < m; a++) {
            Xty[a] += row[a] * y;
            for (int b = 0; b < m; b++) XtX[a][b] += row[a] * row[b];
        }
    }

    double beta[4] = {0, 0, 0, 0};
    if (!solve_linear_system(XtX, Xty, m, beta)) return false;

    s->overall_order = 0.0;
    for (int j = 0; j < nr; j++) {
        s->raw_order[j] = beta[j + 1];
        s->order[j] = snap_order(beta[j + 1]);
        s->overall_order += s->order[j];
    }

    /* Compute per-trial k and k_mean using snapped clean orders */
    double ksum = 0.0;
    for (int i = 0; i < N; i++) {
        double denom = 1.0;
        for (int j = 0; j < nr; j++) denom *= pow(s->conc[i][j], s->order[j]);
        s->k_trial[i] = s->rate[i] / denom;
        ksum += s->k_trial[i];
    }
    s->k_mean = ksum / (double)N;

    /* Find cleanest pair of rows for each reactant (holding others constant if possible) */
    for (int j = 0; j < nr; j++) {
        s->pair_r1[j] = 0;
        s->pair_r2[j] = 1;
        double best_score = -1.0;
        for (int a = 0; a < N; a++) {
            for (int b = a + 1; b < N; b++) {
                double ratio = s->conc[b][j] / s->conc[a][j];
                if (fabs(ratio - 1.0) < 0.05) continue;
                bool others_const = true;
                for (int o = 0; o < nr; o++) {
                    if (o == j) continue;
                    if (fabs(s->conc[b][o] / s->conc[a][o] - 1.0) > 0.02) others_const = false;
                }
                double score = (others_const ? 100.0 : 1.0) + fabs(log(ratio));
                if (score > best_score) {
                    best_score = score;
                    s->pair_r1[j] = a;
                    s->pair_r2[j] = b;
                }
            }
        }
    }

    /* Predict rate at pred_conc */
    double pr = s->k_mean;
    for (int j = 0; j < nr; j++) {
        if (s->pred_conc[j] > 0.0) pr *= pow(s->pred_conc[j], s->order[j]);
    }
    s->pred_rate = pr;
    s->solved = true;
    return true;
}

/* ============================================================================
 * Module 3: Integrated Rate Laws, Linear Regression Plots & Half-Life
 * ============================================================================ */
bool kinetics_regress_order(IntegratedTableState *s) {
    int n = s->num_rows;
    s->solved = false;
    if (n < 3 || n > MAX_INT_ROWS) return false;

    double y0[MAX_INT_ROWS], y1[MAX_INT_ROWS], y2[MAX_INT_ROWS];
    for (int i = 0; i < n; i++) {
        if (s->c[i] <= 0.0) return false;
        y0[i] = s->c[i];
        y1[i] = log(s->c[i]);
        y2[i] = 1.0 / s->c[i];
    }

    if (!linreg_fit(s->t, y0, n, &s->reg[0])) return false;
    if (!linreg_fit(s->t, y1, n, &s->reg[1])) return false;
    if (!linreg_fit(s->t, y2, n, &s->reg[2])) return false;

    int best = 0;
    if (s->reg[1].r2 > s->reg[best].r2) best = 1;
    if (s->reg[2].r2 > s->reg[best].r2) best = 2;
    s->best_order = best;

    if (best == 0) {
        s->k = -s->reg[0].slope;
        s->c0_fit = s->reg[0].intercept;
    } else if (best == 1) {
        s->k = -s->reg[1].slope;
        s->c0_fit = exp(s->reg[1].intercept);
    } else {
        s->k = s->reg[2].slope;
        s->c0_fit = (fabs(s->reg[2].intercept) > 1e-12) ? (1.0 / s->reg[2].intercept) : s->c[0];
    }
    s->solved = true;
    return true;
}

bool kinetics_integrated_solve(IntegratedEqState *s) {
    s->valid = false;
    if (s->order == 0) {
        /* [A]t = -kt + [A]0 */
        if (s->unknown == 0) s->A0 = s->At + s->k * s->t;
        else if (s->unknown == 1) s->At = s->A0 - s->k * s->t;
        else if (s->unknown == 2) { if (fabs(s->t) < 1e-30) return false; s->k = (s->A0 - s->At) / s->t; }
        else if (s->unknown == 3) { if (fabs(s->k) < 1e-30) return false; s->t = (s->A0 - s->At) / s->k; }
        if (s->k <= 0.0 || s->A0 <= 0.0) return false;
        s->t_half_1 = s->A0 / (2.0 * s->k);
        s->t_half_2 = s->t_half_1 * 0.5;
        s->t_half_3 = s->t_half_1 * 0.25;
    } else if (s->order == 1) {
        /* ln([A]t/[A]0) = -kt */
        if (s->unknown == 0) s->A0 = s->At * exp(s->k * s->t);
        else if (s->unknown == 1) s->At = s->A0 * exp(-s->k * s->t);
        else if (s->unknown == 2) {
            if (fabs(s->t) < 1e-30 || s->A0 <= 0.0 || s->At <= 0.0) return false;
            s->k = log(s->A0 / s->At) / s->t;
        } else if (s->unknown == 3) {
            if (fabs(s->k) < 1e-30 || s->A0 <= 0.0 || s->At <= 0.0) return false;
            s->t = log(s->A0 / s->At) / s->k;
        }
        if (s->k <= 0.0 || s->A0 <= 0.0) return false;
        s->t_half_1 = log(2.0) / s->k;
        s->t_half_2 = s->t_half_1;
        s->t_half_3 = s->t_half_1;
    } else if (s->order == 2) {
        /* 1/[A]t - 1/[A]0 = kt */
        if (s->unknown == 0) {
            if (s->At <= 0.0) return false;
            double inv = 1.0 / s->At - s->k * s->t;
            if (inv <= 0.0) return false;
            s->A0 = 1.0 / inv;
        } else if (s->unknown == 1) {
            if (s->A0 <= 0.0) return false;
            double inv = 1.0 / s->A0 + s->k * s->t;
            if (inv <= 0.0) return false;
            s->At = 1.0 / inv;
        } else if (s->unknown == 2) {
            if (fabs(s->t) < 1e-30 || s->A0 <= 0.0 || s->At <= 0.0) return false;
            s->k = (1.0 / s->At - 1.0 / s->A0) / s->t;
        } else if (s->unknown == 3) {
            if (fabs(s->k) < 1e-30 || s->A0 <= 0.0 || s->At <= 0.0) return false;
            s->t = (1.0 / s->At - 1.0 / s->A0) / s->k;
        }
        if (s->k <= 0.0 || s->A0 <= 0.0) return false;
        s->t_half_1 = 1.0 / (s->k * s->A0);
        s->t_half_2 = s->t_half_1 * 2.0;
        s->t_half_3 = s->t_half_1 * 4.0;
    } else {
        /* 3rd order (Zumdahl Q110): 1/(2[A]t^2) - 1/(2[A]0^2) = kt, t1/2 = 3/(2k[A]0^2) */
        if (s->unknown == 1) {
            if (s->A0 <= 0.0) return false;
            double inv = 1.0 / (2.0 * s->A0 * s->A0) + s->k * s->t;
            if (inv <= 0.0) return false;
            s->At = 1.0 / sqrt(2.0 * inv);
        } else if (s->unknown == 2) {
            if (fabs(s->t) < 1e-30 || s->A0 <= 0.0 || s->At <= 0.0) return false;
            s->k = (1.0 / (2.0 * s->At * s->At) - 1.0 / (2.0 * s->A0 * s->A0)) / s->t;
        } else if (s->unknown == 3) {
            if (fabs(s->k) < 1e-30 || s->A0 <= 0.0 || s->At <= 0.0) return false;
            s->t = (1.0 / (2.0 * s->At * s->At) - 1.0 / (2.0 * s->A0 * s->A0)) / s->k;
        } else {
            if (s->At <= 0.0) return false;
            double inv = 1.0 / (2.0 * s->At * s->At) - s->k * s->t;
            if (inv <= 0.0) return false;
            s->A0 = 1.0 / sqrt(2.0 * inv);
        }
        if (s->k <= 0.0 || s->A0 <= 0.0) return false;
        s->t_half_1 = 3.0 / (2.0 * s->k * s->A0 * s->A0);
        s->t_half_2 = s->t_half_1 * 4.0;
        s->t_half_3 = s->t_half_1 * 16.0;
    }
    s->pct_remaining = (s->A0 > 0.0) ? (100.0 * s->At / s->A0) : 0.0;
    s->valid = true;
    return true;
}

double kinetics_pseudo_k(double k_prime, double excess_conc, double excess_order) {
    if (excess_conc <= 0.0) return 0.0;
    return k_prime / pow(excess_conc, excess_order);
}

/* ============================================================================
 * Module 4: Reaction Mechanisms & Algebraic Intermediate Eliminator
 * ============================================================================ */
static int find_or_add_species(MechState *m, const char *name) {
    for (int i = 0; i < m->n_species; i++) {
        if (strcmp(m->species_name[i], name) == 0) return i;
    }
    if (m->n_species >= MAX_SPECIES) return -1;
    int idx = m->n_species++;
    strncpy(m->species_name[idx], name, 11);
    m->species_name[idx][11] = '\0';
    return idx;
}

static void format_formula_math(const char *raw, char *out, size_t sz) {
    /* Converts e.g. "NO2" -> "NO_{2}", "H2O2" -> "H_{2}O_{2}", "Br-" -> "Br^{-}" */
    size_t j = 0;
    for (size_t i = 0; raw[i] && j + 6 < sz; i++) {
        char c = raw[i];
        if (c >= '0' && c <= '9') {
            j += (size_t)snprintf(out + j, sz - j, "_{%c}", c);
        } else if (c == '+' || c == '-') {
            j += (size_t)snprintf(out + j, sz - j, "^{%s}", (c == '-') ? SYM_MINUS : "+");
        } else {
            out[j++] = c;
        }
    }
    out[j] = '\0';
}

bool kinetics_analyze_mech(MechState *m) {
    m->valid = false;
    m->n_species = 0;
    m->slow_step_idx = -1;
    memset(m->net_coeff, 0, sizeof(m->net_coeff));
    memset(m->is_intermediate, 0, sizeof(m->is_intermediate));
    memset(m->is_catalyst, 0, sizeof(m->is_catalyst));
    memset(m->rate_exp, 0, sizeof(m->rate_exp));

    int first_prod_step[MAX_SPECIES];
    int first_react_step[MAX_SPECIES];
    for (int i = 0; i < MAX_SPECIES; i++) {
        first_prod_step[i] = 99;
        first_react_step[i] = 99;
    }

    for (int s = 0; s < m->num_steps; s++) {
        MechStep *st = &m->steps[s];
        int mol = 0;
        for (int i = 0; i < st->n_react; i++) {
            int id = find_or_add_species(m, st->react[i].formula);
            if (id < 0) continue;
            m->net_coeff[id] -= st->react[i].coeff;
            if (s < first_react_step[id]) first_react_step[id] = s;
            mol += st->react[i].coeff;
        }
        for (int i = 0; i < st->n_prod; i++) {
            int id = find_or_add_species(m, st->prod[i].formula);
            if (id < 0) continue;
            m->net_coeff[id] += st->prod[i].coeff;
            if (s < first_prod_step[id]) first_prod_step[id] = s;
        }
        if (mol == 1) strcpy(m->molecularity[s], "Unimolecular");
        else if (mol == 2) strcpy(m->molecularity[s], "Bimolecular");
        else if (mol == 3) strcpy(m->molecularity[s], "Termolecular");
        else snprintf(m->molecularity[s], 16, "%d-molecular", mol);

        if (st->speed == STEP_SLOW && m->slow_step_idx < 0) m->slow_step_idx = s;
    }
    if (m->slow_step_idx < 0) m->slow_step_idx = 0;

    /* Classify Intermediates (produced first, then consumed) vs Catalysts (consumed first, then regenerated) */
    for (int i = 0; i < m->n_species; i++) {
        if (first_prod_step[i] < first_react_step[i] && first_react_step[i] < 99) {
            m->is_intermediate[i] = true;
        } else if (m->net_coeff[i] == 0 && first_react_step[i] < first_prod_step[i]) {
            m->is_catalyst[i] = true;
        }
    }

    /* Build Overall Balanced Equation math string */
    char tmp[32];
    m->overall_eq[0] = '\0';
    bool first = true;
    for (int i = 0; i < m->n_species; i++) {
        if (m->net_coeff[i] < 0) {
            if (!first) strncat(m->overall_eq, " + ", sizeof(m->overall_eq) - strlen(m->overall_eq) - 1);
            first = false;
            format_formula_math(m->species_name[i], tmp, sizeof(tmp));
            int c = -m->net_coeff[i];
            char item[64];
            if (c > 1) snprintf(item, sizeof(item), "%d%s", c, tmp);
            else snprintf(item, sizeof(item), "%s", tmp);
            strncat(m->overall_eq, item, sizeof(m->overall_eq) - strlen(m->overall_eq) - 1);
        }
    }
    strncat(m->overall_eq, " " SYM_ARROW " ", sizeof(m->overall_eq) - strlen(m->overall_eq) - 1);
    first = true;
    for (int i = 0; i < m->n_species; i++) {
        if (m->net_coeff[i] > 0) {
            if (!first) strncat(m->overall_eq, " + ", sizeof(m->overall_eq) - strlen(m->overall_eq) - 1);
            first = false;
            format_formula_math(m->species_name[i], tmp, sizeof(tmp));
            int c = m->net_coeff[i];
            char item[64];
            if (c > 1) snprintf(item, sizeof(item), "%d%s", c, tmp);
            else snprintf(item, sizeof(item), "%s", tmp);
            strncat(m->overall_eq, item, sizeof(m->overall_eq) - strlen(m->overall_eq) - 1);
        }
    }

    /* Start rate law from Slow Step molecularity */
    MechStep *slow = &m->steps[m->slow_step_idx];
    for (int i = 0; i < slow->n_react; i++) {
        int id = find_or_add_species(m, slow->react[i].formula);
        if (id >= 0) m->rate_exp[id] += (double)slow->react[i].coeff;
    }

    /* Eliminate intermediates backward through preceding Fast Equilibrium steps */
    m->elim_step_math[0] = '\0';
    for (int s = m->slow_step_idx - 1; s >= 0; s--) {
        MechStep *eq = &m->steps[s];
        if (eq->speed != STEP_FAST_EQ) continue;
        /* Check if any intermediate with non-zero exponent in rate_exp is a product of step s */
        for (int p = 0; p < eq->n_prod; p++) {
            int id = find_or_add_species(m, eq->prod[p].formula);
            if (id < 0 || !m->is_intermediate[id] || fabs(m->rate_exp[id]) < 1e-6) continue;
            double e = m->rate_exp[id];
            double c_int = (double)eq->prod[p].coeff;
            double scale = e / c_int;
            m->rate_exp[id] = 0.0;
            /* Substitute: [Int]^c_int = (kf/kr) * prod([React]^c_r) / prod([OtherProd]^c_op) */
            for (int r = 0; r < eq->n_react; r++) {
                int rid = find_or_add_species(m, eq->react[r].formula);
                if (rid >= 0) m->rate_exp[rid] += scale * (double)eq->react[r].coeff;
            }
            for (int op = 0; op < eq->n_prod; op++) {
                if (op == p) continue;
                int oid = find_or_add_species(m, eq->prod[op].formula);
                if (oid >= 0) m->rate_exp[oid] -= scale * (double)eq->prod[op].coeff;
            }
            format_formula_math(m->species_name[id], tmp, sizeof(tmp));
            snprintf(m->elim_step_math, sizeof(m->elim_step_math),
                     "Eliminated [%s] via Step %d fast eq.", tmp, s + 1);
        }
    }

    /* Format final derived Rate Law in clean math notation */
    char num[64] = "~k~", den[64] = "";
    bool has_den = false;
    for (int i = 0; i < m->n_species; i++) {
        double e = m->rate_exp[i];
        if (fabs(e) < 1e-6) continue;
        format_formula_math(m->species_name[i], tmp, sizeof(tmp));
        char term[64];
        double ae = fabs(e);
        if (fabs(ae - 1.0) < 1e-6) snprintf(term, sizeof(term), "[%s]", tmp);
        else if (fabs(ae - floor(ae + 0.5)) < 1e-6) snprintf(term, sizeof(term), "[%s]^{%d}", tmp, (int)floor(ae + 0.5));
        else if (fabs(ae - 0.5) < 1e-6) snprintf(term, sizeof(term), "[%s]^{1/2}", tmp);
        else if (fabs(ae - 1.5) < 1e-6) snprintf(term, sizeof(term), "[%s]^{3/2}", tmp);
        else snprintf(term, sizeof(term), "[%s]^{%.1f}", tmp, ae);

        if (e > 0.0) strncat(num, term, sizeof(num) - strlen(num) - 1);
        else { has_den = true; strncat(den, term, sizeof(den) - strlen(den) - 1); }
    }
    if (has_den) {
        snprintf(m->rate_law_math, sizeof(m->rate_law_math), "~Rate~ = \\f{%s}{%s}", num, den);
    } else {
        snprintf(m->rate_law_math, sizeof(m->rate_law_math), "~Rate~ = %s", num);
    }
    m->valid = true;
    return true;
}

/* ============================================================================
 * Module 5: Arrhenius, Collision Model & Catalysis
 * ============================================================================ */
bool kinetics_arrhenius_2pt(ArrhTwoPtState *s) {
    s->valid = false;
    /* ln(k2/k1) = (Ea / R) * (1/T1 - 1/T2), Ea in J/mol = Ea_kj * 1000 */
    if (s->unknown == 0) {
        if (s->k1 <= 0 || s->k2 <= 0 || s->T1 <= 0 || s->T2 <= 0) return false;
        double d = (1.0 / s->T1) - (1.0 / s->T2);
        if (fabs(d) < 1e-12) return false;
        s->Ea_kj = (log(s->k2 / s->k1) * GAS_R / d) / 1000.0;
    } else if (s->unknown == 1) {
        if (s->k1 <= 0 || s->T1 <= 0 || s->T2 <= 0) return false;
        double rhs = (s->Ea_kj * 1000.0 / GAS_R) * (1.0 / s->T1 - 1.0 / s->T2);
        s->k2 = s->k1 * exp(rhs);
    } else if (s->unknown == 2) {
        if (s->k1 <= 0 || s->k2 <= 0 || s->T1 <= 0 || fabs(s->Ea_kj) < 1e-9) return false;
        double invT2 = (1.0 / s->T1) - (log(s->k2 / s->k1) * GAS_R) / (s->Ea_kj * 1000.0);
        if (invT2 <= 0.0) return false;
        s->T2 = 1.0 / invT2;
    } else if (s->unknown == 3) {
        if (s->k2 <= 0 || s->T1 <= 0 || s->T2 <= 0) return false;
        double rhs = (s->Ea_kj * 1000.0 / GAS_R) * (1.0 / s->T1 - 1.0 / s->T2);
        s->k1 = s->k2 / exp(rhs);
    } else if (s->unknown == 4) {
        if (s->k1 <= 0 || s->k2 <= 0 || s->T2 <= 0 || fabs(s->Ea_kj) < 1e-9) return false;
        double invT1 = (1.0 / s->T2) + (log(s->k2 / s->k1) * GAS_R) / (s->Ea_kj * 1000.0);
        if (invT1 <= 0.0) return false;
        s->T1 = 1.0 / invT1;
    }
    s->valid = true;
    return true;
}

bool kinetics_arrhenius_regress(ArrhTableState *s) {
    s->solved = false;
    if (s->num_rows < 2 || s->num_rows > MAX_ARRH_ROWS) return false;
    double x[MAX_ARRH_ROWS], y[MAX_ARRH_ROWS];
    for (int i = 0; i < s->num_rows; i++) {
        if (s->T[i] <= 0.0 || s->k[i] <= 0.0) return false;
        x[i] = 1.0 / s->T[i];
        y[i] = log(s->k[i]);
    }
    if (!linreg_fit(x, y, s->num_rows, &s->reg)) return false;
    s->Ea_kj = (-s->reg.slope * GAS_R) / 1000.0;
    s->A_factor = exp(s->reg.intercept);
    if (s->eval_T > 0.0) {
        s->eval_k = exp(s->reg.slope / s->eval_T + s->reg.intercept);
    }
    s->solved = true;
    return true;
}

void kinetics_catalyst_calc(CatalystState *s) {
    s->Ea_rev_kj = s->Ea_uncat_kj - s->delta_E_kj;
    if (s->T_kelvin > 0.0) {
        double dE_J = (s->Ea_uncat_kj - s->Ea_cat_kj) * 1000.0;
        s->speedup_ratio = exp(dE_J / (GAS_R * s->T_kelvin));
    } else {
        s->speedup_ratio = 1.0;
    }
}
