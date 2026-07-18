/**
 * @file help.h
 * @author john hunter <moliveofscratch@gmail.com>
 * @brief Additional helper functions and macros for SASPPU.
 * @version 0.1
 * @date 2025-04-05
 *
 * @copyright Copyright (c) 2025
 *
 */

#ifndef SASPPU_HELP_H_
#define SASPPU_HELP_H_

#include "sasppu/sasppu.h"
#include "stddef.h"
#include "stdint.h"

#define SASPPU_CMATH(col) ((col) | 0x8000)

#define SASPPU_RGB555(r, g, b)                                                 \
  ((((r) & 0x1F) << 10) | (((g) & 0x1F) << 5) | (((b) & 0x1F) << 0))
#define SASPPU_RGB555_CMATH(r, g, b)                                           \
  (SASPPU_CMATH(SASPPU_RGB555((r), (g), (b))))

#define SASPPU_RGB888(r, g, b) SASPPU_RGB555(((r) >> 3), ((g) >> 3), ((b) >> 3))
#define SASPPU_RGB888_CMATH(r, g, b)                                           \
  (SASPPU_CMATH(SASPPU_RGB888((r), (g), (b))))

#define SASPPU_GREY555(g)                                                      \
  ({                                                                           \
    __typeof__(g) _g = (g);                                                    \
    SASPPU_RGB555(_g, _g, _g);                                                 \
  })
#define SASPPU_GREY555_CMATH(g) (SASPPU_CMATH(SASPPU_GREY555((g))))

#define SASPPU_GREY888(g) (SASPPU_GREY555((g) >> 3))
#define SASPPU_GREY888_CMATH(g) (SASPPU_CMATH(SASPPU_GREY888((g))))

#define SASPPU_MUL_CHANNEL(col, mul) ((((uint32_t)((col))) * (mul)) / 256)

#define SASPPU_MUL_RGB555(r, g, b, mul)                                        \
  ({                                                                           \
    __typeof__(mul) _mul = (mul);                                              \
    SASPPU_RGB555(SASPPU_MUL_CHANNEL((r), (_mul)),                             \
                  SASPPU_MUL_CHANNEL((g), (_mul)),                             \
                  SASPPU_MUL_CHANNEL((b), (_mul)));                            \
  })

#define SASPPU_R_CHANNEL(col) (((col) >> 10) & 0x1F)
#define SASPPU_G_CHANNEL(col) (((col) >> 05) & 0x1F)
#define SASPPU_B_CHANNEL(col) (((col) >> 00) & 0x1F)
#define SASPPU_CMATH_CHANNEL(col) (((col) >> 15) & 0x1)

#define SASPPU_MUL_COL(col, mul)                                               \
  ({                                                                           \
    __typeof__(mul) _mul = (mul);                                              \
    __typeof__(col) _col = (col);                                              \
    SASPPU_RGB555(SASPPU_MUL_CHANNEL((SASPPU_R_CHANNEL((_col))), (_mul)),      \
                  SASPPU_MUL_CHANNEL((SASPPU_G_CHANNEL((_col))), (_mul)),      \
                  SASPPU_MUL_CHANNEL((SASPPU_B_CHANNEL((_col))), (_mul)));     \
  })

#define SASPPU_TRANSPARENT_BLACK (SASPPU_RGB555(0, 0, 0))
#define SASPPU_OPAQUE_BLACK (SASPPU_RGB555(1, 0, 0))
#define SASPPU_RED (SASPPU_RGB555(31, 0, 0))
#define SASPPU_GREEN (SASPPU_RGB555(0, 31, 0))
#define SASPPU_BLUE (SASPPU_RGB555(0, 0, 31))
#define SASPPU_WHITE (SASPPU_RGB555(31, 31, 31))

typedef enum {
  SASPPU_IC_Success = 0,
  SASPPU_IC_TooWide,
  SASPPU_IC_TooTall,
  SASPPU_IC_InvalidBitdepth,
} SASPPUImageCode;

SASPPUImageCode SASPPU_copy_sprite(size_t dst_x, size_t dst_y, size_t width,
                                   size_t height, size_t src_x, size_t src_y,
                                   bool double_size);
SASPPUImageCode SASPPU_copy_sprite_transparent(size_t dst_x, size_t dst_y,
                                               size_t width, size_t height,
                                               size_t src_x, size_t src_y,
                                               bool double_size);
