#include "hal.h"
#include <string.h>

#ifndef HOST_SIM
#include <ti/screen.h>
#include <ti/getcsc.h>

#define LCD_UPBASE  (*(volatile uint32_t *)0xE30010)
#define LCD_CTRL    (*(volatile uint32_t *)0xE30018)
#define LCD_PALETTE ((volatile uint16_t *)0xE30200)

#define VRAM_PAGE0  ((uint8_t *)0xD40000)
#define VRAM_PAGE1  ((uint8_t *)0xD52C00)

static uint8_t s_cur_page = 1;
static uint32_t s_saved_ctrl = 0;

static uint16_t rgb_to_1555(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (uint16_t)(r >> 3) & 0x1F;
    uint16_t g5 = (uint16_t)(g >> 3) & 0x1F;
    uint16_t b5 = (uint16_t)(b >> 3) & 0x1F;
    return (uint16_t)((r5 << 10) | (g5 << 5) | b5);
}

void hal_init(void) {
    s_saved_ctrl = LCD_CTRL;
    memset(VRAM_PAGE0, 0, LCD_W * LCD_H);
    memset(VRAM_PAGE1, 0, LCD_W * LCD_H);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = (s_saved_ctrl & ~0x000E) | 0x0006;
    s_cur_page = 1;
    while (os_GetCSC() != 0) { }
}

void hal_shutdown(void) {
    memset(VRAM_PAGE0, 0xFF, LCD_W * LCD_H * 2);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = s_saved_ctrl;
    os_ClrHome();
}

uint8_t *hal_backbuffer(void) {
    return (s_cur_page == 0) ? VRAM_PAGE0 : VRAM_PAGE1;
}

void hal_swap_buffers(void) {
    LCD_UPBASE = (s_cur_page == 0) ? (uint32_t)VRAM_PAGE0 : (uint32_t)VRAM_PAGE1;
    s_cur_page ^= 1;
}

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    LCD_PALETTE[idx] = rgb_to_1555(r, g, b);
}

KeyCode hal_wait_key(void) {
    for (;;) {
        uint8_t sc = os_GetCSC();
        if (sc == 0) continue;
        switch (sc) {
            case sk_Up:     return KEY_UP;
            case sk_Down:   return KEY_DOWN;
            case sk_Left:   return KEY_LEFT;
            case sk_Right:  return KEY_RIGHT;
            case sk_Enter:  return KEY_ENTER;
            case sk_Clear:  return KEY_CLEAR;
            case sk_Del:    return KEY_DEL;
            case sk_2nd:    return KEY_2ND;
            case sk_Alpha:  return KEY_ALPHA;
            case sk_Mode:   return KEY_MODE;
            case sk_Apps:   return KEY_APPS;
            case sk_Stat:   return KEY_STAT;
            case sk_Math:   return KEY_MATH;
            case sk_Yequ:   return KEY_F1;
            case sk_Window: return KEY_F2;
            case sk_Zoom:   return KEY_F3;
            case sk_Trace:  return KEY_F4;
            case sk_Graph:  return KEY_F5;
            case sk_0:      return KEY_0;
            case sk_1:      return KEY_1;
            case sk_2:      return KEY_2;
            case sk_3:      return KEY_3;
            case sk_4:      return KEY_4;
            case sk_5:      return KEY_5;
            case sk_6:      return KEY_6;
            case sk_7:      return KEY_7;
            case sk_8:      return KEY_8;
            case sk_9:      return KEY_9;
            case sk_DecPnt: return KEY_DOT;
            case sk_Chs:    return KEY_NEG;
            case sk_Comma:  return KEY_EE;
            case sk_Add:    return KEY_PLUS;
            case sk_Sub:    return KEY_MINUS;
            case sk_Mul:    return KEY_MUL;
            case sk_Div:    return KEY_DIV;
            default:        break;
        }
    }
}

#else
#include <stdio.h>

static uint8_t s_fb[LCD_W * LCD_H];
static uint8_t s_pal[256][3];

void hal_init(void) {
    memset(s_fb, 0, sizeof(s_fb));
    memset(s_pal, 0, sizeof(s_pal));
}

void hal_shutdown(void) { }
uint8_t *hal_backbuffer(void) { return s_fb; }
void hal_swap_buffers(void) { }

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    s_pal[idx][0] = r; s_pal[idx][1] = g; s_pal[idx][2] = b;
}

KeyCode hal_wait_key(void) { return KEY_QUIT; }

void host_save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", LCD_W, LCD_H);
    for (int i = 0; i < LCD_W * LCD_H; i++) {
        uint8_t idx = s_fb[i];
        fputc(s_pal[idx][0], f); fputc(s_pal[idx][1], f); fputc(s_pal[idx][2], f);
    }
    fclose(f);
}
#endif
#include <ti/getcsc.h>

