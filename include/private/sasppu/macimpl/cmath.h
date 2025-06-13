#include "sasppu/sasppu.h"
#include "sasppu/internal.h"

// SASPPU_HANDLE_CMATH(IDENT, HALF_MAIN_SCREEN, DOUBLE_MAIN_SCREEN, HALF_SUB_SCREEN, DOUBLE_SUB_SCREEN, ADD_SUB_SCREEN, SUB_SUB_SCREEN, FADE_ENABLE, CMATH_ENABLE)
#ifndef IDENT
#define IDENT handle_cmath_0_0_0_0_0_0_0_0
#endif

#ifndef HALF_MAIN_SCREEN
#define HALF_MAIN_SCREEN 1
#endif
#ifndef DOUBLE_MAIN_SCREEN
#define DOUBLE_MAIN_SCREEN 1
#endif
#ifndef HALF_SUB_SCREEN
#define HALF_SUB_SCREEN 1
#endif
#ifndef DOUBLE_SUB_SCREEN
#define DOUBLE_SUB_SCREEN 1
#endif
#ifndef ADD_SUB_SCREEN
#define ADD_SUB_SCREEN 1
#endif
#ifndef SUB_SUB_SCREEN
#define SUB_SUB_SCREEN 1
#endif
#ifndef FADE_ENABLE
#define FADE_ENABLE 1
#endif
#ifndef CMATH_ENABLE
#define CMATH_ENABLE 1
#endif

#ifndef SASPPU_MACIMPL_CMATH_H_
#define SASPPU_MACIMPL_CMATH_H_

#define CMATH_HELPER_SPLIT_COL(out_r, out_g, out_b, col_in)            \
    int16x8_t out_r;                                                   \
    int16x8_t out_g;                                                   \
    int16x8_t out_b;                                                   \
    {                                                                  \
        static const uint16x8_t mask = VBROADCAST(0b0111110000000000); \
        __typeof__(col_in) _col_in = (col_in);                         \
        out_r = (int16x8_t)((_col_in << 0) & mask);                    \
        out_g = (int16x8_t)((_col_in << 5) & mask);                    \
        out_b = (int16x8_t)((_col_in << 10) & mask);                   \
    }

#define CMATH_HELPER_RECOMBINE_COL(in_r, in_g, in_b, col_out)                       \
    {                                                                               \
        static const uint16x8_t mask = VBROADCAST(0b0111110000000000);              \
        uint16x8_t _in_r = (uint16x8_t)(in_r);                                      \
        uint16x8_t _in_g = (uint16x8_t)(in_g);                                      \
        uint16x8_t _in_b = (uint16x8_t)(in_b);                                      \
        _in_r &= mask;                                                              \
        _in_g &= mask;                                                              \
        _in_b &= mask;                                                              \
        uint16x8_t col_out_unflipped = (_in_r << 1) | (_in_g >> 4) | (_in_b >> 10); \
        col_out = (col_out_unflipped << 8) | (col_out_unflipped >> 8);              \
    }

#define CMATH_HELPER_ADD_SINGLE(out, sub)                   \
    {                                                       \
        out += sub;                                         \
        static const int16x8_t zero = VBROADCAST(0);        \
        static const int16x8_t max = VBROADCAST(INT16_MAX); \
        mask16x8_t choose = (out < zero);                   \
        out = (max & choose) | (out & (~choose));           \
    }

#define CMATH_HELPER_SUB_SINGLE(out, sub)            \
    {                                                \
        out -= sub;                                  \
        static const int16x8_t zero = VBROADCAST(0); \
        mask16x8_t choose = (out < zero);            \
        out = (zero & choose) | (out & (~choose));   \
    }

#define CMATH_HELPER_DOUBLE_SCREEN(out_r, out_g, out_b) \
    {                                                   \
        CMATH_HELPER_ADD_SINGLE(out_r, out_r);          \
        CMATH_HELPER_ADD_SINGLE(out_g, out_g);          \
        CMATH_HELPER_ADD_SINGLE(out_b, out_b);          \
    }

#define CMATH_HELPER_HALVE_SCREEN(out_r, out_g, out_b) \
    {                                                  \
        out_r >>= 1;                                   \
        out_g >>= 1;                                   \
        out_b >>= 1;                                   \
    }

