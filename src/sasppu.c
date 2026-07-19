#include "sasppu/sasppu.h"
#include "assert.h"
#include "sasppu/internal.h"
#include "stdalign.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "stdlib.h"

#include "sasppu/gen.h"
#include <stdint.h>

#if SASPPU_ESP
#include "esp_attr.h"
#include "sdkconfig.h"
#else
#define EXT_RAM_BSS_ATTR
#endif

uint16x8_t SASPPU_sub_screen[240 / 8];
SpriteCache SASPPU_sprite_cache;
mask16x8_t SASPPU_window_cache[(240 / 8) * 2];

MainState SASPPU_main_state;
CMathState SASPPU_cmath_state;
BackgroundState SASPPU_background_state;
BackgroundPlane SASPPU_background_plane;
SpritePlane SASPPU_sprite_plane;
BackgroundMap SASPPU_background_map;
SpriteState SASPPU_sprite_state;

bool SASPPU_forced_blank;
static bool already_blanked[4] = {false, false, false, false};

const uint16x8_t VECTOR_INCREMENTS = {0, 1, 2, 3, 4, 5, 6, 7};
const uint16x8_t VECTOR_SCANLINE_END = VBROADCAST(SCANLINE_END);
const uint16x8_t VECTOR_INCREMENTS_END =
    VECTOR_INCREMENTS + VECTOR_SCANLINE_END;

const uint16_t SASPPU_EIGHT = 8;
const uint16_t CMATH_BIT = 0x8000;
const uint16_t CMATH_MASK_LOW = 0b0000000000011111;
const uint16_t CMATH_MASK_GREEN = 0b0000000001100000;
const uint16_t CMATH_MASK_SPLIT = 0b0111110000000000;
const uint16_t CMATH_ONE = 1;
const uint16_t CMATH_TWO_FOUR = 1 << 4;
const uint16_t CMATH_TWO_FIVE = 1 << 5;
const uint16_t CMATH_TWO_EIGHT = 1 << 8;
const uint16_t CMATH_TWO_NINE = 1 << 9;
const uint16_t CMATH_TWO_TEN = 1 << 10;

const HandleSpriteType HANDLE_SPRITE_LOOKUP[16];
const HandleScanlineType HANDLE_SCANLINE_LOOKUP[64];
const HandleCMathType HANDLE_CMATH_LOOKUP[256];
const HandleWindowType HANDLE_WINDOW_LOOKUP[256];

#if USE_GCC_SIMD
const uint16x8_t REVERSE_MASK = {7, 6, 5, 4, 3, 2, 1, 0};
const uint16x8_t INTERLEAVE_MASK_LOW = {0, 8, 1, 9, 2, 10, 3, 11};
const uint16x8_t INTERLEAVE_MASK_HIGH = {4, 12, 5, 13, 6, 14, 7, 15};
const uint16x8_t VECTOR_SHUFFLES[9] = {
    {0, 1, 2, 3, 4, 5, 6, 7},       {1, 2, 3, 4, 5, 6, 7, 8},
    {2, 3, 4, 5, 6, 7, 8, 9},       {3, 4, 5, 6, 7, 8, 9, 10},
    {4, 5, 6, 7, 8, 9, 10, 11},     {5, 6, 7, 8, 9, 10, 11, 12},
    {6, 7, 8, 9, 10, 11, 12, 13},   {7, 8, 9, 10, 11, 12, 13, 14},
    {8, 9, 10, 11, 12, 13, 14, 15},
};
#endif

void SASSPU_alloc_background_plane(BackgroundPlane *plane) {
  *plane = (BackgroundPlane)(aligned_alloc(
      alignof(uint16x8_t), (BG_WIDTH * BG_HEIGHT / 8) * sizeof(uint16x8_t)));
}
void SASSPU_alloc_sprite_plane(SpritePlane *plane) {
  *plane = (SpritePlane)(aligned_alloc(
      alignof(uint16x8_t), (SPR_WIDTH * SPR_HEIGHT / 8) * sizeof(uint16x8_t)));
}
void SASSPU_alloc_sprite_state(SpriteState *state) {
  *state = (SpriteState)(aligned_alloc(alignof(Sprite),
                                       SPRITE_COUNT * sizeof(Sprite)));
}
void SASSPU_alloc_background_map(BackgroundMap *map) {
  *map = (BackgroundMap)(aligned_alloc(
      alignof(uint16_t), (MAP_WIDTH * MAP_HEIGHT) * sizeof(uint16_t)));
}

