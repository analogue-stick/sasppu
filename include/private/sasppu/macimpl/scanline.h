#include "sasppu/sasppu.h"
#include "sasppu/internal.h"

// SASPPU_HANDLE_SCANLINE(IDENT, BG0_ENABLE, BG1_ENABLE, SPR0_ENABLE, SPR1_ENABLE, CMATH_ENABLE, BGCOL_ENABLE)
#ifndef IDENT
#define IDENT handle_scanline_1_1_1_1_1_1
#endif

#ifndef BG0_ENABLE
#define BG0_ENABLE 1
#endif
#ifndef BG1_ENABLE
#define BG1_ENABLE 1
#endif
#ifndef SPR0_ENABLE
#define SPR0_ENABLE 1
#endif
#ifndef SPR1_ENABLE
#define SPR1_ENABLE 1
#endif
#ifndef CMATH_ENABLE
#define CMATH_ENABLE 1
#endif
#ifndef BGCOL_ENABLE
#define BGCOL_ENABLE 1
#endif

static void IDENT(uint16x8_t *const scanline, int16_t y)
{
    mask16x8_t *window_index = &SASPPU_window_cache[((240 / 8) * 2) - 1];
#if USE_INLINE_ASM
    asm volatile inline("                                                               \n\t \
                 ld.qr q0, %[vector_increments], 0 /* x_window */                \n\t \
                 ee.vldbc.16.ip q4, %[main_state], 2 /* load window_1_left */    \n\t \
                 ee.vldbc.16.ip q5, %[main_state], 2 /* load window_1_right */   \n\t \
                 ee.vldbc.16.ip q6, %[main_state], 2 /* load window_2_left  */   \n\t \
                 ee.vldbc.16.ip q7, %[main_state], 2 /* load window_2_right */  \n\t \
                 "
                        :
                        : [vector_increments] "r"(&VECTOR_INCREMENTS_END),
                          [main_state] "r"(&SASPPU_main_state.window_1_left));

    do
    {
        asm volatile inline("                                                               \n\t \
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
    do
    {
        /* Calculate window 2 */
        *(window_index--) = (x_window >= window_2_left) & (x_window <= window_2_right);

        /* Calculate window 1 */
        *(window_index--) = (x_window >= window_1_left) & (x_window <= window_1_right);

        /* Jump down 8 pixels */
        x_window -= 8;
    } while (window_index > SASPPU_window_cache);
#endif
#if VERIFY_INLINE_ASM
    {
        size_t x = 0;
        do
        {
            if ((x >= SASPPU_main_state.window_1_left) & (x <= SASPPU_main_state.window_1_right))
            {
                assert(SASPPU_window_cache[((x >> 3) * 2) + 0][x & 0x7] == 0xFFFF);
            }
            else
            {
                assert(SASPPU_window_cache[((x >> 3) * 2) + 0][x & 0x7] == 0);
            }
            if ((x >= SASPPU_main_state.window_2_left) & (x <= SASPPU_main_state.window_2_right))
            {
                assert(SASPPU_window_cache[((x >> 3) * 2) + 1][x & 0x7] == 0xFFFF);
            }
            else
            {
                assert(SASPPU_window_cache[((x >> 3) * 2) + 1][x & 0x7] == 0);
            }
        } while ((++x) < 240);
    }
#endif

#if BGCOL_ENABLE
    HandleWindowType main_win = HANDLE_WINDOW_LOOKUP[SASPPU_main_state.bgcol_windows & 0x0F];
    HandleWindowType sub_win = HANDLE_WINDOW_LOOKUP[SASPPU_main_state.bgcol_windows & 0xF0];
#if USE_INLINE_ASM
    asm volatile inline("ee.zero.q q7");
#endif
#endif

#if USE_INLINE_ASM
    asm volatile inline("                                                                \n\t \
        ee.vldbc.16.ip q0, %[main_state], 0 /* load mainscreen_colour */ \n\t \
        ee.vldbc.16.ip q1, %[main_state], 2 /* load subscreen_colour */  \n\t \
        " : : [main_state] "r"(&SASPPU_main_state));
#else
#if BGCOL_ENABLE
    static const uint16x8_t zero = VBROADCAST(0);
#endif
    uint16x8_t vsubcol = VBROADCAST(SASPPU_main_state.subscreen_colour);
    uint16x8_t vmaincol = VBROADCAST(SASPPU_main_state.mainscreen_colour);
#endif

    uint16x8_t *maincol = &scanline[(240 / 8) - 1];
    uint16x8_t *subcol = &SASPPU_subscreen_scanline[(240 / 8) - 1];

    ssize_t x = (240 / 8) - 1;
    do
    {
#if BGCOL_ENABLE
#if USE_INLINE_ASM
        asm volatile inline("                                   \n\t \
            ee.vst.128.ip q7, %[subcol], -16    \n\t \
            mv.qr q2, q1                        \n\t \
            " : [subcol] "+r"(subcol));
        sub_win(scanline, x);
        asm volatile inline("                                   \n\t \
            ee.vst.128.ip q7, %[maincol], -16   \n\t \
            mv.qr q2, q0                        \n\t \
            " : [maincol] "+r"(maincol));
        main_win(scanline, x);
#else
        *(maincol--) = zero;
        sub_win(scanline, x, vsubcol);
        *(subcol--) = zero;
        main_win(scanline, x, vmaincol);
#endif
#else
#if USE_INLINE_ASM
        asm volatile inline("                                   \n\t \
            ee.vst.128.ip q0, %[maincol], -16   \n\t \
            ee.vst.128.ip q1, %[subcol], -16    \n\t \
            " : [subcol] "+r"(subcol),
                            [maincol] "+r"(maincol) :);
#else
        *(maincol--) = vmaincol;
        *(subcol--) = vsubcol;
#endif
#endif
    } while ((--x) >= 0);

#if BG0_ENABLE
    handle_bg0(scanline, y);
#endif

#if SPR0_ENABLE
    {
        Sprite *const *spr = &SASPPU_sprite_cache[0][SPRITE_CACHE - 1];
        do
        {
            Sprite *const sprite = *spr;
            if (!sprite)
            {
                continue;
            }
            HANDLE_SPRITE_LOOKUP[sprite->flags >> 2](scanline, y, sprite);
        } while ((--spr) >= &SASPPU_sprite_cache[0][0]);
    }
#endif

#if BG1_ENABLE
    handle_bg1(scanline, y);
#endif

#if SPR1_ENABLE
    {
        Sprite *const *spr = &SASPPU_sprite_cache[1][SPRITE_CACHE - 1];
        do
        {
            Sprite *const sprite = *spr;
            if (!sprite)
            {
                continue;
            }
            HANDLE_SPRITE_LOOKUP[sprite->flags >> 2](scanline, y, sprite);
        } while ((--spr) >= &SASPPU_sprite_cache[1][0]);
    }
#endif

#if CMATH_ENABLE
    return HANDLE_CMATH_LOOKUP[SASPPU_cmath_state.flags](scanline);
#else
    return HANDLE_CMATH_LOOKUP[0](scanline);
#endif
}

#undef IDENT
#undef BG0_ENABLE
#undef BG1_ENABLE
#undef SPR0_ENABLE
#undef SPR1_ENABLE
#undef CMATH_ENABLE
#undef BGCOL_ENABLE