#define CMATH_HELPER_ADD_SCREEN(out_r, out_g, out_b, sub_r, sub_g, sub_b) \
    {                                                                     \
        CMATH_HELPER_ADD_SINGLE(out_r, sub_r);                            \
        CMATH_HELPER_ADD_SINGLE(out_g, sub_g);                            \
        CMATH_HELPER_ADD_SINGLE(out_b, sub_b);                            \
    }

#define CMATH_HELPER_SUB_SCREEN(out_r, out_g, out_b, sub_r, sub_g, sub_b) \
    {                                                                     \
        CMATH_HELPER_SUB_SINGLE(out_r, sub_r);                            \
        CMATH_HELPER_SUB_SINGLE(out_g, sub_g);                            \
        CMATH_HELPER_SUB_SINGLE(out_b, sub_b);                            \
    }

#define CMATH_HELPER_JUST_SHIFT(main_col)                                                                           \
    {                                                                                                               \
        static const int16x8_t mask_left = VBROADCAST(0b0111111111100000);                                          \
        static const int16x8_t mask_right = VBROADCAST(0b0111111111100000);                                         \
        main_col = ((main_col & u16x8::splat(0b0111111111100000)) << 1) | (main_col[x] & u16x8::splat(0b00011111)); \
    }

#endif // SASPPU_MACIMPL_CMATH_H_

