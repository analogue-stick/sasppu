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
#include "stddef.h"
#include "stdint.h"

#define SASPPU_VERSION_MAJOR 2
#define SASPPU_VERSION_MINOR 1
#define SASPPU_VERSION_PATCH 0

// Whether to edit internal structures to better interact with Micropython.
#ifndef SASPPU_MPY_COMPAT
#define SASPPU_MPY_COMPAT 0
#endif

#if SASPPU_MPY_COMPAT
#define SASPPU_MPY_BASE void *_mpy_base_pointer;
#define SASPPU_MPY_TUPLE_BASE SASPPU_MPY_BASE size_t len;
#define SASPPU_MPY_LIST_BASE SASPPU_MPY_BASE size_t alloc;
#define SASPPU_MPY_DEFAULT ._mpy_base_pointer = (void *)(0),
#define SASPPU_MPY_TUPLE_DEFAULT SASPPU_MPY_DEFAULT.len = 2,
#define SASPPU_MPY_LIST_DEFAULT SASPPU_MPY_DEFAULT.alloc = 0,
#else
#define SASPPU_MPY_BASE
#define SASPPU_MPY_TUPLE_BASE
#define SASPPU_MPY_LIST_BASE
#define SASPPU_MPY_DEFAULT
#define SASPPU_MPY_TUPLE_DEFAULT
#define SASPPU_MPY_LIST_DEFAULT
#endif

// #define SASPPU_VERSION
// "SASPPU_VERSION_MAJOR.SASPPU_VERSION_MINOR.SASPPU_VERSION_PATCH"

#define SCREEN_WIDTH (240)
#define SCREEN_HEIGHT (240)

#define WINDOW_A (0b0001)
#define WINDOW_B (0b0010)
#define WINDOW_AB (0b0100)
#define WINDOW_X (0b1000)
#define WINDOW_ALL (0b1111)

typedef struct {
  SASPPU_MPY_BASE
  int16_t x;
  int16_t y;
  uint8_t width;
  uint8_t height;
  uint8_t graphics_x;
  uint8_t graphics_y;
  uint8_t windows;
  uint8_t flags;
} Sprite;

static const Sprite DEFAULT_SPRITE = {
    SASPPU_MPY_DEFAULT.x = 0, .y = 0,          .width = 8,      .height = 8,
    .graphics_x = 0,          .graphics_y = 0, .windows = 0xFF, .flags = 0};

#define SPR_USER_TYPE (7)
#define SPR_ENABLED (1 << 3)
#define SPR_FLIP_X (1 << 4)
#define SPR_FLIP_Y (1 << 5)
#define SPR_C_MATH (1 << 6)
#define SPR_DOUBLE (1 << 7)

typedef struct {
  SASPPU_MPY_BASE
  int16_t scroll_x;
  int16_t scroll_y;
  uint8_t windows;
  uint8_t flags;
} BackgroundState;

static const BackgroundState DEFAULT_BACKGROUND_STATE = {
    SASPPU_MPY_DEFAULT.scroll_x = 0, .scroll_y = 0, .windows = 0xFF,
    .flags = 0};

#define BG_C_MATH (1 << 0)

typedef struct {
  SASPPU_MPY_BASE
  uint16_t screen_fade;
  uint8_t flags;
} CMathState;

static const CMathState DEFAULT_CMATH_STATE = {
    SASPPU_MPY_DEFAULT.screen_fade = 0, .flags = 0};

#define CMATH_HALF_MAIN_SCREEN (1 << 0)
#define CMATH_DOUBLE_MAIN_SCREEN (1 << 1)
#define CMATH_HALF_SUB_SCREEN (1 << 2)
#define CMATH_DOUBLE_SUB_SCREEN (1 << 3)
#define CMATH_ADD_SUB_SCREEN (1 << 4)
#define CMATH_SUB_SUB_SCREEN (1 << 5)
#define CMATH_FADE_ENABLE (1 << 6)
#define CMATH_CMATH_ENABLE (1 << 7)

