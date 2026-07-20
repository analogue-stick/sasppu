#include "sasppu/help.h"
#include "sasppu/sasppu.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"
#include "string.h"

#define CHECK_BOUNDS_SINGLE(x, y)                                              \
  {                                                                            \
    if ((x + width) > buffer_width) {                                          \
      return SASPPU_IC_TooWide;                                                \
    }                                                                          \
    if ((y + height) > buffer_height) {                                        \
      return SASPPU_IC_TooTall;                                                \
    }                                                                          \
  }

#define CHECK_BOUNDS_DOUBLE(x, y)                                              \
  {                                                                            \
    if ((x + (width * 2)) > buffer_width) {                                    \
      return SASPPU_IC_TooWide;                                                \
    }                                                                          \
    if ((y + (height * 2)) > buffer_height) {                                  \
      return SASPPU_IC_TooTall;                                                \
    }                                                                          \
  }

#define CHECK_BOUNDS(x, y)                                                     \
  {                                                                            \
    if (double_size) {                                                         \
      CHECK_BOUNDS_DOUBLE(x, y)                                                \
    } else {                                                                   \
      CHECK_BOUNDS_SINGLE(x, y)                                                \
    }                                                                          \
  }

#define WRITE_TO_BUFFER(x, y, x_off, y_off, pixel)                             \
  {                                                                            \
    __typeof__(x) _x = (x);                                                    \
    __typeof__(y) _y = (y);                                                    \
    __typeof__(x_off) _x_off = (x_off);                                        \
    __typeof__(y_off) _y_off = (y_off);                                        \
    __typeof__(pixel) _pixel = (pixel);                                        \
    if (pixel || !transparent) {                                               \
      if (double_size) {                                                       \
        buffer[((_y + (_y_off * 2)) * buffer_width) + (_x + (_x_off * 2))] =   \
            _pixel;                                                            \
        buffer[((_y + (_y_off * 2)) * buffer_width) +                          \
               (_x + (_x_off * 2) + 1)] = _pixel;                              \
        buffer[((_y + (_y_off * 2) + 1) * buffer_width) +                      \
               (_x + (_x_off * 2))] = _pixel;                                  \
        buffer[((_y + (_y_off * 2) + 1) * buffer_width) +                      \
               (_x + (_x_off * 2) + 1)] = _pixel;                              \
      } else {                                                                 \
        buffer[((_y + _y_off) * buffer_width) + (_x + _x_off)] = _pixel;       \
      }                                                                        \
    }                                                                          \
  }

static inline SASPPUImageCode
SASPPU_blit(size_t x, size_t y, size_t width, size_t height, bool double_size,
            const uint16_t *data, uint16_t *const buffer, size_t buffer_width,
            size_t buffer_height, bool transparent) {
  CHECK_BOUNDS(x, y);
  for (size_t yi = 0; yi < height; yi++) {
    for (size_t xi = 0; xi < width; xi++) {
      uint16_t pixel = *(data++);
      WRITE_TO_BUFFER(x, y, xi, yi, pixel);
    }
  }
  return SASPPU_IC_Success;
}

static inline SASPPUImageCode
SASPPU_copy(size_t dst_x, size_t dst_y, size_t width, size_t height,
            size_t src_x, size_t src_y, bool double_size,
            uint16_t *const buffer, size_t buffer_width, size_t buffer_height,
            bool transparent) {
  CHECK_BOUNDS(src_x, src_y);
  CHECK_BOUNDS(dst_x, dst_y);
  for (size_t yi = 0; yi < height; yi++) {
    for (size_t xi = 0; xi < width; xi++) {
      uint16_t pixel = buffer[((yi + src_y) * buffer_width) + (xi + src_x)];
      WRITE_TO_BUFFER(dst_x, dst_y, xi, yi, pixel);
    }
  }
  return SASPPU_IC_Success;
}