#define LCD_UPBASE  (*(volatile uint32_t *)0xE30010)
#define LCD_CTRL    (*(volatile uint32_t *)0xE30018)
#define LCD_PALETTE ((volatile uint16_t *)0xE30200)

#define VRAM_PAGE0  ((uint8_t *)0xD40000)
#define VRAM_PAGE1  ((uint8_t *)0xD52C00)

static uint8_t s_cur_page = 1;
static uint32_t s_saved_ctrl = 0;

static uint16_t rgb_to_1555(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (uint16_t)(r >> 3) & 0x1F;
    uint16_t g5 = (uint16_t)(g >> 3) & 0x1F;
    uint16_t b5 = (uint16_t)(b >> 3) & 0x1F;
    return (uint16_t)((r5 << 10) | (g5 << 5) | b5);
}

void hal_init(void) {
    s_saved_ctrl = LCD_CTRL;
    memset(VRAM_PAGE0, 0, LCD_W * LCD_H);
    memset(VRAM_PAGE1, 0, LCD_W * LCD_H);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = (s_saved_ctrl & ~0x000E) | 0x0006;
    s_cur_page = 1;
    while (os_GetCSC() != 0) { }
}

void hal_shutdown(void) {
    memset(VRAM_PAGE0, 0xFF, LCD_W * LCD_H * 2);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = s_saved_ctrl;
    os_ClrHome();
}

uint8_t *hal_backbuffer(void) {
    return (s_cur_page == 0) ? VRAM_PAGE0 : VRAM_PAGE1;
}

void hal_swap_buffers(void) {
    LCD_UPBASE = (s_cur_page == 0) ? (uint32_t)VRAM_PAGE0 : (uint32_t)VRAM_PAGE1;
    s_cur_page ^= 1;
}

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    LCD_PALETTE[idx] = rgb_to_1555(r, g, b);
}

KeyCode hal_wait_key(void) {
    for (;;) {
        uint8_t sc = os_GetCSC();
        if (sc == 0) continue;
        switch (sc) {
            case sk_Up:     return KEY_UP;
            case sk_Down:   return KEY_DOWN;
            case sk_Left:   return KEY_LEFT;
            case sk_Right:  return KEY_RIGHT;
            case sk_Enter:  return KEY_ENTER;
            case sk_Clear:  return KEY_CLEAR;
            case sk_Del:    return KEY_DEL;
            case sk_2nd:    return KEY_2ND;
            case sk_Alpha:  return KEY_ALPHA;
            case sk_Mode:   return KEY_MODE;
            case sk_Apps:   return KEY_APPS;
            case sk_Stat:   return KEY_STAT;
            case sk_Math:   return KEY_MATH;
            case sk_Yequ:   return KEY_F1;
            case sk_Window: return KEY_F2;
            case sk_Zoom:   return KEY_F3;
            case sk_Trace:  return KEY_F4;
            case sk_Graph:  return KEY_F5;
            case sk_0:      return KEY_0;
            case sk_1:      return KEY_1;
            case sk_2:      return KEY_2;
            case sk_3:      return KEY_3;
            case sk_4:      return KEY_4;
            case sk_5:      return KEY_5;
            case sk_6:      return KEY_6;
            case sk_7:      return KEY_7;
            case sk_8:      return KEY_8;
            case sk_9:      return KEY_9;
            case sk_DecPnt: return KEY_DOT;
            case sk_Chs:    return KEY_NEG;
            case sk_Comma:  return KEY_EE;
            case sk_Add:    return KEY_PLUS;
            case sk_Sub:    return KEY_MINUS;
            case sk_Mul:    return KEY_MUL;
            case sk_Div:    return KEY_DIV;
            default:        break;
        }
    }
}

#else
#include <stdio.h>

static uint8_t s_fb[LCD_W * LCD_H];
static uint8_t s_pal[256][3];

void hal_init(void) {
    memset(s_fb, 0, sizeof(s_fb));
    memset(s_pal, 0, sizeof(s_pal));
}

void hal_shutdown(void) { }
uint8_t *hal_backbuffer(void) { return s_fb; }
void hal_swap_buffers(void) { }

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    s_pal[idx][0] = r; s_pal[idx][1] = g; s_pal[idx][2] = b;
}

KeyCode hal_wait_key(void) { return KEY_QUIT; }

void host_save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", LCD_W, LCD_H);
    for (int i = 0; i < LCD_W * LCD_H; i++) {
        uint8_t idx = s_fb[i];
        fputc(s_pal[idx][0], f); fputc(s_pal[idx][1], f); fputc(s_pal[idx][2], f);
    }
    fclose(f);
}
#endif
#include <ti/getcsc.h>