static void IDENT(uint16x8_t *const scanline)
{
#if USE_INLINE_ASM
    asm volatile inline(".include \"sasppu/asm/cmath.i\"");
#endif

    uint16x8_t *maincol = &scanline[(240 / 8) - 1];
#if CMATH_ENABLE
    uint16x8_t *subcol = &SASPPU_subscreen_scanline[(240 / 8) - 1];
#endif

    ssize_t x = (240 / 8) - 1;
    do
    {
#if USE_INLINE_ASM
#if CMATH_ENABLE && FADE_ENABLE
        uint16x8_t spill_area[4];
        asm volatile inline("                                   \n\t \
            ld.qr q0, %[maincol], 0             \n\t \
            ee.vld.128.ip q1, %[subcol], -16    \n\t \
            " : [subcol] "+r"(subcol) : [maincol] "r"(maincol));

        asm volatile("handle_cmath_and_fade %[sub_ss], %[add_ss], %[ss_double], %[ss_half], %[ms_double], %[ms_half], %[five], %[ten], %[mask_split], %[one], %[four], %[fade], %[spill_area]" : :
                     [sub_ss] "i"(SUB_SUB_SCREEN),
                     [add_ss] "i"(ADD_SUB_SCREEN),
                     [ss_double] "i"(DOUBLE_SUB_SCREEN),
                     [ss_half] "i"(HALF_SUB_SCREEN),
                     [ms_double] "i"(DOUBLE_MAIN_SCREEN),
                     [ms_half] "i"(HALF_MAIN_SCREEN),
                     [five] "r"(&CMATH_TWO_FIVE),
                     [ten] "r"(&CMATH_TWO_TEN),
                     [mask_split] "r"(&CMATH_MASK_SPLIT),
                     [one] "r"(&CMATH_ONE),
                     [four] "r"(&CMATH_TWO_FOUR),
                     [fade] "r"(&SASPPU_cmath_state.screen_fade),
                     [spill_area] "r"(&spill_area) : "a15");
#elif CMATH_ENABLE && !FADE_ENABLE
        uint16x8_t spill_area[4];
        asm volatile inline("                                   \n\t \
            ld.qr q0, %[maincol], 0             \n\t \
            ee.vld.128.ip q1, %[subcol], -16    \n\t \
            " : [subcol] "+r"(subcol) : [maincol] "r"(maincol));

        asm volatile("handle_cmath_only %[sub_ss], %[add_ss], %[ss_double], %[ss_half], %[ms_double], %[ms_half], %[five], %[ten], %[mask_split], %[one], %[four], %[spill_area]" : :
                     [sub_ss] "i"(SUB_SUB_SCREEN),
                     [add_ss] "i"(ADD_SUB_SCREEN),
                     [ss_double] "i"(DOUBLE_SUB_SCREEN),
                     [ss_half] "i"(HALF_SUB_SCREEN),
                     [ms_double] "i"(DOUBLE_MAIN_SCREEN),
                     [ms_half] "i"(HALF_MAIN_SCREEN),
                     [five] "r"(&CMATH_TWO_FIVE),
                     [ten] "r"(&CMATH_TWO_TEN),
                     [mask_split] "r"(&CMATH_MASK_SPLIT),
                     [one] "r"(&CMATH_ONE),
                     [four] "r"(&CMATH_TWO_FOUR),
                     [spill_area] "r"(&spill_area) : "a15");
#elif !CMATH_ENABLE && FADE_ENABLE
        asm volatile inline("                       \n\t \
            ld.qr q0, %[maincol], 0 \n\t \
            " : : [maincol] "r"(maincol));

        asm volatile("handle_fade_only %[fade], %[one], %[four], %[five], %[ten], %[mask_split]" : :
                     [fade] "r"(&SASPPU_cmath_state.screen_fade),
                     [one] "r"(&CMATH_ONE),
                     [four] "r"(&CMATH_TWO_FOUR),
                     [five] "r"(&CMATH_TWO_FIVE),
                     [ten] "r"(&CMATH_TWO_TEN),
                     [mask_split] "r"(&CMATH_MASK_SPLIT) : "a15");
#elif !CMATH_ENABLE && !FADE_ENABLE
        asm volatile inline("                       \n\t \
            ld.qr q0, %[maincol], 0 \n\t \
            " : : [maincol] "r"(maincol));

        asm volatile("no_cmath_colour_shift %[one], %[eight], %[nine], %[mask_low], %[mask_green]" : :
                     [one] "r"(&CMATH_ONE),
                     [eight] "r"(&CMATH_TWO_EIGHT),
                     [nine] "r"(&CMATH_TWO_NINE),
                     [mask_low] "r"(&CMATH_MASK_LOW),
                     [mask_green] "r"(&CMATH_MASK_GREEN) : "a15");
#endif
        asm volatile inline("ee.vst.128.ip q0, %[maincol], -16" : [maincol] "+r"(maincol));
#else
        CMATH_HELPER_SPLIT_COL(main_r, main_g, main_b, *maincol);
#if CMATH_ENABLE
        static const uint16x8_t cmath_bit = VBROADCAST(0x8000);
        mask16x8_t use_cmath = *maincol >= cmath_bit;
        CMATH_HELPER_SPLIT_COL(sub_r, sub_g, sub_b, *(subcol--));

        int16x8_t main_r_bak = main_r;
        int16x8_t main_g_bak = main_g;
        int16x8_t main_b_bak = main_b;

#if DOUBLE_MAIN_SCREEN
        CMATH_HELPER_DOUBLE_SCREEN(main_r, main_g, main_b);
#endif
#if HALF_MAIN_SCREEN
        CMATH_HELPER_HALVE_SCREEN(main_r, main_g, main_b);
#endif
#if DOUBLE_SUB_SCREEN
        CMATH_HELPER_DOUBLE_SCREEN(sub_r, sub_g, sub_b);
#endif
#if HALF_SUB_SCREEN
        CMATH_HELPER_HALVE_SCREEN(sub_r, sub_g, sub_b);
#endif
#if ADD_SUB_SCREEN
        CMATH_HELPER_ADD_SCREEN(main_r, main_g, main_b, sub_r, sub_g, sub_b);
#endif
#if SUB_SUB_SCREEN
        CMATH_HELPER_SUB_SCREEN(main_r, main_g, main_b, sub_r, sub_g, sub_b);
#endif

        main_r = (main_r & use_cmath) | (main_r_bak & (~use_cmath));
        main_g = (main_g & use_cmath) | (main_g_bak & (~use_cmath));
        main_b = (main_b & use_cmath) | (main_b_bak & (~use_cmath));
#endif
#if FADE_ENABLE
        int16x8_t fade = VBROADCAST((int16_t)(SASPPU_cmath_state.screen_fade));
        main_r = (main_r >> 8) * fade;
        main_g = (main_g >> 8) * fade;
        main_b = (main_b >> 8) * fade;
#endif
        CMATH_HELPER_RECOMBINE_COL(main_r, main_g, main_b, *(maincol--));
#endif
    } while ((--x) >= 0);
}

#undef IDENT
#undef HALF_MAIN_SCREEN
#undef DOUBLE_MAIN_SCREEN
#undef HALF_SUB_SCREEN
#undef DOUBLE_SUB_SCREEN
#undef ADD_SUB_SCREEN
#undef SUB_SUB_SCREEN
#undef FADE_ENABLE
#undef CMATH_ENABLE
