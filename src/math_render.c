#include "math_render.h"
#include "gfx.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static void pick_fonts(int size_px, const BakedFont **reg, const BakedFont **ital,
                       const BakedFont **sreg, const BakedFont **sital) {
    if (size_px >= 18) {
        *reg = &FONT_MATH_20;
        *ital = &FONT_MATH_20I;
        *sreg = &FONT_MATH_14;
        *sital = &FONT_MATH_14I;
    } else if (size_px >= 13) {
        *reg = &FONT_MATH_14;
        *ital = &FONT_MATH_14I;
        *sreg = &FONT_MATH_10;
        *sital = &FONT_MATH_10I;
    } else {
        *reg = &FONT_MATH_10;
        *ital = &FONT_MATH_10I;
        *sreg = &FONT_MATH_10;
        *sital = &FONT_MATH_10I;
    }
}

/* Extract balanced {...} group starting at *p == '{'. Advances *p past '}'. */
static int extract_group(const char **p, char *out, int max_len) {
    if (**p != '{') return 0;
    (*p)++;
    int depth = 1, n = 0;
    while (**p && depth > 0) {
        if (**p == '{') depth++;
        else if (**p == '}') {
            depth--;
            if (depth == 0) { (*p)++; break; }
        }
        if (n < max_len - 1) out[n++] = **p;
        (*p)++;
    }
    out[n] = '\0';
    return n;
}

static int render_internal(int x, int y, const char *expr, int size_px,
                           uint8_t ramp, uint8_t bar_col, bool do_draw) {
    const BakedFont *f_reg, *f_ital, *f_sreg, *f_sital;
    pick_fonts(size_px, &f_reg, &f_ital, &f_sreg, &f_sital);

    int cx = x;
    bool italic = false;
    const char *p = expr;

    while (*p) {
        if (*p == '~') {
            italic = !italic;
            p++;
            continue;
        }
        /* Stacked fraction: \f{num}{den} */
        if (p[0] == '\\' && p[1] == 'f' && p[2] == '{') {
            char num[64], den[64];
            p += 2;
            extract_group(&p, num, sizeof(num));
            extract_group(&p, den, sizeof(den));
            int fsz = (size_px >= 18) ? 14 : 10;
            int nw = render_internal(0, 0, num, fsz, ramp, bar_col, false);
            int dw = render_internal(0, 0, den, fsz, ramp, bar_col, false);
            int fw = (nw > dw ? nw : dw) + 6;
            int mid_y = y + (f_reg->ascent * 6) / 10;
            if (do_draw) {
                int num_h = (fsz >= 14) ? FONT_MATH_14.ascent : FONT_MATH_10.ascent;
                render_internal(cx + (fw - nw) / 2, mid_y - num_h - 2, num, fsz, ramp, bar_col, true);
                gfx_hline(cx + 1, mid_y, fw - 2, bar_col);
                render_internal(cx + (fw - dw) / 2, mid_y + 2, den, fsz, ramp, bar_col, true);
            }
            cx += fw + 2;
            continue;
        }
        /* True raised superscript: ^{...} */
        if (p[0] == '^' && p[1] == '{') {
            char sub[48];
            p++;
            extract_group(&p, sub, sizeof(sub));
            int ssz = (size_px >= 18) ? 14 : 10;
            int sup_y = y - (size_px >= 18 ? 4 : 3);
            int w = render_internal(cx, sup_y, sub, ssz, ramp, bar_col, do_draw);
            cx += w;
            continue;
        }
        /* True lowered subscript: _{...} */
        if (p[0] == '_' && p[1] == '{') {
            char sub[48];
            p++;
            extract_group(&p, sub, sizeof(sub));
            int ssz = (size_px >= 18) ? 14 : 10;
            int sub_y = y + (size_px >= 18 ? 8 : 5);
            int w = render_internal(cx, sub_y, sub, ssz, ramp, bar_col, do_draw);
            cx += w;
            continue;
        }
        /* Single glyph */
        char ch_buf[2] = { *p, '\0' };
        const BakedFont *use_f = italic ? f_ital : f_reg;
        /* Keep digits, brackets, +, -, = upright even inside ~...~ */
        if ((*p >= '0' && *p <= '9') || *p == '[' || *p == ']' || *p == '(' || *p == ')' ||
            *p == '=' || *p == '+' || *p == '-' || (unsigned char)*p >= 0x80) {
            use_f = f_reg;
        }
        if (do_draw) {
            cx += gfx_draw_text(cx, y, use_f, ch_buf, ramp);
        } else {
            cx += gfx_text_width(use_f, ch_buf);
        }
        p++;
    }
    return cx - x;
}

int math_measure_w(const char *expr, int size_px) {
    return render_internal(0, 0, expr, size_px, RAMP_INK_WHITE, COL_INK, false);
}

int math_draw(int x, int y, const char *expr, int size_px, uint8_t ramp, uint8_t bar_col) {
    return render_internal(x, y, expr, size_px, ramp, bar_col, true);
}

void math_draw_center(int cx, int y, const char *expr, int size_px, uint8_t ramp, uint8_t bar_col) {
    int w = math_measure_w(expr, size_px);
    render_internal(cx - w / 2, y, expr, size_px, ramp, bar_col, true);
}

void math_draw_right(int rx, int y, const char *expr, int size_px, uint8_t ramp, uint8_t bar_col) {
    int w = math_measure_w(expr, size_px);
    render_internal(rx - w, y, expr, size_px, ramp, bar_col, true);
}

void fmt_sci_math(double val, char *buf, size_t sz, int sigfigs) {
    if (!isfinite(val)) { snprintf(buf, sz, "NaN"); return; }
    if (fabs(val) < 1e-30) { snprintf(buf, sz, "0"); return; }
    double av = fabs(val);
    const char *sgn = (val < 0.0) ? SYM_MINUS : "";
    if (av >= 0.01 && av < 1000.0) {
        if (av >= 100.0) snprintf(buf, sz, "%s%.1f", sgn, av);
        else if (av >= 10.0) snprintf(buf, sz, "%s%.2f", sgn, av);
        else if (av >= 1.0) snprintf(buf, sz, "%s%.3f", sgn, av);
        else snprintf(buf, sz, "%s%.4f", sgn, av);
        return;
    }
    int exp10 = (int)floor(log10(av));
    double mant = av / pow(10.0, (double)exp10);
    if (mant >= 9.995) { mant /= 10.0; exp10++; }
    int dp = (sigfigs > 1) ? (sigfigs - 1) : 2;
    if (dp > 3) dp = 3;
    if (exp10 < 0) {
        snprintf(buf, sz, "%s%.*f" SYM_TIMES "10^{" SYM_MINUS "%d}", sgn, dp, mant, -exp10);
    } else {
        snprintf(buf, sz, "%s%.*f" SYM_TIMES "10^{%d}", sgn, dp, mant, exp10);
    }
}

void fmt_compact(double val, char *buf, size_t sz) {
    if (!isfinite(val)) { snprintf(buf, sz, "--"); return; }
    double av = fabs(val);
    if (av == 0.0) { snprintf(buf, sz, "0"); return; }
    if (av >= 0.001 && av < 10000.0) {
        snprintf(buf, sz, "%.4g", val);
    } else {
        snprintf(buf, sz, "%.2e", val);
    }
}
