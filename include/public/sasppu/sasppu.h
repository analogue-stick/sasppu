/**
 * @file sasppu.h
 * @author John Hunter <moliveofscratch@gmail.com>
 * @brief The main include file for SASPPU.
 * @version 2.0
 * @date 2026-07-17
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef SASPPU_SASPPU_H_
#define SASPPU_SASPPU_H_

#include "stdbool.h"
#include "stdint.h"

#define SASPPU_VERSION_MAJOR 2
#define SASPPU_VERSION_MINOR 0
#define SASPPU_VERSION_PATCH 0

// #define SASPPU_VERSION
// "SASPPU_VERSION_MAJOR.SASPPU_VERSION_MINOR.SASPPU_VERSION_PATCH"

#define WINDOW_A (0b0001)
#define WINDOW_B (0b0010)
#define WINDOW_AB (0b0100)
#define WINDOW_X (0b1000)
#define WINDOW_ALL (0b1111)

typedef struct {
  int16_t x;
  int16_t y;
  uint8_t width;
  uint8_t height;
  uint8_t graphics_x;
  uint8_t graphics_y;
  uint8_t windows;
  uint8_t flags;
} Sprite;

#define DEFAULT_SPRITE                                                         \
  Sprite{                                                                      \
    x : 0,                                                                     \
    y : 0,                                                                     \
    width : 8,                                                                 \
    height : 8,                                                                \
    graphics_x : 0,                                                            \
    graphics_y : 0,                                                            \
    windows : 0xFF,                                                            \
    flags : 0                                                                  \
  }

#define SPR_USER_TYPE (7)
#define SPR_ENABLED (1 << 3)
#define SPR_FLIP_X (1 << 4)
#define SPR_FLIP_Y (1 << 5)
#define SPR_C_MATH (1 << 6)
#define SPR_DOUBLE (1 << 7)

typedef struct {
  int16_t scroll_x;
  int16_t scroll_y;
  uint8_t windows;
  uint8_t flags;
} BackgroundState;

#define DEFAULT_BACKGROUND_STATE                                               \
  BackgroundState{scroll_x : 0, scroll_y : 0, windows : 0xFF, flags : 0}

#define BG_C_MATH (1 << 0)

typedef struct {
  uint16_t screen_fade;
  uint8_t flags;
} CMathState;

#define DEFAULT_CMATH_STATE CMathState{screen_fade : 0, flags : 0}

#define CMATH_HALF_MAIN_SCREEN (1 << 0)
#define CMATH_DOUBLE_MAIN_SCREEN (1 << 1)
#define CMATH_HALF_SUB_SCREEN (1 << 2)
#define CMATH_DOUBLE_SUB_SCREEN (1 << 3)
#define CMATH_ADD_SUB_SCREEN (1 << 4)
#define CMATH_SUB_SUB_SCREEN (1 << 5)
#define CMATH_FADE_ENABLE (1 << 6)
#define CMATH_CMATH_ENABLE (1 << 7)

typedef struct {
  uint16_t mainscreen_colour;
  uint16_t subscreen_colour;

  // windowing
  int16_t window_1_left;
  int16_t window_1_right;
  int16_t window_2_left;
  int16_t window_2_right;
  uint8_t bgcol_windows;

  uint8_t flags;
} MainState;

#define DEFAULT_MAIN_STATE                                                     \
  MainState {                                                                  \
  mainscreen_colour:                                                           \
    0, subscreen_colour : 0, window_1_left : 0, window_1_right : 255,          \
        window_2_left : 0, window_2_right : 255, bgcol_windows : 0xFF,         \
        flags : 0,                                                             \
  }

#define MAIN_CMATH_ENABLE (1 << 0)
#define MAIN_BGCOL_WINDOW_ENABLE (1 << 1)

#define BG_WIDTH_POWER (8)
#define BG_HEIGHT_POWER (8)
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

typedef uint16_t uint16x8_t __attribute__((vector_size(16)));
typedef int16_t int16x8_t __attribute__((vector_size(16)));
typedef uint16_t mask16x8_t __attribute__((vector_size(16)));

typedef Sprite *SpriteCache[SPRITE_CACHE];

typedef uint16x8_t *BackgroundPlane; // [(BG_WIDTH / 8) * BG_HEIGHT];
typedef uint16x8_t *SpritePlane;     // [SPR_WIDTH * SPR_HEIGHT / 8];
typedef Sprite *SpriteState;         // [SPRITE_COUNT];
typedef uint16_t *BackgroundMap;     // [MAP_WIDTH * MAP_HEIGHT];

void alloc_background_plane(BackgroundPlane &plane);
void alloc_sprite_plane(SpritePlane &plane);
void alloc_sprite_state(SpriteState &state);
void alloc_background_map(BackgroundMap &map);

void free_background_plane(BackgroundPlane plane);
void free_sprite_plane(SpritePlane plane);
void free_sprite_state(SpriteState state);
void free_background_map(BackgroundMap map);

#define HDMA_LEN (240)

#if __STDC_VERSION__ >= 202000
typedef enum : uint16_t
#else
typedef enum
#endif
{ HDMA_NOOP = 0,
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
  HDMA_BACKGROUND_X,
  HDMA_BACKGROUND_Y,
  HDMA_BACKGROUND_WINDOWS,
  HDMA_BACKGROUND_FLAGS,
  HDMA_LAST,
} HDMACommand;

typedef struct {
  HDMACommand command;
  uint16_t value;
} HDMAEntry;

typedef HDMAEntry HdmaTable[HDMA_LEN];

#if __STDC_VERSION__ >= 202000
typedef enum : uint16_t
#else
typedef enum
#endif
{ CMD_BIND_MAIN_STATE,
  CMD_BIND_C_MATH_STATE,
  CMD_BIND_BACKGROUND_PLANE,
  CMD_BIND_SPRITE_PLANE,
  CMD_BIND_BACKGROUND_MAP,
  CMD_BIND_BACKGROUND_STATE,
  CMD_BIND_SPRITE_STATE,
  CMD_APPLY_HDMA,
  CMD_BEGIN_FRAME,
  CMD_UPDATE_WINDOWS,
  CMD_DRAW_BACKGROUND,
  CMD_DRAW_SPRITES,
  CMD_END_FRAME,
} BufferCommand;

typedef struct {
  BufferCommand command;
  void *data;
} BufferEntry;

typedef struct {
  BufferEntry *buf;
} CommandBuffer;

extern bool SASPPU_forced_blank;

void SASPPU_render(uint16x8_t *fb, uint8_t section);

#endif // SASPPU_SASPPU_H_