void SASSPU_calloc_background_plane(BackgroundPlane *plane) {
  SASSPU_alloc_background_plane(plane);
  if (*plane) {
    memset(*plane, 0, (BG_WIDTH * BG_HEIGHT / 8) * sizeof(uint16x8_t));
  }
}
void SASSPU_calloc_sprite_plane(SpritePlane *plane) {
  SASSPU_alloc_sprite_plane(plane);
  if (plane) {
    memset(plane, 0, (SPR_WIDTH * SPR_HEIGHT / 8) * sizeof(uint16x8_t));
  }
}
void SASSPU_calloc_sprite_state(SpriteState *state) {
  SASSPU_alloc_sprite_state(state);
  if (*state) {
    memset(*state, 0, SPRITE_COUNT * sizeof(Sprite));
  }
}
void SASSPU_calloc_background_map(BackgroundMap *map) {
  SASSPU_alloc_background_map(map);
  if (*map) {
    memset(*map, 0, (MAP_WIDTH * MAP_HEIGHT) * sizeof(uint16_t));
  }
}

void SASSPU_free_background_plane(BackgroundPlane plane) { free(plane); }
void SASSPU_free_sprite_plane(SpritePlane plane) { free(plane); }
void SASSPU_free_sprite_state(SpriteState state) { free(state); }
void SASSPU_free_background_map(BackgroundMap map) { free(map); }

