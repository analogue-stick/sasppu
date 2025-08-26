.ifndef SASPPU_HANDLE_CMATH
.set SASPPU_HANDLE_CMATH, 1

.set CMATH_TARNISH, a15

.macro colour_split_main five, ten, mask_split
  movi a15, 0
  wsr.sar a15
  ee.vldbc.16.ip q2, \five, 0
  ee.vldbc.16.ip q3, \ten, 0
  ee.vldbc.16.ip q4, \mask_split, 0
  ee.vmul.u16 q7, q0, q3 // blue
  ee.vmul.u16 q6, q0, q2 // green
  ee.andq q7, q7, q4
  ee.andq q6, q6, q4
  ee.andq q5, q0, q4
.endm

.macro colour_split_sub
  ee.vmul.u16 q3, q1, q3 // blue
  ee.vmul.u16 q2, q1, q2 // green
  ee.andq q3, q3, q4
  ee.andq q2, q2, q4
  ee.andq q1, q1, q4
.endm

.macro colour_split_both five, ten, mask_split
  colour_split_main \five, \ten, \mask_split
  colour_split_sub
.endm

.macro no_cmath_colour_shift one, eight, nine, mask_low, mask_green
// 0rrrrrgggggbbbbb
// gg0bbbbbrrrrrggg
  ee.vldbc.16.ip q6, \one, 0
  ee.vldbc.16.ip q5, \eight, 0
  ee.vldbc.16.ip q4, \nine, 0
  ee.vldbc.16.ip q2, \mask_low, 0
  ee.vldbc.16.ip q1, \mask_green, 0

  ee.andq q2, q0, q2
  ee.andq q1, q0, q1

  movi a15, 7
  wsr.sar a15
  ee.vmul.u16 q3, q0, q6

  movi a15, 0
  wsr.sar a15
  ee.vmul.u16 q2, q2, q5

  ee.vmul.u16 q1, q1, q4

  ee.orq q0, q1, q2
  ee.orq q0, q0, q3
.endm

.macro recombine_colour one, four
// 0rrrrr0000000000
// 0ggggg0000000000
// 0bbbbb0000000000
// gg0bbbbbrrrrrggg
  ee.vldbc.16.ip q0, \one, 0
  ee.vldbc.16.ip q1, \four, 0

  movi a15, 12 // green shift down 12
  wsr.sar a15
  ee.vmul.u16 q2, q6, q0

  movi a15, 2 // blue shift down 2
  wsr.sar a15
  ee.vmul.u16 q7, q7, q0

  movi a15, 7 // red shift down 7
  wsr.sar a15
  ee.vmul.u16 q5, q5, q0

  movi a15, 0 // green shift up 4
  wsr.sar a15
  ee.vmul.u16 q6, q6, q1

  ee.orq q0, q6, q5
  ee.orq q0, q0, q7
  ee.orq q0, q0, q2
.endm

.macro double_screen colr, colg, colb
  ee.vadds.s16 \colr, \colr, \colr //shl 1
  ee.vadds.s16 \colg, \colg, \colg //shl 1
  ee.vadds.s16 \colb, \colb, \colb //shl 1
.endm

.macro halve_screen colr, colg, colb // assumes cmath_one in q0, 1 in SAR
  ee.vmul.u16 \colr, \colr, q0 //shr 1
  ee.vmul.u16 \colg, \colg, q0 //shr 1
  ee.vmul.u16 \colb, \colb, q0 //shr 1
.endm

.macro add_screens
  ee.vadds.s16 q5, q5, q1
  ee.vadds.s16 q6, q6, q2
  ee.vadds.s16 q7, q7, q3
.endm

.macro sub_screens
  movi a15, 0
  ee.vsubs.s16 q5, q5, q1
  ee.vsubs.s16 q6, q6, q2
  ee.vsubs.s16 q7, q7, q3
  ee.vrelu.s16 q5, a15, a15
  ee.vrelu.s16 q6, a15, a15
  ee.vrelu.s16 q7, a15, a15
.endm

.macro handle_fade_only fade, one, four, five, ten, mask_split
  colour_split_main \five, \ten, \mask_split
  movi a15, 8
  wsr.sar a15
  ee.vldbc.16.ip q3, \fade, 0
  ee.vmul.u16 q5, q5, q3
  ee.vmul.u16 q6, q6, q3
  ee.vmul.u16 q7, q7, q3
  ee.andq q5, q5, q4
  ee.andq q6, q6, q4
  ee.andq q7, q7, q4
  recombine_colour \one, \four
.endm

.macro handle_cmath_body sub_ss, add_ss, ss_double, ss_half, ms_double, ms_half, one
  ee.vldbc.16.ip q0, \one, 0
.if \ms_half || \ss_half // preload the halving constants
  movi a15, 1
  wsr.sar a15
.endif

.if \ms_double
  double_screen q5, q6, q7
.endif

.if \ms_half
  halve_screen q5, q6, q7
.endif

.if \ss_double
  double_screen q1, q2, q3
