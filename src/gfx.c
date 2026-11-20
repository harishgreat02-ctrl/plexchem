#include "gfx.h"
#include "hal.h"
#include "icons_baked.h"
#include <string.h>
#include <stdlib.h>

static const uint8_t CORE_RGB[16][3] = {
    {255, 255, 255}, /* 0  COL_WHITE */
    {250, 249, 247}, /* 1  COL_CHROME */
    {223, 221, 216}, /* 2  COL_BORDER */
    {237, 243, 255}, /* 3  COL_BLUE_BG */
    {245, 245, 243}, /* 4  COL_GRAY_BG */
    { 21, 101, 216}, /* 5  COL_ACCENT */
    { 13,  71, 161}, /* 6  COL_ACCENT_DK */
    { 24,  24,  27}, /* 7  COL_INK */
    {110, 113, 120}, /* 8  COL_MUTED */
    { 21, 128,  61}, /* 9  COL_GREEN */
    {236, 253, 243}, /* 10 COL_GREEN_BG */
    {198,  40,  40}, /* 11 COL_RED */
    {254, 242, 242}, /* 12 COL_RED_BG */
    {180,  83,   9}, /* 13 COL_AMBER */
    {255, 251, 235}, /* 14 COL_AMBER_BG */
    {236, 234, 228}, /* 15 COL_GRID */
};

static void build_ramp(uint8_t base, uint8_t fg_idx, uint8_t bg_idx) {
    const uint8_t *fg = CORE_RGB[fg_idx];
    const uint8_t *bg = CORE_RGB[bg_idx];
    for (int a = 0; a < 4; a++) {
        uint8_t r = (uint8_t)((bg[0] * (3 - a) + fg[0] * a) / 3);
        uint8_t g = (uint8_t)((bg[1] * (3 - a) + fg[1] * a) / 3);
        uint8_t b = (uint8_t)((bg[2] * (3 - a) + fg[2] * a) / 3);
        hal_set_palette_entry((uint8_t)(base + a), r, g, b);
    }
}

void gfx_init_palette(void) {
    for (int i = 0; i < 16; i++) {
        hal_set_palette_entry((uint8_t)i, CORE_RGB[i][0], CORE_RGB[i][1], CORE_RGB[i][2]);
    }
    build_ramp(RAMP_INK_WHITE,    COL_INK,    COL_WHITE);
    build_ramp(RAMP_INK_BLUE,     COL_INK,    COL_BLUE_BG);
    build_ramp(RAMP_INK_GRAY,     COL_INK,    COL_GRAY_BG);
    build_ramp(RAMP_INK_CHROME,   COL_INK,    COL_CHROME);
    build_ramp(RAMP_MUTED_WHITE,  COL_MUTED,  COL_WHITE);
    build_ramp(RAMP_MUTED_BLUE,   COL_MUTED,  COL_BLUE_BG);
    build_ramp(RAMP_MUTED_CHROME, COL_MUTED,  COL_CHROME);
    build_ramp(RAMP_ACCENT_WHITE, COL_ACCENT, COL_WHITE);
    build_ramp(RAMP_ACCENT_BLUE,  COL_ACCENT, COL_BLUE_BG);
    build_ramp(RAMP_WHITE_ACCENT, COL_WHITE,  COL_ACCENT);
    build_ramp(RAMP_GREEN_WHITE,  COL_GREEN,  COL_WHITE);
    build_ramp(RAMP_GREEN_BG,     COL_GREEN,  COL_GREEN_BG);
    build_ramp(RAMP_RED_WHITE,    COL_RED,    COL_WHITE);
    build_ramp(RAMP_RED_BG,       COL_RED,    COL_RED_BG);
    build_ramp(RAMP_AMBER_BG,     COL_AMBER,  COL_AMBER_BG);
    build_ramp(RAMP_INK_GREEN_BG, COL_INK,    COL_GREEN_BG);
}

void gfx_clear(uint8_t col) {
    memset(hal_backbuffer(), col, LCD_W * LCD_H);
}

