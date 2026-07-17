/**
 * @file sasppu_internal.h
 * @author john hunter <moliveofscratch@gmail.com>
 * @brief Internal header to SASPPU. Don't include.
 * @version 2.0
 * @date 2026-07-17
 *
 * @copyright Copyright (c) 2026
 *
 */

#ifndef SASPPU_INTERNAL_H_
#define SASPPU_INTERNAL_H_

#ifndef SASPPU_ESP
#define SASPPU_ESP 1
#endif

#ifndef USE_INLINE_ASM
#define USE_INLINE_ASM 1
#endif
#ifndef VERIFY_INLINE_ASM
#define VERIFY_INLINE_ASM 0
#endif
#ifndef USE_GCC_SIMD
#define USE_GCC_SIMD ((!USE_INLINE_ASM) | VERIFY_INLINE_ASM)
#endif

#ifndef QEMU_EMULATOR
#define QEMU_EMULATOR 0
#endif

#include "assert.h"
#include "sasppu/sasppu.h"
#include "stddef.h"
#include "unistd.h"
#if SASPPU_ESP
#include "esp_log.h"
#else
#define ESP_LOGI
#endif

void SASPPU_assert_fail();

extern uint16x8_t SASPPU_sub_screen[240 / 8];
extern SpriteCache SASPPU_sprite_cache;
extern mask16x8_t SASPPU_window_cache[(240 / 8) * 2];

extern MainState SASPPU_main_state;
extern CMathState SASPPU_cmath_state;
extern BackgroundState SASPPU_background_state;
extern BackgroundPlane SASPPU_background_plane;
extern SpritePlane SASPPU_sprite_plane;
extern BackgroundMap SASPPU_background_map;
extern SpriteState SASPPU_sprite_state;

#define SIMD_ASSERT(index, cpu_val, simd_val)                                  \
  if (cpu_val[index] == simd_val[index]) {                                     \
  } else {                                                                     \
    ESP_LOGI("SIMD_ASRT", "cpu_val[%u]: %u, simd_val[%u]: %u", index,          \
             cpu_val[index], index, simd_val[index]);                          \
    assert(cpu_val[index] == simd_val[index]);                                 \
  }

#define CHECK_SIMD(cpu_val, asm_segment)                                       \
  {                                                                            \
    volatile uint16x8_t check = VBROADCAST(0);                                 \
    asm_segment;                                                               \
    SIMD_ASSERT(0, cpu_val, check);                                            \
    SIMD_ASSERT(1, cpu_val, check);                                            \
    SIMD_ASSERT(2, cpu_val, check);                                            \
    SIMD_ASSERT(3, cpu_val, check);                                            \
    SIMD_ASSERT(4, cpu_val, check);                                            \
    SIMD_ASSERT(5, cpu_val, check);                                            \
    SIMD_ASSERT(6, cpu_val, check);                                            \
    SIMD_ASSERT(7, cpu_val, check);                                            \
  }

#define CHECK_SIMD_Q0(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q0, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q1(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q1, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q2(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q2, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q3(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q3, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q4(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q4, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q5(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q5, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q6(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q6, %[check], 0" : : [check] "r"(&check)));
#define CHECK_SIMD_Q7(cpu_val)                                                 \
  CHECK_SIMD(cpu_val, asm volatile inline(                                     \
                          "st.qr q7, %[check], 0" : : [check] "r"(&check)));

#if USE_INLINE_ASM
typedef void (*HandleWindowType)(uint16x8_t *const scanline, const uint16_t x);
#else
typedef void (*HandleWindowType)(uint16x8_t *const scanline, const uint16_t x,
                                 const uint16x8_t col);
#endif

typedef void (*HandleSpriteType)(uint16x8_t *const scanline, const int16_t y,
                                 Sprite *const sprite);
typedef void (*HandleCMathType)(uint16x8_t *const scanline);
typedef void (*HandleScanlineType)(uint16x8_t *const scanline, const int16_t y);
typedef void (*HandleBackgroundType)(uint16x8_t *const scanline,
                                     const int16_t y);

#define SCANLINE_END (240 - 8)
#define VBROADCAST(val) {val, val, val, val, val, val, val, val}
extern const uint16x8_t VECTOR_INCREMENTS;
extern const uint16x8_t VECTOR_SCANLINE_END;
extern const uint16x8_t VECTOR_INCREMENTS_END;

extern const uint16_t SASPPU_EIGHT;
extern const uint16_t CMATH_BIT;
extern const uint16_t CMATH_MASK_LOW;
extern const uint16_t CMATH_MASK_GREEN;
extern const uint16_t CMATH_MASK_SPLIT;
extern const uint16_t CMATH_ONE;
extern const uint16_t CMATH_TWO_FOUR;
extern const uint16_t CMATH_TWO_FIVE;
extern const uint16_t CMATH_TWO_EIGHT;
extern const uint16_t CMATH_TWO_NINE;
extern const uint16_t CMATH_TWO_TEN;

extern const HandleSpriteType HANDLE_SPRITE_LOOKUP[16];
extern const HandleScanlineType HANDLE_SCANLINE_LOOKUP[64];
extern const HandleCMathType HANDLE_CMATH_LOOKUP[256];
extern const HandleWindowType HANDLE_WINDOW_LOOKUP[256];

void handle_bg0(uint16x8_t *const scanline, const int16_t y);
void handle_bg1(uint16x8_t *const scanline, const int16_t y);

#if USE_GCC_SIMD
extern const uint16x8_t REVERSE_MASK;
extern const uint16x8_t INTERLEAVE_MASK_LOW;
extern const uint16x8_t INTERLEAVE_MASK_HIGH;
extern const uint16x8_t VECTOR_SHUFFLES[9];
#endif

#ifndef __builtin_shuffle
static inline uint16x8_t SHUFFLE_1(uint16x8_t a, uint16x8_t shuf) {
  uint16x8_t out = VBROADCAST(0);
  ssize_t i = 7;
  do {
    out[i] = a[shuf[i]];
  } while ((--i) >= 0);
  return out;
}
static inline uint16x8_t SHUFFLE_2(uint16x8_t a, uint16x8_t b,
                                   uint16x8_t shuf) {
  uint16x8_t out = VBROADCAST(0);
  ssize_t i = 7;
  do {
    if (shuf[i] < 8) {
      out[i] = a[shuf[i]];
    } else {
      out[i] = b[shuf[i] - 8];
    }
  } while ((--i) >= 0);
  return out;
}
#else
#define SHUFFLE_1(a, shuf) __builtin_shuffle(a, shuf)
#define SHUFFLE_2(a, b, shuf) __builtin_shuffle(a, b, shuf)
#endif

#endif // SASPPU_INTERNAL_H_