.endif

.if \ss_half
  halve_screen q1, q2, q3
.endif

.if \add_ss
  add_screens
.endif

.if \sub_ss
  sub_screens
.endif
.endm

.macro handle_cmath_only sub_ss, add_ss, ss_double, ss_half, ms_double, ms_half, five, ten, mask_split, one, four, spill_area
  ee.zero.q q7
  ee.vcmp.lt.s16 q4, q0, q7 // less than zero means bit 15 set
  ee.vcmp.eq.s16 q5, q1, q7 // subscreen not zero
  ee.notq q5, q5
  ee.andq q4, q4, q5
  st.qr q4, \spill_area, 0 // Ive run out of registers, so this is going to be sent to the stack for its crimes.
  colour_split_both \five, \ten, \mask_split
  st.qr q5, \spill_area, 16 // main_r_bak
  st.qr q6, \spill_area, 32 // main_g_bak
  st.qr q7, \spill_area, 48 // main_b_bak

  handle_cmath_body \sub_ss, \add_ss, \ss_double, \ss_half, \ms_double, \ms_half, \one

  ld.qr q3, \spill_area, 0 // use_cmath
  ld.qr q0, \spill_area, 16 // main_r_bak
  ld.qr q1, \spill_area, 32 // main_g_bak
  ld.qr q2, \spill_area, 48 // main_b_bak
  ee.andq q5, q5, q3
  ee.andq q6, q6, q3
  ee.andq q7, q7, q3
  ee.notq q3, q3
  ee.andq q0, q0, q3
  ee.andq q1, q1, q3
  ee.andq q2, q2, q3
  ee.orq q5, q5, q0
  ee.orq q6, q6, q1
  ee.orq q7, q7, q2

  ee.andq q5, q5, q4
  ee.andq q6, q6, q4
  ee.andq q7, q7, q4
  
  recombine_colour \one, \four
.endm

.macro handle_cmath_and_fade sub_ss, add_ss, ss_double, ss_half, ms_double, ms_half, five, ten, mask_split, one, four, fade, spill_area
  ee.zero.q q7
  ee.vcmp.lt.s16 q4, q0, q7 // less than zero means bit 15 set
  ee.vcmp.eq.s16 q5, q1, q7 // subscreen not zero
  ee.notq q5, q5
  ee.andq q4, q4, q5
  st.qr q4, \spill_area, 0 // Ive run out of registers, so this is going to be sent to the stack for its crimes.
  colour_split_both \five, \ten, \mask_split
  st.qr q5, \spill_area, 16 // main_r_bak
  st.qr q6, \spill_area, 32 // main_g_bak
  st.qr q7, \spill_area, 48 // main_b_bak

  handle_cmath_body \sub_ss, \add_ss, \ss_double, \ss_half, \ms_double, \ms_half, \one

  ld.qr q3, \spill_area, 0 // use_cmath
  ld.qr q0, \spill_area, 16 // main_r_bak
  ld.qr q1, \spill_area, 32 // main_g_bak
  ld.qr q2, \spill_area, 48 // main_b_bak
  ee.andq q5, q5, q3
  ee.andq q6, q6, q3
  ee.andq q7, q7, q3
  ee.notq q3, q3
  ee.andq q0, q0, q3
  ee.andq q1, q1, q3
  ee.andq q2, q2, q3
  ee.orq q5, q5, q0
  ee.orq q6, q6, q1
  ee.orq q7, q7, q2
  
  movi a15, 8
  wsr.sar a15
  ee.vldbc.16.ip q3, \fade, 0
  ee.vmul.u16 q5, q5, q3
  ee.vmul.u16 q6, q6, q3
  ee.vmul.u16 q7, q7, q3
  ee.andq q5, q5, q4
  ee.andq q6, q6, q4
  ee.andq q7, q7, q4

  recombine_colour \one, \four
.endm

.macro handle_cmath cmath_enable, fade_enable, sub_ss, add_ss, ss_double, ss_half, ms_double, ms_half, five, ten, mask_split, one, four, eight, nine, fade, mask_low, mask_green, spill_area
.if \fade_enable && !\cmath_enable
  handle_fade_only \fade, \one, \four, \five, \ten, \mask_split
  .exitm
.endif
.if !\fade_enable && \cmath_enable
  handle_cmath_only \sub_ss, \add_ss, \ss_double, \ss_half, \ms_double, \ms_half, \five, \ten, \mask_split, \one, \four, \spill_area
  .exitm
.endif
.if \fade_enable && \cmath_enable
  handle_cmath_and_fade \sub_ss, \add_ss, \ss_double, \ss_half, \ms_double, \ms_half, \five, \ten, \mask_split, \one, \four, \fade, \spill_area
  .exitm
.endif
.if !\fade_enable && !\cmath_enable
  no_cmath_colour_shift \one, \eight, \nine, \mask_low, \mask_green
  .exitm
.endif
.endm

.endif // SASPPU_HANDLE_CMATH