static inline void handle_background(uint16x8_t *const scanline, int16_t y) {
  size_t y_pos = (((size_t)(y + SASPPU_background_state.scroll_y)) >> 3) &
                 ((MAP_HEIGHT)-1);
  size_t x_pos = (((size_t)(240 - 8 + SASPPU_background_state.scroll_x)) >> 3) &
                 ((MAP_WIDTH)-1);
  size_t offset_x = ((size_t)(SASPPU_background_state.scroll_x) & 0x7);
  size_t offset_y = ((size_t)(y + SASPPU_background_state.scroll_y) & 0x7);

#if USE_INLINE_ASM
  asm volatile inline("wur.sar_byte %[offset_x]"
                      :
                      : [offset_x] "r"(offset_x << 1));
#endif

  uint16_t bg_map = SASPPU_background_map[y_pos * MAP_WIDTH + x_pos];

  uint16x8_t *bg_1_p;

  if ((bg_map & 0b10) > 0) {
    bg_1_p = &SASPPU_background_plane[(size_t)(bg_map >> 2) +
                                      ((7 - offset_y) * (BG_WIDTH >> 3))];
  } else {
    bg_1_p = &SASPPU_background_plane[(size_t)(bg_map >> 2) +
                                      (offset_y * (BG_WIDTH >> 3))];
  };

#if USE_GCC_SIMD
  uint16x8_t bg_1;
  uint16x8_t bg_2;
  uint16x8_t bg;
#endif

  if ((bg_map & 0b01) > 0) {
#if USE_INLINE_ASM
    asm volatile inline("                      \n\t \
            ld.qr q1, %[bg_1_p], 0 \n\t \
            ee.vzip.16 q0, q1      \n\t \
            ee.vzip.16 q1, q0      \n\t \
            ee.vzip.16 q0, q1      \n\t \
            ee.vzip.16 q1, q0      \n\t \
            "
                        :
                        : [bg_1_p] "r"(bg_1_p));
#endif
#if USE_GCC_SIMD
    bg_1 = SHUFFLE_1(*bg_1_p, REVERSE_MASK);
#endif
#if VERIFY_INLINE_ASM
    CHECK_SIMD_Q0(bg_1);
#endif
  } else {
#if USE_INLINE_ASM
    asm volatile inline("ld.qr q0, %[bg_1_p], 0" : : [bg_1_p] "r"(bg_1_p));
#endif
#if USE_GCC_SIMD
    bg_1 = *bg_1_p;
#endif
#if VERIFY_INLINE_ASM
    CHECK_SIMD_Q0(bg_1);
#endif
  }

  ssize_t x = (240 / 8) - 1;
  do {
#if USE_INLINE_ASM
    asm volatile inline("mv.qr q1, q0");
#endif
#if USE_GCC_SIMD
    bg_2 = bg_1;
#endif
#if VERIFY_INLINE_ASM
    CHECK_SIMD_Q1(bg_2);
#endif
    x_pos = (x_pos - 1) & ((MAP_WIDTH)-1);

    bg_map = SASPPU_background_map[y_pos * MAP_WIDTH + x_pos];

    if ((bg_map & 0b10) > 0) {
      bg_1_p = &SASPPU_background_plane[(size_t)(bg_map >> 2) +
                                        ((7 - offset_y) * (BG_WIDTH >> 3))];
    } else {
      bg_1_p = &SASPPU_background_plane[(size_t)(bg_map >> 2) +
                                        (offset_y * (BG_WIDTH >> 3))];
    };

    if ((bg_map & 0b01) > 0) {
#if USE_INLINE_ASM
      asm volatile inline("                      \n\t \
                ld.qr q2, %[bg_1_p], 0 \n\t \
                ee.vzip.16 q0, q2      \n\t \
                ee.vzip.16 q2, q0      \n\t \
                ee.vzip.16 q0, q2      \n\t \
                ee.vzip.16 q2, q0      \n\t \
                "
                          :
                          : [bg_1_p] "r"(bg_1_p));
#endif
#if USE_GCC_SIMD
      bg_1 = SHUFFLE_1(*bg_1_p, REVERSE_MASK);
#endif
#if VERIFY_INLINE_ASM
      CHECK_SIMD_Q0(bg_1);
#endif
    } else {
#if USE_INLINE_ASM
      asm volatile inline("ld.qr q0, %[bg_1_p], 0" : : [bg_1_p] "r"(bg_1_p));
#endif
#if USE_GCC_SIMD
      bg_1 = *bg_1_p;
#endif
#if VERIFY_INLINE_ASM
      CHECK_SIMD_Q0(bg_1);
#endif
    }

#if USE_INLINE_ASM
    asm volatile inline("                                           \n\t \
            mv.qr q2, q0                                \n\t \
            ee.src.q.ld.ip q3, %[cmath_bit], 0, q2, q1     \n\t \
            "
                        :
                        : [cmath_bit] "r"(&CMATH_BIT));
#endif
#if USE_GCC_SIMD
    bg = SHUFFLE_2(bg_1, bg_2, VECTOR_SHUFFLES[offset_x]);
#endif
#if VERIFY_INLINE_ASM
    CHECK_SIMD_Q2(bg);
#endif

    if ((SASPPU_background_state.flags & BG_C_MATH) > 0) {
#if USE_INLINE_ASM
      asm volatile inline("ee.orq q2, q2, q3");
#endif
#if USE_GCC_SIMD
      bg |= CMATH_BIT;
#endif
#if VERIFY_INLINE_ASM
      CHECK_SIMD_Q2(bg);
#endif
    }

    HANDLE_WINDOW_LOOKUP[SASPPU_background_state.windows]
#if USE_INLINE_ASM
        (scanline, x);
#else
        (scanline, x, bg);
#endif
  } while ((--x) >= 0);
}

