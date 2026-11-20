#ifndef PLEXCHEM_GFX_H
#define PLEXCHEM_GFX_H

#include <stdint.h>
#include <stdbool.h>
#include "fonts_baked.h"

/* Core PlexMath minimalist light palette */
enum {
    COL_WHITE       = 0,  /* #FFFFFF main canvas */
    COL_CHROME      = 1,  /* #FAF9F7 top/bottom bar */
    COL_BORDER      = 2,  /* #DFDDD8 crisp 1px rules */
    COL_BLUE_BG     = 3,  /* #EDF3FF active step card */
    COL_GRAY_BG     = 4,  /* #F5F5F3 subtle tile/input fill */
    COL_ACCENT      = 5,  /* #1565D8 PlexMath blue */
    COL_ACCENT_DK   = 6,  /* #0D47A1 */
    COL_INK         = 7,  /* #18181B primary text */
    COL_MUTED       = 8,  /* #6E7178 secondary caption */
    COL_GREEN       = 9,  /* #15803D catalyzed / winner */
    COL_GREEN_BG    = 10, /* #ECFDF3 */
    COL_RED         = 11, /* #C62828 uncatalyzed / slow */
    COL_RED_BG      = 12, /* #FEF2F2 */
    COL_AMBER       = 13, /* #B45309 intermediate */
    COL_AMBER_BG    = 14, /* #FFFBEB */
    COL_GRID        = 15  /* #ECEAE4 subtle plot grid */
};

/* 4-level anti-aliasing palette ramps (base index + alpha 0..3) */
enum {
    RAMP_INK_WHITE     = 16,
    RAMP_INK_BLUE      = 20,
    RAMP_INK_GRAY      = 24,
    RAMP_INK_CHROME    = 28,
    RAMP_MUTED_WHITE   = 32,
    RAMP_MUTED_BLUE    = 36,
    RAMP_MUTED_CHROME  = 40,
    RAMP_ACCENT_WHITE  = 44,
    RAMP_ACCENT_BLUE   = 48,
    RAMP_WHITE_ACCENT  = 52,
    RAMP_GREEN_WHITE   = 56,
    RAMP_GREEN_BG      = 60,
    RAMP_RED_WHITE     = 64,
    RAMP_RED_BG        = 68,
    RAMP_AMBER_BG      = 72,
    RAMP_INK_GREEN_BG  = 76
};

void gfx_init_palette(void);
void gfx_clear(uint8_t col);
void gfx_pixel(int x, int y, uint8_t col);
void gfx_hline(int x, int y, int w, uint8_t col);
void gfx_vline(int x, int y, int h, uint8_t col);
void gfx_fill_rect(int x, int y, int w, int h, uint8_t col);
void gfx_draw_rect(int x, int y, int w, int h, uint8_t col);
void gfx_fill_rrect(int x, int y, int w, int h, int r, uint8_t col);
void gfx_draw_rrect(int x, int y, int w, int h, int r, uint8_t col);
void gfx_line(int x0, int y0, int x1, int y1, uint8_t col);
void gfx_dashed_line(int x0, int y0, int x1, int y1, uint8_t col);
void gfx_fill_circle(int cx, int cy, int r, uint8_t col);

/* 2-bit Anti-Aliased Proportional Text & Icon Rendering */
int  gfx_text_width(const BakedFont *font, const char *s);
int  gfx_draw_text(int x, int y, const BakedFont *font, const char *s, uint8_t ramp);
void gfx_draw_text_right(int right_x, int y, const BakedFont *font, const char *s, uint8_t ramp);
void gfx_draw_text_center(int cx, int y, const BakedFont *font, const char *s, uint8_t ramp);
void gfx_draw_icon(int x, int y, uint8_t icon_id, uint8_t ramp);

/* Shared PlexMath Chrome Components */
void ui_draw_topbar(const char *left_title, const char *right_status);
void ui_draw_bottom_tabs(int active_tab);
void ui_draw_keycap(int x, int y, const char *key_label, const char *desc);
void ui_draw_pill(int x, int y, const char *label, uint8_t bg, uint8_t ramp);

#endif /* PLEXCHEM_GFX_H */