SASPPUImageCode SASPPU_blit_sprite(size_t x, size_t y, size_t width,
                                   size_t height, bool double_size,
                                   const uint16_t *data);
SASPPUImageCode SASPPU_blit_sprite_transparent(size_t x, size_t y, size_t width,
                                               size_t height, bool double_size,
                                               const uint16_t *data);
SASPPUImageCode SASPPU_paletted_sprite(size_t x, size_t y, size_t width,
                                       size_t height, bool double_size,
                                       const uint8_t *data,
                                       const uint16_t *const palette,
                                       size_t bitdepth);
SASPPUImageCode SASPPU_paletted_sprite_transparent(
    size_t x, size_t y, size_t width, size_t height, bool double_size,
    const uint8_t *data, const uint16_t *const palette, size_t bitdepth);
SASPPUImageCode SASPPU_compressed_sprite(size_t x, size_t y, size_t width,
                                         size_t height, bool double_size,
                                         const uint8_t *data,
                                         const uint16_t *const palette,
                                         size_t bitdepth);
SASPPUImageCode SASPPU_compressed_sprite_transparent(
    size_t x, size_t y, size_t width, size_t height, bool double_size,
    const uint8_t *data, const uint16_t *const palette, size_t bitdepth);
SASPPUImageCode SASPPU_fill_sprite(size_t x, size_t y, size_t width,
                                   size_t height, uint16_t colour);
SASPPUImageCode SASPPU_draw_text_sprite(size_t x, size_t y, uint16_t colour,
                                        size_t line_width,
                                        size_t newline_height, bool double_size,
                                        const char *text);
SASPPUImageCode SASPPU_draw_text_next_sprite(
    size_t *x, size_t *y, uint16_t colour, size_t line_start, size_t line_width,
    size_t newline_height, bool double_size, const char **text);

SASPPUImageCode SASPPU_copy_background(size_t dst_x, size_t dst_y, size_t width,
                                       size_t height, size_t src_x,
                                       size_t src_y, bool double_size);
SASPPUImageCode SASPPU_copy_background_transparent(size_t dst_x, size_t dst_y,
                                                   size_t width, size_t height,
                                                   size_t src_x, size_t src_y,
                                                   bool double_size);
SASPPUImageCode SASPPU_blit_background(size_t x, size_t y, size_t width,
                                       size_t height, bool double_size,
                                       const uint16_t *data);
SASPPUImageCode SASPPU_blit_background_transparent(size_t x, size_t y,
                                                   size_t width, size_t height,
                                                   bool double_size,
                                                   const uint16_t *data);
SASPPUImageCode SASPPU_paletted_background(size_t x, size_t y, size_t width,
                                           size_t height, bool double_size,
                                           const uint8_t *data,
                                           const uint16_t *const palette,
                                           size_t bitdepth);
SASPPUImageCode SASPPU_paletted_background_transparent(
    size_t x, size_t y, size_t width, size_t height, bool double_size,
    const uint8_t *data, const uint16_t *const palette, size_t bitdepth);
SASPPUImageCode SASPPU_compressed_background(size_t x, size_t y, size_t width,
                                             size_t height, bool double_size,
                                             const uint8_t *data,
                                             const uint16_t *const palette,
                                             size_t bitdepth);
SASPPUImageCode SASPPU_compressed_background_transparent(
    size_t x, size_t y, size_t width, size_t height, bool double_size,
    const uint8_t *data, const uint16_t *const palette, size_t bitdepth);
SASPPUImageCode SASPPU_fill_background(size_t x, size_t y, size_t width,
                                       size_t height, uint16_t colour);
SASPPUImageCode SASPPU_draw_text_background(size_t x, size_t y, uint16_t colour,
                                            size_t line_width,
                                            size_t newline_height,
                                            bool double_size, const char *text);
SASPPUImageCode SASPPU_draw_text_next_background(
    size_t *x, size_t *y, uint16_t colour, size_t line_start, size_t line_width,
    size_t newline_height, bool double_size, const char **text);

void SASPPU_get_text_size(size_t *width, size_t *height, size_t line_width,
                          size_t newline_height, bool double_size,
                          const char *text);

void SASPPU_gfx_reset();

#endif // SASPPU_HELP_H_