static inline SASPPUImageCode
SASPPU_paletted(size_t x, size_t y, size_t width, size_t height,
                bool double_size, const uint8_t *data, uint16_t *const buffer,
                const uint16_t *const palette, size_t bitdepth,
                size_t buffer_width, size_t buffer_height, bool transparent) {
  CHECK_BOUNDS(x, y);
  if (bitdepth >= 4) {
    return SASPPU_IC_InvalidBitdepth;
  }
  uint8_t pixel_shift = 0;
  size_t bits_valid = 0;
  size_t bits_per_pixel = (1 << bitdepth);
  uint8_t mask = (1 << bits_per_pixel) - 1;
  for (size_t yi = 0; yi < height; yi++) {
    for (size_t xi = 0; xi < width; xi++) {
      if (bits_valid == 0) {
        pixel_shift = *(data++);
        bits_valid = 8;
      }
      size_t index = pixel_shift & mask;
      pixel_shift >>= bits_per_pixel;
      bits_valid -= bits_per_pixel;
      uint16_t pixel = palette[index];
      WRITE_TO_BUFFER(x, y, xi, yi, pixel);
    }
  }
  return SASPPU_IC_Success;
}

static inline SASPPUImageCode
SASPPU_compressed(size_t x, size_t y, size_t width, size_t height,
                  bool double_size, const uint8_t *data, uint16_t *const buffer,
                  const uint16_t *const palette, size_t bitdepth,
                  size_t buffer_width, size_t buffer_height, bool transparent) {
  CHECK_BOUNDS(x, y);
  if (bitdepth >= 4) {
    return SASPPU_IC_InvalidBitdepth;
  }
  uint8_t pixel_shift = 0;
  size_t bits_valid = 0;
  size_t running_decode = 0;
  size_t bits_per_pixel = (1 << bitdepth);
  uint8_t mask = (1 << bits_per_pixel) - 1;
  for (size_t yi = 0; yi < height; yi++) {
    for (size_t xi = 0; xi < width; xi++) {
      if (bits_valid == 0) {
        pixel_shift = *(data++);
        bits_valid = 8;
      }
      size_t index = pixel_shift & mask;
      pixel_shift >>= bits_per_pixel;
      bits_valid -= bits_per_pixel;
      running_decode += index;
      running_decode &= mask;
      uint16_t pixel = palette[running_decode];
      WRITE_TO_BUFFER(x, y, xi, yi, pixel);
    }
  }
  return SASPPU_IC_Success;
}

static inline SASPPUImageCode
SASPPU_fill(size_t x, size_t y, size_t width, size_t height, uint16_t colour,
            uint16_t *const buffer, size_t buffer_width, size_t buffer_height) {
  static const bool double_size = false;
  static const bool transparent = false;
  CHECK_BOUNDS(x, y);
  for (size_t yi = 0; yi < height; yi++) {
    for (size_t xi = 0; xi < width; xi++) {
      WRITE_TO_BUFFER(x, y, xi, yi, colour);
    }
  }
  return SASPPU_IC_Success;
}

#include "sasppu/font/font.h"
#include "sasppu/font/metadata.h"

static inline SASPPUImageCode SASPPU_draw_text_next(
    size_t *x, size_t *y, uint16_t colour, size_t line_start, size_t line_width,
    size_t newline_height, bool double_size, const char **text,
    uint16_t *const buffer, size_t buffer_width, size_t buffer_height) {
  // uint16_t palette[] = {
  //     0,
  //     SASPPU_MUL_COL(colour, 85),
  //     SASPPU_MUL_COL(colour, 170),
  //     SASPPU_MUL_COL(colour, 256)};

  uint16_t fg_palette[] = {0, colour};
  uint16_t bg_palette[] = {0, SASPPU_GREY555(2)};

  SASPPUImageCode res = SASPPU_IC_Success;
  char next_char = *((*text)++);
  // if (next_char == 0)
  //{
  //     return res;
  // }
  if (next_char == '\n') {
    *x = line_start;
    if (double_size) {
      *y += newline_height * 2;
    } else {
      *y += newline_height;
    }
    return res;
  }
  if ((next_char < 0x20) || (next_char == 127)) {
    return res;
  }

  if (*x != line_start) {
    if (*x >= (line_start + line_width)) {
      *x = line_start;
      if (double_size) {
        *y += newline_height * 2;
      } else {
        *y += newline_height;
      }
    }
  }

  CharacterData data = CHARACTER_DATA[next_char - 0x20];
  const uint8_t *glyph_start = SASPPU_font + data.offset;
  res = SASPPU_paletted((*x) + 1, (*y) + 1, data.width, data.height - 2,
                        double_size, glyph_start, buffer, bg_palette, 0,
                        buffer_width, buffer_height, true);
  if (res != SASPPU_IC_Success) {
    return res;
  }
  res = SASPPU_paletted(*x, *y, data.width, data.height - 2, double_size,
                        glyph_start, buffer, fg_palette, 0, buffer_width,
                        buffer_height, true);
  if (res != SASPPU_IC_Success) {
    return res;
  }

  if (double_size) {
    *x += data.width * 2;
  } else {
    *x += data.width;
  }
  return res;
}