static void command_update_windows() {
  mask16x8_t *window_index = &SASPPU_window_cache[((240 / 8) * 2) - 1];
#if USE_INLINE_ASM
  asm volatile inline(
      "                                                               \n\t \
                 ld.qr q0, %[vector_increments], 0 /* x_window */                \n\t \
                 ee.vldbc.16.ip q4, %[main_state], 2 /* load window_1_left */    \n\t \
                 ee.vldbc.16.ip q5, %[main_state], 2 /* load window_1_right */   \n\t \
                 ee.vldbc.16.ip q6, %[main_state], 2 /* load window_2_left  */   \n\t \
                 ee.vldbc.16.ip q7, %[main_state], 2 /* load window_2_right */  \n\t \
                 "
      :
      : [vector_increments] "r"(&VECTOR_INCREMENTS_END),
        [main_state] "r"(&SASPPU_main_state.window_1_left));

  do {
    asm volatile inline(
        "                                                               \n\t \
                     /* Calculate window 2 */                                        \n\t \
                     ee.vcmp.eq.s16 q1, q0, q6                                       \n\t \
                     ee.vcmp.gt.s16 q2, q0, q6                                       \n\t \
                     ee.orq q1, q1, q2                                               \n\t \
                     ee.vcmp.eq.s16 q2, q0, q7                                       \n\t \
                     ee.vcmp.lt.s16 q3, q0, q7                                       \n\t \
                     ee.orq q2, q2, q3                                               \n\t \
                     ee.andq q1, q1, q2                                              \n\t \
                     ee.vst.128.ip q1, %[window_index], -16                          \n\t \
                                                                                     \n\t \
                     /* Calculate window 1 */                                        \n\t \
                     ee.vcmp.eq.s16 q1, q0, q4                                       \n\t \
                     ee.vcmp.gt.s16 q2, q0, q4                                       \n\t \
                     ee.orq q1, q1, q2                                               \n\t \
                     ee.vcmp.eq.s16 q2, q0, q5                                       \n\t \
                     ee.vcmp.lt.s16 q3, q0, q5                                       \n\t \
                     ee.orq q2, q2, q3                                               \n\t \
                     ee.andq q1, q1, q2                                              \n\t \
                     ee.vst.128.ip q1, %[window_index], -16                          \n\t \
                                                                                     \n\t \
                     /* Jump down 8 pixels */                                        \n\t \
                     ee.vldbc.16.ip q2, %[eight], 0                                  \n\t \
                     ee.vsubs.s16 q0, q0, q2                                         \n\t \
                     "
        : [window_index] "+r"(window_index)
        : [eight] "r"(&SASPPU_EIGHT));
  } while (window_index >= SASPPU_window_cache);
#else
  uint16x8_t x_window = VECTOR_INCREMENTS_END;
  uint16x8_t window_1_left = VBROADCAST(SASPPU_main_state.window_1_left);
  uint16x8_t window_1_right = VBROADCAST(SASPPU_main_state.window_1_right);
  uint16x8_t window_2_left = VBROADCAST(SASPPU_main_state.window_2_left);
  uint16x8_t window_2_right = VBROADCAST(SASPPU_main_state.window_2_right);
  do {
    /* Calculate window 2 */
    *(window_index--) =
        (x_window >= window_2_left) & (x_window <= window_2_right);

    /* Calculate window 1 */
    *(window_index--) =
        (x_window >= window_1_left) & (x_window <= window_1_right);

    /* Jump down 8 pixels */
    x_window -= 8;
  } while (window_index > SASPPU_window_cache);
#endif
#if VERIFY_INLINE_ASM
  {
    size_t x = 0;
    do {
      if ((x >= SASPPU_main_state.window_1_left) &
          (x <= SASPPU_main_state.window_1_right)) {
        assert(SASPPU_window_cache[((x >> 3) * 2) + 0][x & 0x7] == 0xFFFF);
      } else {
        assert(SASPPU_window_cache[((x >> 3) * 2) + 0][x & 0x7] == 0);
      }
      if ((x >= SASPPU_main_state.window_2_left) &
          (x <= SASPPU_main_state.window_2_right)) {
        assert(SASPPU_window_cache[((x >> 3) * 2) + 1][x & 0x7] == 0xFFFF);
      } else {
        assert(SASPPU_window_cache[((x >> 3) * 2) + 1][x & 0x7] == 0);
      }
    } while ((++x) < 240);
  }
#endif
}