typedef struct {
  SASPPU_MPY_BASE
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

static const MainState DEFAULT_MAIN_STATE = {
    SASPPU_MPY_DEFAULT.mainscreen_colour = 0,
    .subscreen_colour = 0,
    .window_1_left = 0,
    .window_1_right = 255,
    .window_2_left = 0,
    .window_2_right = 255,
    .bgcol_windows = 0xFF,
    .flags = 0,
};

#define MAIN_CMATH_ENABLE (1 << 0)
#define MAIN_BGCOL_WINDOW_ENABLE (1 << 1)

#define GRAPHICS_WIDTH_POWER_MAX (8)
#define GRAPHICS_HEIGHT_POWER_MAX (9)
#define GRAPHICS_WIDTH_POWER_MIN (3)
#define GRAPHICS_HEIGHT_POWER_MIN (3)
#define GRAPHICS_WIDTH_MAX (1 << GRAPHICS_WIDTH_POWER_MAX)
#define GRAPHICS_HEIGHT_MAX (1 << GRAPHICS_HEIGHT_POWER_MAX)
#define GRAPHICS_WIDTH_MIN (1 << GRAPHICS_WIDTH_POWER_MIN)
#define GRAPHICS_HEIGHT_MIN (1 << GRAPHICS_HEIGHT_POWER_MIN)

#define SPRITE_COUNT_POWER_MAX (8)
#define SPRITE_COUNT_POWER_MIN (0)
#define SPRITE_CACHE_POWER (4)
#define SPRITE_COUNT_MAX (1 << SPRITE_COUNT_POWER_MAX)
#define SPRITE_COUNT_MIN (1 << SPRITE_COUNT_POWER_MIN)
#define SPRITE_CACHE (1 << SPRITE_CACHE_POWER)

#define MAP_WIDTH_POWER_MAX (8)
#define MAP_HEIGHT_POWER_MAX (8)
#define MAP_WIDTH_POWER_MIN (0)
#define MAP_HEIGHT_POWER_MIN (0)
#define MAP_WIDTH_MAX (1 << MAP_WIDTH_POWER_MAX)
#define MAP_HEIGHT_MAX (1 << MAP_HEIGHT_POWER_MAX)
#define MAP_WIDTH_MIN (1 << MAP_WIDTH_POWER_MIN)
#define MAP_HEIGHT_MIN (1 << MAP_HEIGHT_POWER_MIN)

typedef uint16_t uint16x8_t __attribute__((vector_size(16)));
typedef int16_t int16x8_t __attribute__((vector_size(16)));
typedef uint16_t mask16x8_t __attribute__((vector_size(16)));

typedef Sprite *SpriteCache[SPRITE_CACHE];

typedef struct {
  SASPPU_MPY_BASE
  size_t width;
  size_t height;
  uint16x8_t *dat; // [(BG_WIDTH / 8) * BG_HEIGHT];
} GraphicsPlane;

typedef struct {
  SASPPU_MPY_LIST_BASE
  size_t count;
#if SASPPU_MPY_COMPAT
  Sprite **dat;
#else
  Sprite *dat;
#endif
} SpriteState;

typedef struct {
  SASPPU_MPY_BASE
  size_t width;
  size_t height;
  uint16_t *dat; // [MAP_WIDTH * MAP_HEIGHT];
} BackgroundMap;

static const GraphicsPlane DEFAULT_GRAPHICS_PLANE = {
    SASPPU_MPY_DEFAULT.width = 0, .height = 0, .dat = NULL};

static const SpriteState DEFAULT_SPRITE_STATE = {
    SASPPU_MPY_LIST_DEFAULT.count = 0, .dat = NULL};

static const BackgroundMap DEFAULT_BACKGROUND_MAP = {
    SASPPU_MPY_DEFAULT.width = 0, .height = 0, .dat = NULL};

void SASPPU_alloc_graphics_plane(GraphicsPlane *plane);
void SASPPU_alloc_sprite_state(SpriteState *state);
void SASPPU_alloc_background_map(BackgroundMap *map);

void SASPPU_calloc_graphics_plane(GraphicsPlane *plane);
void SASPPU_calloc_sprite_state(SpriteState *state);
void SASPPU_calloc_background_map(BackgroundMap *map);

void SASPPU_free_graphics_plane(GraphicsPlane plane);
void SASPPU_free_sprite_state(SpriteState state);
void SASPPU_free_background_map(BackgroundMap map);

#define HDMA_LEN SCREEN_HEIGHT

#if __STDC_VERSION__ >= 202000
typedef enum : uint32_t
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
  SASPPU_MPY_BASE
  HDMACommand command;
  uint16_t value;
} HDMAEntry;

typedef struct {
  SASPPU_MPY_LIST_BASE
#if SASPPU_MPY_COMPAT
  size_t count;
  HDMAEntry *buf[HDMA_LEN];
#else
  HDMAEntry buf[HDMA_LEN];
#endif
} HDMATable;

#if __STDC_VERSION__ >= 202000
typedef enum : uint32_t
#else
typedef enum
#endif
{ CMD_NOOP,
  CMD_BIND_MAIN_STATE,
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
  CMD_LAST,
} BufferCommand;

typedef struct {
  SASPPU_MPY_BASE
  BufferCommand command;
  void *data;
} BufferEntry;

typedef struct {
  SASPPU_MPY_LIST_BASE
#if SASPPU_MPY_COMPAT
  size_t count;
  BufferEntry **buf;
#else
  BufferEntry *buf;
#endif
} CommandBuffer;

extern bool SASPPU_forced_blank;

void SASPPU_render(uint16x8_t *fb, uint8_t section,
                   CommandBuffer *command_buffer);

#endif // SASPPU_SASPPU_H_
