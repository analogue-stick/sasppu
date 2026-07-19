#include "sasppu/sasppu.h"
#include "sasppu/internal.h"
#include <unistd.h>

// SASPPU_HANDLE_SPRITE(IDENT, FLIP_X, FLIP_Y, CMATH, DOUBLE)
#ifndef IDENT
#define IDENT handle_sprite_1_1_1_1
#endif

#ifndef FLIP_X
#define FLIP_X 1
#endif
#ifndef FLIP_Y
#define FLIP_Y 1
#endif
#ifndef CMATH
#define CMATH 1
#endif
#ifndef DOUBLE
#define DOUBLE 1
#endif

static void IDENT(uint16x8_t *const scanline, const int16_t y, Sprite *const sprite)
{
#if DOUBLE
    uint8_t sprite_width = sprite->width << 1;
#else
    uint8_t sprite_width = sprite->width;
#endif

    size_t offset = (size_t)(8 - (sprite->x & 0x7));

#if SASPPU_QEMU_EMULATOR
    offset = offset & 0x7;
#endif

#if USE_INLINE_ASM
    asm volatile inline("wur.sar_byte %[offset]" : : [offset] "r"(offset << 1));
#endif

    int16_t offset_y_gen = y - sprite->y;
#if FLIP_Y
    offset_y_gen = (int16_t)(sprite_width)-offset_y_gen - 1;
#endif
#if DOUBLE
    offset_y_gen >>= 1;
#endif
    size_t offset_y = (size_t)offset_y_gen;

#if FLIP_X
    ssize_t x_pos = -8;
#else
    ssize_t x_pos = (ssize_t)(sprite->width);
#endif

#if USE_INLINE_ASM
#if (!DOUBLE) && FLIP_X
    asm volatile inline("ee.zero.q q3");
#else
    asm volatile inline("ee.zero.q q0");
#endif
#endif
#if USE_GCC_SIMD
    static const uint16x8_t zero = VBROADCAST(0);
    uint16x8_t spr_1 = zero;
    uint16x8_t spr_2;
    uint16x8_t spr_col;
#endif
#if VERIFY_INLINE_ASM
#if (!DOUBLE) && FLIP_X
    CHECK_SIMD_Q3(spr_1);
#else
    CHECK_SIMD_Q0(spr_1);
#endif
#endif

    HandleWindowType windows = HANDLE_WINDOW_LOOKUP[sprite->windows];

    ssize_t start_x = (ssize_t)(sprite->x) / 8;
    ssize_t end_x = ((ssize_t)(sprite->x + (int16_t)(sprite_width))) / 8;

    if ((sprite->x & 0x7) == 0)
    {
        start_x -= 1;
        end_x -= 1;
    }

    ssize_t x = end_x;
    do
    {
#if FLIP_X
        x_pos += 8;
#else
        x_pos -= 8;
#endif

#if USE_INLINE_ASM
#if (!DOUBLE) && FLIP_X
        asm volatile inline("mv.qr q1, q3");
#else
        asm volatile inline("mv.qr q1, q0");
#endif
#endif
#if USE_GCC_SIMD
        spr_2 = spr_1;
#endif
#if VERIFY_INLINE_ASM
        CHECK_SIMD_Q1(spr_2);
#endif

#if FLIP_X
        if (x_pos >= (ssize_t)(sprite->width))
#else
        if (x_pos < 0)
#endif
        {
#if USE_INLINE_ASM
            asm volatile inline("ee.zero.q q0");
#endif
#if USE_GCC_SIMD
            spr_1 = zero;
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q0(spr_1);
#endif
        }
        else
        {
            uint16x8_t *spr_1_p = &SASPPU_sprites[((offset_y + (size_t)(sprite->graphics_y)) * (SPR_WIDTH >> 3)) + (((size_t)(x_pos) >> 3) + (size_t)(sprite->graphics_x >> 3))];
#if USE_INLINE_ASM
            asm volatile inline("ld.qr q0, %[spr_1_p], 0" : : [spr_1_p] "r"(spr_1_p));
#endif
#if USE_GCC_SIMD
            spr_1 = *spr_1_p;
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q0(spr_1);
#endif
        }

#if DOUBLE
#if USE_INLINE_ASM
        asm volatile inline("                 \n\t \
            mv.qr q3, q0      \n\t \
            ee.vzip.16 q0, q3 \n\t \
            ");
#endif
#if USE_GCC_SIMD
        uint16x8_t spr_1_high;
        {
            uint16x8_t spr_1_tmp = spr_1;
            spr_1 = SHUFFLE_2(spr_1_tmp, spr_1_tmp, INTERLEAVE_MASK_LOW);
            spr_1_high = SHUFFLE_2(spr_1_tmp, spr_1_tmp, INTERLEAVE_MASK_HIGH);
        }
#endif
#if VERIFY_INLINE_ASM
        CHECK_SIMD_Q0(spr_1);
        CHECK_SIMD_Q3(spr_1_high);
#endif

#if FLIP_X
#if USE_INLINE_ASM
        asm volatile inline("                 \n\t \
            ee.vzip.16 q0, q3 \n\t \
            ee.vzip.16 q3, q0 \n\t \
            ee.vzip.16 q0, q3 \n\t \
            ee.vzip.16 q3, q0 \n\t \
            ");
#endif
#if USE_GCC_SIMD
        {
            uint16x8_t spr_1_tmp = spr_1;
            spr_1 = SHUFFLE_1(spr_1_high, REVERSE_MASK);
            spr_1_high = SHUFFLE_1(spr_1_tmp, REVERSE_MASK);
        }
#endif
#if VERIFY_INLINE_ASM
        CHECK_SIMD_Q0(spr_1);
        CHECK_SIMD_Q3(spr_1_high);
#endif
#endif

        if (x >= 0 && x < 30)
        {
#if USE_INLINE_ASM
            asm volatile inline("                                           \n\t \
                mv.qr q2, q3                                \n\t \
                ee.src.q.ld.ip q7, %[cmath_bit], 0, q2, q1  \n\t \
                " : : [cmath_bit] "r"(&CMATH_BIT));
#endif
#if USE_GCC_SIMD
            spr_col = SHUFFLE_2(spr_1_high, spr_2, VECTOR_SHUFFLES[offset]);
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q2(spr_col);
#endif

#if CMATH
#if USE_INLINE_ASM
            asm volatile inline("ee.orq q2, q2, q7");
#endif
#if USE_GCC_SIMD
            spr_col |= CMATH_BIT;
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q2(spr_col);
#endif
#endif

#if USE_INLINE_ASM
            windows(scanline, x);
#else
            windows(scanline, x, spr_col);
#endif
        }
        x -= 1;

        if (x >= 0 && x < 30)
        {

#if USE_INLINE_ASM
            asm volatile inline("                                           \n\t \
                mv.qr q2, q0                                \n\t \
                ee.src.q.ld.ip q7, %[cmath_bit], 0, q2, q3  \n\t \
                " : : [cmath_bit] "r"(&CMATH_BIT));
#endif
#if USE_GCC_SIMD
            spr_col = SHUFFLE_2(spr_1, spr_1_high, VECTOR_SHUFFLES[offset]);
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q2(spr_col);
#endif

#if CMATH
#if USE_INLINE_ASM
            asm volatile inline("ee.orq q2, q2, q7");
#endif
#if USE_GCC_SIMD
            spr_col |= CMATH_BIT;
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q2(spr_col);
#endif
#endif

#if USE_INLINE_ASM
            windows(scanline, x);
#else
            windows(scanline, x, spr_col);
#endif
        }
        x -= 1;
#else
#if FLIP_X
#if USE_INLINE_ASM
        asm volatile inline("                 \n\t \
            ee.vzip.16 q0, q3 \n\t \
            ee.vzip.16 q3, q0 \n\t \
            ee.vzip.16 q0, q3 \n\t \
            ee.vzip.16 q3, q0 \n\t \
            ");
#endif
#if USE_GCC_SIMD
        spr_1 = SHUFFLE_1(spr_1, REVERSE_MASK);
#endif
#if VERIFY_INLINE_ASM
        CHECK_SIMD_Q3(spr_1);
#endif
#endif

        if (x >= 0 && x < 30)
        {

#if USE_INLINE_ASM
#if FLIP_X
            asm volatile inline("                                           \n\t \
                mv.qr q2, q3                                \n\t \
                ee.src.q.ld.ip q7, %[cmath_bit], 0, q2, q1  \n\t \
                " : : [cmath_bit] "r"(&CMATH_BIT));
#else
            asm volatile inline("                                           \n\t \
                mv.qr q2, q0                                \n\t \
                ee.src.q.ld.ip q7, %[cmath_bit], 0, q2, q1  \n\t \
                " : : [cmath_bit] "r"(&CMATH_BIT));
#endif
#endif
#if USE_GCC_SIMD
            spr_col = SHUFFLE_2(spr_1, spr_2, VECTOR_SHUFFLES[offset]);
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q2(spr_col);
#endif

#if CMATH
#if USE_INLINE_ASM
            asm volatile inline("ee.orq q2, q2, q7");
#endif
#if USE_GCC_SIMD
            spr_col |= CMATH_BIT;
#endif
#if VERIFY_INLINE_ASM
            CHECK_SIMD_Q2(spr_col);
#endif
#endif

#if USE_INLINE_ASM
            windows(scanline, x);
#else
            windows(scanline, x, spr_col);
#endif
        }
        x -= 1;
#endif
    } while (x >= start_x);
}

#undef IDENT
#undef FLIP_X
#undef FLIP_Y
#undef CMATH
#undef DOUBLE