void gfx_pixel(int x, int y, uint8_t col) {
    if ((unsigned)x < LCD_W && (unsigned)y < LCD_H) {
        hal_backbuffer()[y * LCD_W + x] = col;
    }
}

void gfx_hline(int x, int y, int w, uint8_t col) {
    if (y < 0 || y >= LCD_H || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > LCD_W) w = LCD_W - x;
    if (w <= 0) return;
    memset(hal_backbuffer() + y * LCD_W + x, col, (size_t)w);
}

void gfx_vline(int x, int y, int h, uint8_t col) {
    if (x < 0 || x >= LCD_W || h <= 0) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > LCD_H) h = LCD_H - y;
    uint8_t *p = hal_backbuffer() + y * LCD_W + x;
    for (int i = 0; i < h; i++) {
        *p = col;
        p += LCD_W;
    }
}

void gfx_fill_rect(int x, int y, int w, int h, uint8_t col) {
    for (int j = 0; j < h; j++) gfx_hline(x, y + j, w, col);
}

void gfx_draw_rect(int x, int y, int w, int h, uint8_t col) {
    gfx_hline(x, y, w, col);
    gfx_hline(x, y + h - 1, w, col);
    gfx_vline(x, y, h, col);
    gfx_vline(x + w - 1, y, h, col);
}

void gfx_fill_rrect(int x, int y, int w, int h, int r, uint8_t col) {
    if (r <= 1) { gfx_fill_rect(x, y, w, h, col); return; }
    /* Clean 3px rounded corner approximation */
    gfx_hline(x + 2, y, w - 4, col);
    gfx_hline(x + 1, y + 1, w - 2, col);
    for (int j = 2; j < h - 2; j++) gfx_hline(x, y + j, w, col);
    gfx_hline(x + 1, y + h - 2, w - 2, col);
    gfx_hline(x + 2, y + h - 1, w - 4, col);
}

void gfx_draw_rrect(int x, int y, int w, int h, int r, uint8_t col) {
    if (r <= 1) { gfx_draw_rect(x, y, w, h, col); return; }
    gfx_hline(x + 2, y, w - 4, col);
    gfx_hline(x + 2, y + h - 1, w - 4, col);
    gfx_vline(x, y + 2, h - 4, col);
    gfx_vline(x + w - 1, y + 2, h - 4, col);
    gfx_pixel(x + 1, y + 1, col);
    gfx_pixel(x + w - 2, y + 1, col);
    gfx_pixel(x + 1, y + h - 2, col);
    gfx_pixel(x + w - 2, y + h - 2, col);
}

void gfx_line(int x0, int y0, int x1, int y1, uint8_t col) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        gfx_pixel(x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_dashed_line(int x0, int y0, int x1, int y1, uint8_t col) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, step = 0;
    for (;;) {
        if ((step % 6) < 3) gfx_pixel(x0, y0, col);
        step++;
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_fill_circle(int cx, int cy, int r, uint8_t col) {
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) gfx_pixel(cx + dx, cy + dy, col);
        }
    }
}

static void blit_2bit(int dx, int dy, int w, int h, const uint8_t *src, uint8_t ramp) {
    uint8_t *fb = hal_backbuffer();
    int stride = (w + 3) >> 2;
    for (int y = 0; y < h; y++) {
        int py = dy + y;
        if ((unsigned)py >= LCD_H) { src += stride; continue; }
        uint8_t *row = fb + py * LCD_W;
        for (int x = 0; x < w; x++) {
            uint8_t b = src[x >> 2];
            uint8_t a = (uint8_t)((b >> (6 - ((x & 3) << 1))) & 3);
            if (a) {
                int px = dx + x;
                if ((unsigned)px < LCD_W) row[px] = (uint8_t)(ramp + a);
            }
        }
        src += stride;
    }
}

int gfx_text_width(const BakedFont *font, const char *s) {
    int w = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        uint8_t c = *p;
        if (c < 0x20 || c > 0x90) continue;
        w += font->metrics[c - 0x20].adv;
    }
    return w;
}