static void command_begin_frame(uint16x8_t *const scanline) {
  HandleWindowType main_win =
      HANDLE_WINDOW_LOOKUP[SASPPU_main_state.bgcol_windows & 0x0F];
  HandleWindowType sub_win =
      HANDLE_WINDOW_LOOKUP[SASPPU_main_state.bgcol_windows & 0xF0];
#if USE_INLINE_ASM
  if (SASPPU_main_state.flags & MAIN_BGCOL_WINDOW_ENABLE) {
    asm volatile inline("ee.zero.q q3");
  }
#endif

#if USE_INLINE_ASM
  asm volatile inline(
      "                                                                \n\t \
        ee.vldbc.16.ip q0, %[main_state], 2 /* load mainscreen_colour */ \n\t \
        ee.vldbc.16.ip q1, %[main_state], 2 /* load subscreen_colour */  \n\t \
        "
      :
      : [main_state] "r"(&SASPPU_main_state.mainscreen_colour));
#else
  static const uint16x8_t zero = VBROADCAST(0);
  uint16x8_t vsubcol = VBROADCAST(SASPPU_main_state.subscreen_colour);
  uint16x8_t vmaincol = VBROADCAST(SASPPU_main_state.mainscreen_colour);
#endif

  uint16x8_t *maincol = &scanline[(240 / 8) - 1];
  uint16x8_t *subcol = &SASPPU_sub_screen[(240 / 8) - 1];

  if (SASPPU_main_state.flags & MAIN_BGCOL_WINDOW_ENABLE) {
    ssize_t x = (240 / 8) - 1;
    do {
#if USE_INLINE_ASM
      asm volatile inline("                                   \n\t \
            ee.vst.128.ip q3, %[subcol], -16    \n\t \
            mv.qr q2, q1                        \n\t \
            "
                          : [subcol] "+r"(subcol));
      sub_win(scanline, x);
      asm volatile inline("                                   \n\t \
            ee.vst.128.ip q3, %[maincol], -16   \n\t \
            mv.qr q2, q0                        \n\t \
            "
                          : [maincol] "+r"(maincol));
      main_win(scanline, x);
#else
      *(maincol--) = zero;
      sub_win(scanline, x, vsubcol);
      *(subcol--) = zero;
      main_win(scanline, x, vmaincol);
#endif
    } while ((--x) >= 0);
  } else {
    do {
#if USE_INLINE_ASM
      asm volatile inline("                                   \n\t \
            ee.vst.128.ip q0, %[maincol], -16   \n\t \
            ee.vst.128.ip q1, %[subcol], -16    \n\t \
            "
                          : [subcol] "+r"(subcol), [maincol] "+r"(maincol)
                          :);
#else
      *(maincol--) = vmaincol;
      *(subcol--) = vsubcol;
#endif
    } while (maincol != scanline);
  }
}

static void command_draw_background(uint16x8_t *const scanline, int16_t y) {
  if (!SASPPU_background_map) {
    return;
  }
  if (!SASPPU_background_plane) {
    return;
  }
  handle_background(scanline, y);
}

static void command_draw_sprites(uint16x8_t *const scanline, int16_t y) {
  if (!SASPPU_sprite_plane) {
    return;
  }
  Sprite *const *spr = &SASPPU_sprite_cache[SPRITE_CACHE - 1];
  do {
    Sprite *const sprite = *spr;
    if (!sprite) {
      continue;
    }
    HANDLE_SPRITE_LOOKUP[sprite->flags >> 4](scanline, y, sprite);
  } while ((--spr) >= &SASPPU_sprite_cache[0]);
}

static void command_end_frame(uint16x8_t *const scanline) {
  if (SASPPU_main_state.flags & MAIN_CMATH_ENABLE) {
    return HANDLE_CMATH_LOOKUP[SASPPU_cmath_state.flags](scanline);
  } else {
    return HANDLE_CMATH_LOOKUP[0](scanline);
  }
}