static inline SASPPUImageCode
SASPPU_draw_text(size_t x, size_t y, uint16_t colour, size_t line_width,
                 size_t newline_height, bool double_size, const char *text,
                 uint16_t *const buffer, size_t buffer_width,
                 size_t buffer_height) {
  size_t line_start = x;
  if (line_start + line_width >= buffer_width) {
    return SASPPU_IC_TooWide;
  }
  SASPPUImageCode res = SASPPU_IC_Success;
  while (*text != 0) {
    res = SASPPU_draw_text_next(&x, &y, colour, line_start, line_width,
                                newline_height, double_size, &text, buffer,
                                buffer_width, buffer_height);
    if (res != SASPPU_IC_Success) {
      return res;
    }
  }
  return res;
}

void SASPPU_get_text_size(size_t *width, size_t *height, size_t line_width,
                          size_t newline_height, bool double_size,
                          const char *text) {
  *width = 0;
  *height = 0;
  size_t x = 0;
  while (1) {
    char next_char = *(text++);
    if (next_char == 0) {
      if (double_size) {
        *height += 10;
      } else {
        *height += 10;
      }
      return;
    }
    if (next_char == '\n') {
      x = 0;
      if (double_size) {
        *height += newline_height * 2;
      } else {
        *height += newline_height;
      }
      continue;
    }
    if ((next_char < 0x20) || (next_char == 127)) {
      continue;
    }

    if (x != 0) {
      if (x >= line_width) {
        x = 0;
        if (double_size) {
          *height += newline_height * 2;
        } else {
          *height += newline_height;
        }
      }
    }

    CharacterData data = CHARACTER_DATA[next_char - 0x20];
    if (double_size) {
      x += data.width * 2;
    } else {
      x += data.width;
    }
    if (x > *width) {
      *width = x;
    }
  }
}