int gfx_draw_text(int x, int y, const BakedFont *font, const char *s, uint8_t ramp) {
    int cx = x;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        uint8_t c = *p;
        if (c < 0x20 || c > 0x90) continue;
        const GlyphMetric *m = &font->metrics[c - 0x20];
        if (m->w > 0 && m->h > 0) {
            blit_2bit(cx + m->xoff, y + m->yoff, m->w, m->h, font->bitmap + m->offset, ramp);
        }
        cx += m->adv;
    }
    return cx - x;
}

void gfx_draw_text_right(int right_x, int y, const BakedFont *font, const char *s, uint8_t ramp) {
    int w = gfx_text_width(font, s);
    gfx_draw_text(right_x - w, y, font, s, ramp);
}

void gfx_draw_text_center(int cx, int y, const BakedFont *font, const char *s, uint8_t ramp) {
    int w = gfx_text_width(font, s);
    gfx_draw_text(cx - w / 2, y, font, s, ramp);
}

void gfx_draw_icon(int x, int y, uint8_t icon_id, uint8_t ramp) {
    if (icon_id >= 6) return;
    blit_2bit(x, y, ICON_SIZE, ICON_SIZE, ALL_ICONS[icon_id], ramp);
}

/* ============================================================================
 * Shared PlexMath Chrome (Top Header + 5-Tab Bottom Softkey Bar + Keycaps)
 * ============================================================================ */
void ui_draw_topbar(const char *left_title, const char *right_status) {
    gfx_fill_rect(0, 0, LCD_W, 20, COL_CHROME);
    gfx_hline(0, 20, LCD_W, COL_BORDER);
    gfx_draw_text(8, 3, &FONT_UI_11B, left_title, RAMP_INK_CHROME);
    if (right_status && right_status[0]) {
        gfx_draw_text_right(LCD_W - 8, 3, &FONT_UI_11, right_status, RAMP_MUTED_CHROME);
    }
}

void ui_draw_bottom_tabs(int active_tab) {
    static const char * const LABELS[5] = {
        "Rates", "Init.R", "Integ", "Mech", "Arrh"
    };
    const int y0 = LCD_H - 21;
    gfx_fill_rect(0, y0, LCD_W, 21, COL_CHROME);
    gfx_hline(0, y0, LCD_W, COL_BORDER);
    for (int i = 0; i < 5; i++) {
        int x0 = i * 64;
        if (i == active_tab) {
            gfx_hline(x0 + 6, y0 + 1, 52, COL_ACCENT);
            gfx_hline(x0 + 6, y0 + 2, 52, COL_ACCENT);
            gfx_draw_text_center(x0 + 32, y0 + 5, &FONT_UI_11B, LABELS[i], RAMP_ACCENT_WHITE);
        } else {
            gfx_draw_text_center(x0 + 32, y0 + 5, &FONT_UI_11, LABELS[i], RAMP_INK_CHROME);
        }
        if (i > 0) {
            gfx_vline(x0, y0 + 5, 11, COL_BORDER);
        }
    }
}

void ui_draw_keycap(int x, int y, const char *key_label, const char *desc) {
    int kw = gfx_text_width(&FONT_UI_11B, key_label) + 10;
    gfx_fill_rrect(x, y, kw, 15, 2, COL_WHITE);
    gfx_draw_rrect(x, y, kw, 15, 2, COL_BORDER);
    gfx_draw_text(x + 5, y + 1, &FONT_UI_11B, key_label, RAMP_INK_WHITE);
    if (desc && desc[0]) {
        gfx_draw_text(x + kw + 5, y + 1, &FONT_UI_11, desc, RAMP_MUTED_WHITE);
    }
}

void ui_draw_pill(int x, int y, const char *label, uint8_t bg, uint8_t ramp) {
    int w = gfx_text_width(&FONT_UI_11B, label) + 10;
    gfx_fill_rrect(x, y, w, 15, 3, bg);
    gfx_draw_text(x + 5, y + 1, &FONT_UI_11B, label, ramp);
}