static void command_apply_hdma(HDMATable *const table, uint8_t y) {
  HDMAEntry *entry = &(*table)[y];

  switch (entry->command) {
  case HDMA_NOOP:
  default:
    break;
  case HDMA_MAIN_STATE_MAINSCREEN_COLOUR: {
    SASPPU_main_state.mainscreen_colour = (uint16_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_SUBSCREEN_COLOUR: {
    SASPPU_main_state.subscreen_colour = (uint16_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_WINDOW1_LEFT: {
    SASPPU_main_state.window_1_left = (int16_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_WINDOW1_RIGHT: {
    SASPPU_main_state.window_1_right = (int16_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_WINDOW2_LEFT: {
    SASPPU_main_state.window_2_left = (int16_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_WINDOW2_RIGHT: {
    SASPPU_main_state.window_2_right = (int16_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_BGCOL_WINDOWS: {
    SASPPU_main_state.bgcol_windows = (uint8_t)(entry->value);
  } break;
  case HDMA_MAIN_STATE_FLAGS: {
    SASPPU_main_state.flags = (uint8_t)(entry->value);
  } break;
  case HDMA_CMATH_STATE_SCREEN_FADE: {
    SASPPU_cmath_state.screen_fade = (uint16_t)(entry->value);
  } break;
  case HDMA_CMATH_STATE_FLAGS: {
    SASPPU_cmath_state.flags = (uint8_t)(entry->value);
  } break;
  case HDMA_BACKGROUND_X: {
    SASPPU_background_state.scroll_x = (int16_t)(entry->value);
  } break;
  case HDMA_BACKGROUND_Y: {
    SASPPU_background_state.scroll_y = (int16_t)(entry->value);
  } break;
  case HDMA_BACKGROUND_WINDOWS: {
    SASPPU_background_state.windows = (uint8_t)(entry->value);
  } break;
  case HDMA_BACKGROUND_FLAGS: {
    SASPPU_background_state.flags = (uint8_t)(entry->value);
  } break;
  }
}

static inline void handle_sprite_cache(uint8_t y, uint8_t user_type) {
  uint32_t sprites_index = 0;
  size_t i = 0;
  do {
    Sprite *spr = &SASPPU_sprite_state[i];
    uint8_t flags = spr->flags;
    uint8_t windows = spr->windows;
    int16_t iy = (int16_t)y;

    int16_t spr_height = (int16_t)(spr->height);
    int16_t spr_width = (int16_t)(spr->width);

    bool main_screen_enable = (windows & 0x0F) > 0;
    bool sub_screen_enable = (windows & 0xF0) > 0;

    bool enabled = (flags & SPR_ENABLED) > 0;
    // bool flip_x = (flags & SPR_FLIP_X) > 0;
    // bool flip_y = (flags & SPR_FLIP_Y) > 0;
    // bool cmath_enabled = (flags & SPR_C_MATH) > 0;
    bool double_enabled = (flags & SPR_DOUBLE) > 0;
    uint8_t sprite_user_type = flags & SPR_USER_TYPE;

    // If not enabled, skip
    if (!enabled) {
      continue;
    }

    // If user type does not match, skip
    if (sprite_user_type != user_type) {
      continue;
    }

    // If we've hit the limit, skip
    if (sprites_index == SPRITE_CACHE) {
      continue;
    }

    bool window_enabled = (main_screen_enable) || (sub_screen_enable);
    bool top_border = spr->y <= iy;
    bool bottom_border = double_enabled ? (spr->y > (iy - (spr_height << 1)))
                                        : (spr->y > (iy - (spr_height)));
    bool right_border = spr->x < SCREEN_WIDTH;
    bool left_border =
        double_enabled ? (spr->x > -(spr_width << 1)) : (spr->x > -(spr_width));

    if (window_enabled && top_border && bottom_border && right_border &&
        left_border) {
      SASPPU_sprite_cache[sprites_index] = spr;
      sprites_index += 1;

      if (sprites_index == SPRITE_CACHE) {
        break;
      }
    }
  } while ((++i) < SPRITE_COUNT);
  while (sprites_index < SPRITE_CACHE) {
    SASPPU_sprite_cache[sprites_index] = NULL;
    sprites_index += 1;
  };
}

void SASPPU_render(uint16x8_t *fb, uint8_t section,
                   CommandBuffer *const command_buffer) {
  // Screen is rendered top to bottom for sanity's sake
  size_t y = 60 * section;

  // If we're forcing a blank, fill the screen black and return immediately
  if (SASPPU_forced_blank) {
    // If we've already filled this section in black we can call it a day here
    if (already_blanked[section]) {
      return;
    }

#if USE_INLINE_ASM
    asm volatile inline("ee.zero.q q0");
#else
    static const uint16x8_t zero = VBROADCAST(0);
#endif

    ssize_t x = ((240 / 8) * 60) - 1;
    uint16x8_t *section_pointer = fb + (y * 240 / 8) + x;
    do {
#if USE_INLINE_ASM
      asm volatile inline("                           \n\t \
                ee.vst.128.ip q0, %[section_pointer], -16   \n\t \
                "
                          : [section_pointer] "+r"(section_pointer)
                          :);
#else
      *(section_pointer--) = zero;
#endif
#if VERIFY_INLINE_ASM
      for (ssize_t i = 7; i >= 0; i--) {
        assert(*(section_pointer + 1)[i] == 0);
      }
#endif
    } while ((--x) >= 0);

    already_blanked[section] = true;
    return;
  }

  already_blanked[section] = false;

  SASPPU_main_state = DEFAULT_MAIN_STATE;
  SASPPU_cmath_state = DEFAULT_CMATH_STATE;
  SASPPU_background_state = DEFAULT_BACKGROUND_STATE;
  SASPPU_background_plane = NULL;
  SASPPU_sprite_plane = NULL;
  SASPPU_background_map = NULL;
  SASPPU_sprite_state = NULL;

  do {
    BufferEntry *entry = command_buffer->buf;
    uint16x8_t *scanline = fb + (y * 240 / 8);
    bool running = true;
    while (running) {
      switch (entry->command) {
      case CMD_BIND_MAIN_STATE: {
        SASPPU_main_state = *(MainState *)(entry->data);
      } break;
      case CMD_BIND_C_MATH_STATE: {
        SASPPU_cmath_state = *(CMathState *)(entry->data);
      } break;
      case CMD_BIND_BACKGROUND_PLANE: {
        SASPPU_background_plane = (BackgroundPlane)(entry->data);
      } break;
      case CMD_BIND_SPRITE_PLANE: {
        SASPPU_sprite_plane = (SpritePlane)(entry->data);
      } break;
      case CMD_BIND_BACKGROUND_MAP: {
        SASPPU_background_map = (BackgroundMap)(entry->data);
      } break;
      case CMD_BIND_BACKGROUND_STATE: {
        SASPPU_background_state = *(BackgroundState *)(entry->data);
      } break;
      case CMD_BIND_SPRITE_STATE: {
        {
          uint32_t sprites_index = SPRITE_CACHE;
          while (sprites_index != 0) {
            sprites_index -= 1;
            SASPPU_sprite_cache[sprites_index] = NULL;
          };
        }
        SASPPU_sprite_state = (SpriteState)(entry->data);
      } break;
      case CMD_APPLY_HDMA: {
        command_apply_hdma((HDMATable *)(entry->data), y);
      } break;
      case CMD_BEGIN_FRAME: {
        command_begin_frame(scanline);
      } break;
      case CMD_UPDATE_WINDOWS: {
        command_update_windows();
      } break;
      case CMD_DRAW_BACKGROUND: {
        command_draw_background(scanline, y);
      } break;
      case CMD_DRAW_SPRITES: {
        handle_sprite_cache(y, (uint8_t)(uint32_t)(entry->data));
        command_draw_sprites(scanline, y);
      } break;
      case CMD_END_FRAME: {
        command_end_frame(scanline);
        running = false;
      } break;
      default: {
        running = false;
      } break;
      }
      entry++;
    }
  } while ((++y) < (60 * (section + 1)));
}