SASPPUImageCode SASPPU_copy_sprite(SpritePlane sprite_plane, size_t dst_x,
                                   size_t dst_y, size_t width, size_t height,
                                   size_t src_x, size_t src_y,
                                   bool double_size) {
  return SASPPU_copy(dst_x, dst_y, width, height, src_x, src_y, double_size,
                     (uint16_t *)sprite_plane, SPR_WIDTH, SPR_HEIGHT, false);
}
SASPPUImageCode SASPPU_copy_sprite_transparent(SpritePlane sprite_plane,
                                               size_t dst_x, size_t dst_y,
                                               size_t width, size_t height,
                                               size_t src_x, size_t src_y,
                                               bool double_size) {
  return SASPPU_copy(dst_x, dst_y, width, height, src_x, src_y, double_size,
                     (uint16_t *)sprite_plane, SPR_WIDTH, SPR_HEIGHT, true);
}
SASPPUImageCode SASPPU_blit_sprite(SpritePlane sprite_plane, size_t x, size_t y,
                                   size_t width, size_t height,
                                   bool double_size, const uint16_t *data) {
  return SASPPU_blit(x, y, width, height, double_size, data,
                     (uint16_t *)sprite_plane, SPR_WIDTH, SPR_HEIGHT, false);
}
SASPPUImageCode SASPPU_blit_sprite_transparent(SpritePlane sprite_plane,
                                               size_t x, size_t y, size_t width,
                                               size_t height, bool double_size,
                                               const uint16_t *data) {
  return SASPPU_blit(x, y, width, height, double_size, data,
                     (uint16_t *)sprite_plane, SPR_WIDTH, SPR_HEIGHT, true);
}
SASPPUImageCode SASPPU_paletted_sprite(SpritePlane sprite_plane, size_t x,
                                       size_t y, size_t width, size_t height,
                                       bool double_size, const uint8_t *data,
                                       const uint16_t *const palette,
                                       size_t bitdepth) {
  return SASPPU_paletted(x, y, width, height, double_size, data,
                         (uint16_t *)sprite_plane, palette, bitdepth, SPR_WIDTH,
                         SPR_HEIGHT, false);
}
SASPPUImageCode SASPPU_paletted_sprite_transparent(
    SpritePlane sprite_plane, size_t x, size_t y, size_t width, size_t height,
    bool double_size, const uint8_t *data, const uint16_t *const palette,
    size_t bitdepth) {
  return SASPPU_paletted(x, y, width, height, double_size, data,
                         (uint16_t *)sprite_plane, palette, bitdepth, SPR_WIDTH,
                         SPR_HEIGHT, true);
}
SASPPUImageCode SASPPU_compressed_sprite(SpritePlane sprite_plane, size_t x,
                                         size_t y, size_t width, size_t height,
                                         bool double_size, const uint8_t *data,
                                         const uint16_t *const palette,
                                         size_t bitdepth) {
  return SASPPU_compressed(x, y, width, height, double_size, data,
                           (uint16_t *)sprite_plane, palette, bitdepth,
                           SPR_WIDTH, SPR_HEIGHT, false);
}
SASPPUImageCode SASPPU_compressed_sprite_transparent(
    SpritePlane sprite_plane, size_t x, size_t y, size_t width, size_t height,
    bool double_size, const uint8_t *data, const uint16_t *const palette,
    size_t bitdepth) {
  return SASPPU_compressed(x, y, width, height, double_size, data,
                           (uint16_t *)sprite_plane, palette, bitdepth,
                           SPR_WIDTH, SPR_HEIGHT, true);
}
SASPPUImageCode SASPPU_fill_sprite(SpritePlane sprite_plane, size_t x, size_t y,
                                   size_t width, size_t height,
                                   uint16_t colour) {
  return SASPPU_fill(x, y, width, height, colour, (uint16_t *)sprite_plane,
                     SPR_WIDTH, SPR_HEIGHT);
}
SASPPUImageCode SASPPU_draw_text_sprite(SpritePlane sprite_plane, size_t x,
                                        size_t y, uint16_t colour,
                                        size_t line_width,
                                        size_t newline_height, bool double_size,
                                        const char *text) {
  return SASPPU_draw_text(x, y, colour, line_width, newline_height, double_size,
                          text, (uint16_t *)sprite_plane, SPR_WIDTH,
                          SPR_HEIGHT);
}
SASPPUImageCode
SASPPU_draw_text_next_sprite(SpritePlane sprite_plane, size_t *x, size_t *y,
                             uint16_t colour, size_t line_start,
                             size_t line_width, size_t newline_height,
                             bool double_size, const char **text) {
  return SASPPU_draw_text_next(x, y, colour, line_start, line_width,
                               newline_height, double_size, text,
                               (uint16_t *)sprite_plane, SPR_WIDTH, SPR_HEIGHT);
}

