#include "hal.h"
#include <string.h>

#ifndef HOST_SIM
/* ============================================================================
 * Bare-Metal TI-84 Plus CE Hardware Driver (ZERO external .8xv libraries!)
 * Targets OS 5.3 - 5.8.4.0058 (Flash App via app_tools or .8xp via arTIfiCE)
 * Uses dedicated PL111 LCD VRAM (0xD40000 & 0xD52C00) -> 0 bytes user RAM!
 * ============================================================================ */
#include <ti/screen.h>

#define LCD_UPBASE  (*(volatile uint32_t *)0xE30010)
#define LCD_CTRL    (*(volatile uint32_t *)0xE30018)
#define LCD_PALETTE ((volatile uint16_t *)0xE30200)
#define LCD_RIS     (*(volatile uint32_t *)0xE30020)
#define LCD_ICR     (*(volatile uint32_t *)0xE30028)

#define VRAM_PAGE0  ((uint8_t *)0xD40000)
#define VRAM_PAGE1  ((uint8_t *)0xD52C00) /* + 320*240 = 0x12C00 */

/* Direct memory-mapped keypad controller (0xF50000) */
#define KB_CTRL     (*(volatile uint16_t *)0xF50000)
#define KB_DATA     ((volatile uint16_t *)0xF50010)

static uint8_t s_cur_page = 1;
static uint32_t s_saved_ctrl = 0;

static uint16_t rgb_to_1555(uint8_t r, uint8_t g, uint8_t b) {
    /* TI-84 Plus CE PL111 BGR/RGB1555: R5 G5 B5 (LSB=G0) */
    uint16_t r5 = (uint16_t)(r >> 3) & 0x1F;
    uint16_t g5 = (uint16_t)(g >> 3) & 0x1F;
    uint16_t b5 = (uint16_t)(b >> 3) & 0x1F;
    return (uint16_t)((r5 << 10) | (g5 << 5) | b5);
}

void hal_init(void) {
    s_saved_ctrl = LCD_CTRL;
    /* Clear both 8bpp VRAM pages before switching mode */
    memset(VRAM_PAGE0, 0, LCD_W * LCD_H);
    memset(VRAM_PAGE1, 0, LCD_W * LCD_H);
    /* Switch LCD controller to 8bpp paletted, BGR=0, LCDEN=1, LCDPWR=1 */
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = (s_saved_ctrl & ~0x000E) | 0x0006; /* 8bpp (bits 3:1 = 011) */
    s_cur_page = 1;
    /* Set keypad to continuous scan mode */
    KB_CTRL = 0x0003;
}

void hal_shutdown(void) {
    /* Restore TI-OS 16bpp mode cleanly so OS 5.8.4 never glitches or crashes */
    memset(VRAM_PAGE0, 0xFF, LCD_W * LCD_H * 2);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = s_saved_ctrl;
    KB_CTRL = 0x0000;
    os_ClrHome();
}

uint8_t *hal_backbuffer(void) {
    return (s_cur_page == 0) ? VRAM_PAGE0 : VRAM_PAGE1;
}

void hal_swap_buffers(void) {
    /* Wait for VSYNC (LNBU / VComp) to prevent tearing */
    LCD_ICR = 0x04;
    while (!(LCD_RIS & 0x04)) { }
    LCD_UPBASE = (s_cur_page == 0) ? (uint32_t)VRAM_PAGE0 : (uint32_t)VRAM_PAGE1;
    s_cur_page ^= 1;
}

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    LCD_PALETTE[idx] = rgb_to_1555(r, g, b);
}

