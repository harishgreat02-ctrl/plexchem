#ifndef PLEXCHEM_MATH_RENDER_H
#define PLEXCHEM_MATH_RENDER_H

#include <stdint.h>
#include <stddef.h>

/*
 * True Math Typesetter (ZERO '^' carets!)
 * Supports inline markup:
 *   ~var~        -> Italic math serif variable (e.g. ~k~, ~t~, ~E~)
 *   ^{sup}       -> Raised superscript (true smaller font)
 *   _{sub}       -> Lowered subscript (true smaller font)
 *   \f{num}{den} -> Stacked horizontal fraction with 1px bar
 *   SYM_*        -> Single-byte glyphs (Δ, →, ⇌, ×, ·, −, ≈, °, ′)
 */
int  math_measure_w(const char *expr, int size_px);
int  math_draw(int x, int y, const char *expr, int size_px, uint8_t ramp, uint8_t bar_col);
void math_draw_center(int cx, int y, const char *expr, int size_px, uint8_t ramp, uint8_t bar_col);
void math_draw_right(int rx, int y, const char *expr, int size_px, uint8_t ramp, uint8_t bar_col);

/* Formats a float into clean textbook scientific notation (e.g. "4.20 × 10^{-5}") */
void fmt_sci_math(double val, char *buf, size_t sz, int sigfigs);
/* Formats a compact float for table cells */
void fmt_compact(double val, char *buf, size_t sz);

#endif /* PLEXCHEM_MATH_RENDER_H */
