/**
 * @file sasppu.h
 * @author John Hunter <moliveofscratch@gmail.com>
 * @brief The main include file for SASPPU.
 * @version 0.1
 * @date 2025-04-05
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef SASPPU_SASPPU_H_
#define SASPPU_SASPPU_H_

#include "stdint.h"
#include "stdbool.h"

#define SASPPU_VERSION_MAJOR 1
#define SASPPU_VERSION_MINOR 1
#define SASPPU_VERSION_PATCH 1

// #define SASPPU_VERSION "SASPPU_VERSION_MAJOR.SASPPU_VERSION_MINOR.SASPPU_VERSION_PATCH"

#define BG_WIDTH_POWER (8)
#define BG_HEIGHT_POWER (9)
#define BG_WIDTH (1 << BG_WIDTH_POWER)
#define BG_HEIGHT (1 << BG_HEIGHT_POWER)

#define SPRITE_COUNT (256)
#define SPRITE_CACHE (16)

#define SPR_WIDTH_POWER (8)
#define SPR_HEIGHT_POWER (8)
#define SPR_WIDTH (1 << SPR_WIDTH_POWER)
#define SPR_HEIGHT (1 << SPR_HEIGHT_POWER)

#define MAP_WIDTH_POWER (6)
#define MAP_HEIGHT_POWER (6)
#define MAP_WIDTH (1 << MAP_WIDTH_POWER)
#define MAP_HEIGHT (1 << MAP_HEIGHT_POWER)

typedef struct
{
    int16_t x;
    int16_t y;
    uint8_t width;
    uint8_t height;
    uint8_t graphics_x;
    uint8_t graphics_y;
    uint8_t windows;
    uint8_t flags;
} Sprite;

#define SPR_ENABLED (1 << 0)
#define SPR_PRIORITY (1 << 1)
#define SPR_FLIP_X (1 << 2)
#define SPR_FLIP_Y (1 << 3)
#define SPR_C_MATH (1 << 4)
#define SPR_DOUBLE (1 << 5)

typedef struct
{
    int16_t x;
    int16_t y;
    uint8_t windows;
    uint8_t flags;
} Background;

#define BG_C_MATH (1 << 0)

typedef struct
{
    uint16_t mainscreen_colour;
    uint16_t subscreen_colour;
    int16_t window_1_left;
    int16_t window_1_right;
    int16_t window_2_left;
    int16_t window_2_right;
    uint8_t bgcol_windows;
    uint8_t flags;
} MainState;

#define CMATH_HALF_MAIN_SCREEN (1 << 0)
#define CMATH_DOUBLE_MAIN_SCREEN (1 << 1)
#define CMATH_HALF_SUB_SCREEN (1 << 2)
#define CMATH_DOUBLE_SUB_SCREEN (1 << 3)
#define CMATH_ADD_SUB_SCREEN (1 << 4)
#define CMATH_SUB_SUB_SCREEN (1 << 5)
#define CMATH_FADE_ENABLE (1 << 6)
#define CMATH_CMATH_ENABLE (1 << 7)

typedef struct
{
    uint16_t screen_fade;
    uint8_t flags;
} CMathState;

#define MAIN_SPR0_ENABLE (1 << 0)
#define MAIN_SPR1_ENABLE (1 << 1)
#define MAIN_BG0_ENABLE (1 << 2)
#define MAIN_BG1_ENABLE (1 << 3)
#define MAIN_CMATH_ENABLE (1 << 4)
#define MAIN_BGCOL_WINDOW_ENABLE (1 << 5)

#define WINDOW_A (0b0001)
#define WINDOW_B (0b0010)
#define WINDOW_AB (0b0100)
#define WINDOW_X (0b1000)
#define WINDOW_ALL (0b1111)

typedef uint16_t uint16x8_t __attribute__((vector_size(16)));
typedef int16_t int16x8_t __attribute__((vector_size(16)));
typedef uint16_t mask16x8_t __attribute__((vector_size(16)));

extern MainState SASPPU_main_state;
extern Background SASPPU_bg0_state;
extern Background SASPPU_bg1_state;
extern CMathState SASPPU_cmath_state;
extern uint8_t SASPPU_hdma_enable;

extern Sprite SASPPU_oam[SPRITE_COUNT];
extern uint16_t SASPPU_bg0[MAP_WIDTH * MAP_HEIGHT];
extern uint16_t SASPPU_bg1[MAP_WIDTH * MAP_HEIGHT];

extern uint16x8_t SASPPU_background[BG_WIDTH * BG_HEIGHT / 8];
extern uint16x8_t SASPPU_sprites[SPR_WIDTH * SPR_HEIGHT / 8];

extern Sprite *SASPPU_sprite_cache[2][SPRITE_CACHE];

extern bool SASPPU_forced_blank;

#if __STDC_VERSION__ >= 202000
typedef enum : uint16_t
#else
typedef enum
#endif
{
    HDMA_NOOP = 0,
    HDMA_DISABLE,
    HDMA_MAIN_STATE_MAINSCREEN_COLOUR,
    HDMA_MAIN_STATE_SUBSCREEN_COLOUR,
    HDMA_MAIN_STATE_WINDOW1_LEFT,
    HDMA_MAIN_STATE_WINDOW1_RIGHT,
    HDMA_MAIN_STATE_WINDOW2_LEFT,
    HDMA_MAIN_STATE_WINDOW2_RIGHT,
    HDMA_MAIN_STATE_BGCOL_WINDOWS,
    HDMA_MAIN_STATE_FLAGS,
    HDMA_CMATH_STATE_SCREEN_FADE,
    HDMA_CMATH_STATE_FLAGS,
    HDMA_BACKGROUND0_X,
    HDMA_BACKGROUND0_Y,
    HDMA_BACKGROUND0_WINDOWS,
    HDMA_BACKGROUND0_FLAGS,
    HDMA_BACKGROUND1_X,
    HDMA_BACKGROUND1_Y,
    HDMA_BACKGROUND1_WINDOWS,
    HDMA_BACKGROUND1_FLAGS,
    HDMA_HDMA_ENABLE,
    HDMA_LAST,
} HDMACommand;

typedef struct
{
    HDMACommand command;
    uint16_t value;
} HDMAEntry;

#define SASPPU_HDMA_TABLE_COUNT (8)
#define HDMA_LEN (240)

extern HDMAEntry SASPPU_hdma_tables[SASPPU_HDMA_TABLE_COUNT][HDMA_LEN];

void SASPPU_render(uint16x8_t *fb, uint8_t section);

#endif // SASPPU_SASPPU_H_