static KeyCode scan_raw_key(void) {
    uint16_t g1 = KB_DATA[1];
    uint16_t g2 = KB_DATA[2];
    uint16_t g3 = KB_DATA[3];
    uint16_t g4 = KB_DATA[4];
    uint16_t g5 = KB_DATA[5];
    uint16_t g6 = KB_DATA[6];
    uint16_t g7 = KB_DATA[7];

    /* Top row softkeys (Group 1) */
    if (g1 & (1 << 4)) return KEY_F1;   /* Y= */
    if (g1 & (1 << 3)) return KEY_F2;   /* WINDOW */
    if (g1 & (1 << 2)) return KEY_F3;   /* ZOOM */
    if (g1 & (1 << 1)) return KEY_F4;   /* TRACE */
    if (g1 & (1 << 0)) return KEY_F5;   /* GRAPH */
    if (g1 & (1 << 5)) return KEY_2ND;
    if (g1 & (1 << 6)) return KEY_MODE;
    if (g1 & (1 << 7)) return KEY_DEL;

    /* Arrow keys (Group 7) */
    if (g7 & (1 << 3)) return KEY_UP;
    if (g7 & (1 << 0)) return KEY_DOWN;
    if (g7 & (1 << 1)) return KEY_LEFT;
    if (g7 & (1 << 2)) return KEY_RIGHT;

    /* Control & Mode keys */
    if (g6 & (1 << 0)) return KEY_ENTER;
    if (g6 & (1 << 6)) return KEY_CLEAR;
    if (g2 & (1 << 7)) return KEY_ALPHA;
    if (g2 & (1 << 6)) return KEY_MATH;
    if (g3 & (1 << 6)) return KEY_APPS;
    if (g3 & (1 << 7)) return KEY_STAT;

    /* Operators & EE */
    if (g6 & (1 << 1)) return KEY_PLUS;
    if (g6 & (1 << 2)) return KEY_MINUS;
    if (g6 & (1 << 3)) return KEY_MUL;
    if (g6 & (1 << 4)) return KEY_DIV;
    if (g4 & (1 << 5)) return KEY_EE;   /* ',' / EE key */

    /* Digits & punctuation */
    if (g3 & (1 << 0)) return KEY_0;
    if (g3 & (1 << 1)) return KEY_1;
    if (g4 & (1 << 1)) return KEY_2;
    if (g5 & (1 << 1)) return KEY_3;
    if (g3 & (1 << 2)) return KEY_4;
    if (g4 & (1 << 2)) return KEY_5;
    if (g5 & (1 << 2)) return KEY_6;
    if (g3 & (1 << 3)) return KEY_7;
    if (g4 & (1 << 3)) return KEY_8;
    if (g5 & (1 << 3)) return KEY_9;
    if (g4 & (1 << 0)) return KEY_DOT;
    if (g5 & (1 << 0)) return KEY_NEG;

    return KEY_NONE;
}

KeyCode hal_wait_key(void) {
    /* Wait until previous key is released (clean debounce) */
    while (scan_raw_key() != KEY_NONE) { }
    /* Wait for a new key press */
    KeyCode k = KEY_NONE;
    while ((k = scan_raw_key()) == KEY_NONE) { }
    return k;
}

#else
/* ============================================================================
 * Host Simulator Backend (compiles natively with gcc to render 320x240 PNGs!)
 * ============================================================================ */
#include <stdio.h>

static uint8_t s_fb[LCD_W * LCD_H];
static uint8_t s_pal[256][3];

void hal_init(void) {
    memset(s_fb, 0, sizeof(s_fb));
    memset(s_pal, 0, sizeof(s_pal));
}

void hal_shutdown(void) { }

uint8_t *hal_backbuffer(void) {
    return s_fb;
}

void hal_swap_buffers(void) { }

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    s_pal[idx][0] = r;
    s_pal[idx][1] = g;
    s_pal[idx][2] = b;
}

KeyCode hal_wait_key(void) {
    return KEY_QUIT;
}

void host_save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", LCD_W, LCD_H);
    for (int i = 0; i < LCD_W * LCD_H; i++) {
        uint8_t idx = s_fb[i];
        fputc(s_pal[idx][0], f);
        fputc(s_pal[idx][1], f);
        fputc(s_pal[idx][2], f);
    }
    fclose(f);
}
#endif