SASPPUImageCode SASPPU_copy_background(BackgroundPlane background_plane,
                                       size_t dst_x, size_t dst_y, size_t width,
                                       size_t height, size_t src_x,
                                       size_t src_y, bool double_size) {
  return SASPPU_copy(dst_x, dst_y, width, height, src_x, src_y, double_size,
                     (uint16_t *)background_plane, BG_WIDTH, BG_HEIGHT, false);
}
SASPPUImageCode SASPPU_copy_background_transparent(
    BackgroundPlane background_plane, size_t dst_x, size_t dst_y, size_t width,
    size_t height, size_t src_x, size_t src_y, bool double_size) {
  return SASPPU_copy(dst_x, dst_y, width, height, src_x, src_y, double_size,
                     (uint16_t *)background_plane, BG_WIDTH, BG_HEIGHT, true);
}
SASPPUImageCode SASPPU_blit_background(BackgroundPlane background_plane,
                                       size_t x, size_t y, size_t width,
                                       size_t height, bool double_size,
                                       const uint16_t *data) {
  return SASPPU_blit(x, y, width, height, double_size, data,
                     (uint16_t *)background_plane, BG_WIDTH, BG_HEIGHT, false);
}
SASPPUImageCode
SASPPU_blit_background_transparent(BackgroundPlane background_plane, size_t x,
                                   size_t y, size_t width, size_t height,
                                   bool double_size, const uint16_t *data) {
  return SASPPU_blit(x, y, width, height, double_size, data,
                     (uint16_t *)background_plane, BG_WIDTH, BG_HEIGHT, true);
}
SASPPUImageCode SASPPU_paletted_background(BackgroundPlane background_plane,
                                           size_t x, size_t y, size_t width,
                                           size_t height, bool double_size,
                                           const uint8_t *data,
                                           const uint16_t *const palette,
                                           size_t bitdepth) {
  return SASPPU_paletted(x, y, width, height, double_size, data,
                         (uint16_t *)background_plane, palette, bitdepth,
                         BG_WIDTH, BG_HEIGHT, false);
}
SASPPUImageCode SASPPU_paletted_background_transparent(
    BackgroundPlane background_plane, size_t x, size_t y, size_t width,
    size_t height, bool double_size, const uint8_t *data,
    const uint16_t *const palette, size_t bitdepth) {
  return SASPPU_paletted(x, y, width, height, double_size, data,
                         (uint16_t *)background_plane, palette, bitdepth,
                         BG_WIDTH, BG_HEIGHT, true);
}
SASPPUImageCode SASPPU_compressed_background(BackgroundPlane background_plane,
                                             size_t x, size_t y, size_t width,
                                             size_t height, bool double_size,
                                             const uint8_t *data,
                                             const uint16_t *const palette,
                                             size_t bitdepth) {
  return SASPPU_compressed(x, y, width, height, double_size, data,
                           (uint16_t *)background_plane, palette, bitdepth,
                           BG_WIDTH, BG_HEIGHT, false);
}
SASPPUImageCode SASPPU_compressed_background_transparent(
    BackgroundPlane background_plane, size_t x, size_t y, size_t width,
    size_t height, bool double_size, const uint8_t *data,
    const uint16_t *const palette, size_t bitdepth) {
  return SASPPU_compressed(x, y, width, height, double_size, data,
                           (uint16_t *)background_plane, palette, bitdepth,
                           BG_WIDTH, BG_HEIGHT, true);
}
SASPPUImageCode SASPPU_fill_background(BackgroundPlane background_plane,
                                       size_t x, size_t y, size_t width,
                                       size_t height, uint16_t colour) {
  return SASPPU_fill(x, y, width, height, colour, (uint16_t *)background_plane,
                     BG_WIDTH, BG_HEIGHT);
}
SASPPUImageCode SASPPU_draw_text_background(BackgroundPlane background_plane,
                                            size_t x, size_t y, uint16_t colour,
                                            size_t line_width,
                                            size_t newline_height,
                                            bool double_size,
                                            const char *text) {
  return SASPPU_draw_text(x, y, colour, line_width, newline_height, double_size,
                          text, (uint16_t *)background_plane, BG_WIDTH,
                          BG_HEIGHT);
}
SASPPUImageCode
SASPPU_draw_text_next_background(BackgroundPlane background_plane, size_t *x,
                                 size_t *y, uint16_t colour, size_t line_start,
                                 size_t line_width, size_t newline_height,
                                 bool double_size, const char **text) {
  return SASPPU_draw_text_next(
      x, y, colour, line_start, line_width, newline_height, double_size, text,
      (uint16_t *)background_plane, BG_WIDTH, BG_HEIGHT);
}

void SASPPU_render_all(uint16x8_t *fb, CommandBuffer *command_buffer) {
  SASPPU_render(fb, 0, command_buffer);
  SASPPU_render(fb, 1, command_buffer);
  SASPPU_render(fb, 2, command_buffer);
  SASPPU_render(fb, 3, command_buffer);
}