#define LCD_UPBASE  (*(volatile uint32_t *)0xE30010)
#define LCD_CTRL    (*(volatile uint32_t *)0xE30018)
#define LCD_PALETTE ((volatile uint16_t *)0xE30200)
#define LCD_RIS     (*(volatile uint32_t *)0xE30020)
#define LCD_ICR     (*(volatile uint32_t *)0xE30028)

#define VRAM_PAGE0  ((uint8_t *)0xD40000)
#define VRAM_PAGE1  ((uint8_t *)0xD52C00)

static uint8_t s_cur_page = 1;
static uint32_t s_saved_ctrl = 0;

static uint16_t rgb_to_1555(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t r5 = (uint16_t)(r >> 3) & 0x1F;
    uint16_t g5 = (uint16_t)(g >> 3) & 0x1F;
    uint16_t b5 = (uint16_t)(b >> 3) & 0x1F;
    return (uint16_t)((r5 << 10) | (g5 << 5) | b5);
}

void hal_init(void) {
    s_saved_ctrl = LCD_CTRL;
    memset(VRAM_PAGE0, 0, LCD_W * LCD_H);
    memset(VRAM_PAGE1, 0, LCD_W * LCD_H);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = (s_saved_ctrl & ~0x000E) | 0x0006;
    s_cur_page = 1;
    while (os_GetCSC() != 0) { }
}

void hal_shutdown(void) {
    memset(VRAM_PAGE0, 0xFF, LCD_W * LCD_H * 2);
    LCD_UPBASE = (uint32_t)VRAM_PAGE0;
    LCD_CTRL = s_saved_ctrl;
    os_ClrHome();
}

uint8_t *hal_backbuffer(void) {
    return (s_cur_page == 0) ? VRAM_PAGE0 : VRAM_PAGE1;
}

void hal_swap_buffers(void) {
    LCD_ICR = 0x04;
    while (!(LCD_RIS & 0x04)) { }
    LCD_UPBASE = (s_cur_page == 0) ? (uint32_t)VRAM_PAGE0 : (uint32_t)VRAM_PAGE1;
    s_cur_page ^= 1;
}

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    LCD_PALETTE[idx] = rgb_to_1555(r, g, b);
}

KeyCode hal_wait_key(void) {
    for (;;) {
        uint8_t sc = os_GetCSC();
        if (sc == 0) continue;
        switch (sc) {
            case sk_Up:     return KEY_UP;
            case sk_Down:   return KEY_DOWN;
            case sk_Left:   return KEY_LEFT;
            case sk_Right:  return KEY_RIGHT;
            case sk_Enter:  return KEY_ENTER;
            case sk_Clear:  return KEY_CLEAR;
            case sk_Del:    return KEY_DEL;
            case sk_2nd:    return KEY_2ND;
            case sk_Alpha:  return KEY_ALPHA;
            case sk_Mode:   return KEY_MODE;
            case sk_Apps:   return KEY_APPS;
            case sk_Stat:   return KEY_STAT;
            case sk_Math:   return KEY_MATH;
            case sk_Yequ:   return KEY_F1;
            case sk_Window: return KEY_F2;
            case sk_Zoom:   return KEY_F3;
            case sk_Trace:  return KEY_F4;
            case sk_Graph:  return KEY_F5;
            case sk_0:      return KEY_0;
            case sk_1:      return KEY_1;
            case sk_2:      return KEY_2;
            case sk_3:      return KEY_3;
            case sk_4:      return KEY_4;
            case sk_5:      return KEY_5;
            case sk_6:      return KEY_6;
            case sk_7:      return KEY_7;
            case sk_8:      return KEY_8;
            case sk_9:      return KEY_9;
            case sk_DecPnt: return KEY_DOT;
            case sk_Chs:    return KEY_NEG;
            case sk_Comma:  return KEY_EE;
            case sk_Add:    return KEY_PLUS;
            case sk_Sub:    return KEY_MINUS;
            case sk_Mul:    return KEY_MUL;
            case sk_Div:    return KEY_DIV;
            default:        break;
        }
    }
}

#else
#include <stdio.h>

static uint8_t s_fb[LCD_W * LCD_H];
static uint8_t s_pal[256][3];

void hal_init(void) {
    memset(s_fb, 0, sizeof(s_fb));
    memset(s_pal, 0, sizeof(s_pal));
}

void hal_shutdown(void) { }
uint8_t *hal_backbuffer(void) { return s_fb; }
void hal_swap_buffers(void) { }

void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b) {
    s_pal[idx][0] = r; s_pal[idx][1] = g; s_pal[idx][2] = b;
}

KeyCode hal_wait_key(void) { return KEY_QUIT; }

void host_save_ppm(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", LCD_W, LCD_H);
    for (int i = 0; i < LCD_W * LCD_H; i++) {
        uint8_t idx = s_fb[i];
        fputc(s_pal[idx][0], f); fputc(s_pal[idx][1], f); fputc(s_pal[idx][2], f);
    }
    fclose(f);
}
#endif
