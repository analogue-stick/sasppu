#include "sasppu/internal.h"
#include "sasppu/sasppu.h"

// SASPPU_HANDLE_WINDOW(IDENT, LOGIC)
#ifndef IDENT
#define IDENT handle_window_0
#endif

#ifndef LOGIC_MAIN
#define LOGIC_MAIN 0
#endif

#ifndef LOGIC_SUB
#define LOGIC_SUB 0
#endif

#if USE_INLINE_ASM
static void IDENT(uint16x8_t *const scanline, uint16_t x) {
  asm volatile(
      "                                                                                   \n\t \
        .include \"sasppu/asm/window.i\"                                                 \n\t \
        window_macro %[logic_main], %[logic_sub], %[window_index], %[maincol], %[subcol]    \n\t \
        "
      :
      : [maincol] "r"(&scanline[x]), [subcol] "r"(&SASPPU_sub_screen[x]),
        [window_index] "r"(&SASPPU_window_cache[x * 2]),
        [logic_main] "i"(LOGIC_MAIN), [logic_sub] "i"(LOGIC_SUB));
}
#else
static void IDENT(uint16x8_t *const scanline, uint16_t x, uint16x8_t col) {
  static const uint16x8_t zero = VBROADCAST(0);
#define window_logic_window(enable, w1, w2) ((enable) ? ((w1) & (w2)) : zero)
#define window_macro(enable_a, enable_b, enable_c, enable_d, w1, w2)           \
  (window_logic_window((enable_a), (w1), ~(w2)) |                              \
   window_logic_window((enable_b), ~(w1), (w2)) |                              \
   window_logic_window((enable_c), (w1), (w2)) |                               \
   window_logic_window((enable_d), ~(w1), ~(w2)))

#if LOGIC_MAIN == 0
#define get_window_main(w1, w2) window_macro(0, 0, 0, 0, (w1), (w2))
#elif LOGIC_MAIN == 1
#define get_window_main(w1, w2) window_macro(1, 0, 0, 0, (w1), (w2))
#elif LOGIC_MAIN == 2
#define get_window_main(w1, w2) window_macro(0, 1, 0, 0, (w1), (w2))
#elif LOGIC_MAIN == 3
#define get_window_main(w1, w2) window_macro(1, 1, 0, 0, (w1), (w2))
#elif LOGIC_MAIN == 4
#define get_window_main(w1, w2) window_macro(0, 0, 1, 0, (w1), (w2))
#elif LOGIC_MAIN == 5
#define get_window_main(w1, w2) window_macro(1, 0, 1, 0, (w1), (w2))
#elif LOGIC_MAIN == 6
#define get_window_main(w1, w2) window_macro(0, 1, 1, 0, (w1), (w2))
#elif LOGIC_MAIN == 7
#define get_window_main(w1, w2) window_macro(1, 1, 1, 0, (w1), (w2))
#elif LOGIC_MAIN == 8
#define get_window_main(w1, w2) window_macro(0, 0, 0, 1, (w1), (w2))
#elif LOGIC_MAIN == 9
#define get_window_main(w1, w2) window_macro(1, 0, 0, 1, (w1), (w2))
#elif LOGIC_MAIN == 10
#define get_window_main(w1, w2) window_macro(0, 1, 0, 1, (w1), (w2))
#elif LOGIC_MAIN == 11
#define get_window_main(w1, w2) window_macro(1, 1, 0, 1, (w1), (w2))
#elif LOGIC_MAIN == 12
#define get_window_main(w1, w2) window_macro(0, 0, 1, 1, (w1), (w2))
#elif LOGIC_MAIN == 13
#define get_window_main(w1, w2) window_macro(1, 0, 1, 1, (w1), (w2))
#elif LOGIC_MAIN == 14
#define get_window_main(w1, w2) window_macro(0, 1, 1, 1, (w1), (w2))
#elif LOGIC_MAIN == 15
#define get_window_main(w1, w2) window_macro(1, 1, 1, 1, (w1), (w2))
#endif

#if LOGIC_SUB == 0
#define get_window_sub(w1, w2) window_macro(0, 0, 0, 0, (w1), (w2))
#elif LOGIC_SUB == 1
#define get_window_sub(w1, w2) window_macro(1, 0, 0, 0, (w1), (w2))
#elif LOGIC_SUB == 2
#define get_window_sub(w1, w2) window_macro(0, 1, 0, 0, (w1), (w2))
#elif LOGIC_SUB == 3
#define get_window_sub(w1, w2) window_macro(1, 1, 0, 0, (w1), (w2))
#elif LOGIC_SUB == 4
#define get_window_sub(w1, w2) window_macro(0, 0, 1, 0, (w1), (w2))
#elif LOGIC_SUB == 5
#define get_window_sub(w1, w2) window_macro(1, 0, 1, 0, (w1), (w2))
#elif LOGIC_SUB == 6
#define get_window_sub(w1, w2) window_macro(0, 1, 1, 0, (w1), (w2))
#elif LOGIC_SUB == 7
#define get_window_sub(w1, w2) window_macro(1, 1, 1, 0, (w1), (w2))
#elif LOGIC_SUB == 8
#define get_window_sub(w1, w2) window_macro(0, 0, 0, 1, (w1), (w2))
#elif LOGIC_SUB == 9
#define get_window_sub(w1, w2) window_macro(1, 0, 0, 1, (w1), (w2))
#elif LOGIC_SUB == 10
#define get_window_sub(w1, w2) window_macro(0, 1, 0, 1, (w1), (w2))
#elif LOGIC_SUB == 11
#define get_window_sub(w1, w2) window_macro(1, 1, 0, 1, (w1), (w2))
#elif LOGIC_SUB == 12
#define get_window_sub(w1, w2) window_macro(0, 0, 1, 1, (w1), (w2))
#elif LOGIC_SUB == 13
#define get_window_sub(w1, w2) window_macro(1, 0, 1, 1, (w1), (w2))
#elif LOGIC_SUB == 14
#define get_window_sub(w1, w2) window_macro(0, 1, 1, 1, (w1), (w2))
#elif LOGIC_SUB == 15
#define get_window_sub(w1, w2) window_macro(1, 1, 1, 1, (w1), (w2))
#endif

  mask16x8_t w1 = SASPPU_window_cache[(x * 2) + 0];
  mask16x8_t w2 = SASPPU_window_cache[(x * 2) + 1];
  mask16x8_t main_window = get_window_main(w1, w2);
  main_window = main_window & (col != (short)0);
  mask16x8_t sub_window = get_window_sub(w1, w2);
  sub_window = sub_window & (col != (short)0);

  scanline[x] &= ~main_window;
  scanline[x] |= col & main_window;

  SASPPU_sub_screen[x] &= ~sub_window;
  SASPPU_sub_screen[x] |= col & sub_window;

#undef window_logic_window
#undef window_macro
#undef get_window_main
#undef get_window_sub
}
#endif

#undef IDENT
#undef LOGIC_MAIN
#undef LOGIC_SUB
