#ifndef PLEXCHEM_HAL_H
#define PLEXCHEM_HAL_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_W 320
#define LCD_H 240

/* Unified key codes (TI-84 Plus CE direct keypad + host simulator) */
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_ENTER,
    KEY_CLEAR,
    KEY_DEL,
    KEY_2ND,
    KEY_ALPHA,
    KEY_MODE,
    KEY_APPS,
    KEY_STAT,
    KEY_MATH,
    /* Bottom 5 softkeys directly below the 320x240 screen */
    KEY_F1, /* Y=     -> Rates */
    KEY_F2, /* WINDOW -> Init.R */
    KEY_F3, /* ZOOM   -> Integ */
    KEY_F4, /* TRACE  -> Mech */
    KEY_F5, /* GRAPH  -> Arrh */
    /* Numeric & scientific input */
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4,
    KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
    KEY_DOT,
    KEY_NEG,  /* (-) key */
    KEY_EE,   /* 2nd , (EE) or x^-1 for scientific notation 'e' */
    KEY_PLUS,
    KEY_MINUS,
    KEY_MUL,
    KEY_DIV,
    KEY_QUIT  /* ON or 2nd+MODE */
} KeyCode;

void hal_init(void);
void hal_shutdown(void);
uint8_t *hal_backbuffer(void);
void hal_swap_buffers(void);
void hal_set_palette_entry(uint8_t idx, uint8_t r, uint8_t g, uint8_t b);
KeyCode hal_wait_key(void);

#endif /* PLEXCHEM_HAL_H */
