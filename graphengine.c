/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * With Grateful Acknowledgements to the projects:
 * Tau Labs - Brain FPV Flight Controller(https://github.com/BrainFPV/TauLabs)
 */

#include "bcm_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "VG/openvg.h"
#include "VG/vgu.h"
#include "fontinfo.h"
#include "shapes.h"
#include <math.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <stdio.h>

#include "graphengine.h"
#include "math3d.h"
#include "fonts.h"
#include "font12x18.h"
#include "font8x10.h"


int screen_width = 0, screen_height = 0;
static uint8_t* video_buf = NULL;


void render_init() {
    init(&screen_width, &screen_height);
    printf("Screen HW %dx%d, virtual %dx%d \n", screen_width, screen_height, GRAPHICS_WIDTH, GRAPHICS_HEIGHT);
    video_buf = malloc(GRAPHICS_WIDTH * GRAPHICS_HEIGHT * 4); // ARGB
}

void clearGraphics(void) {
    memset(video_buf, '\0', GRAPHICS_WIDTH * GRAPHICS_HEIGHT * 4);
}

void displayGraphics(void) {
    Start(screen_width, screen_height);

    unsigned int dstride = GRAPHICS_WIDTH * 4;
    VGImageFormat rgbaFormat = VG_sABGR_8888;
    VGImage img = vgCreateImage(rgbaFormat, GRAPHICS_WIDTH, GRAPHICS_HEIGHT, VG_IMAGE_QUALITY_BETTER);
    float screen_scale_x = (float)screen_width / GRAPHICS_WIDTH;
    float screen_scale_y = (float)screen_height / GRAPHICS_HEIGHT;
    float screen_scale = MIN(screen_scale_x, screen_scale_y);

    vgImageSubData(img, (void *)video_buf, dstride, rgbaFormat, 0, 0, GRAPHICS_WIDTH, GRAPHICS_HEIGHT);
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
    vgLoadIdentity();
    vgTranslate(0, 0);
    vgScale(screen_scale, screen_scale);
    vgDrawImage(img);
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgDestroyImage(img);
    End();
}

//void drawArrow(uint16_t x, uint16_t y, uint16_t angle, uint16_t size_quarter)
//{
//	float sin_angle = sin_lookup_deg(angle);
//	float cos_angle = cos_lookup_deg(angle);
//	int16_t peak_x  = (int16_t)(sin_angle * size_quarter * 2);
//	int16_t peak_y  = (int16_t)(cos_angle * size_quarter * 2);
//	int16_t d_end_x = (int16_t)(cos_angle * size_quarter);
//	int16_t d_end_y = (int16_t)(sin_angle * size_quarter);

//	write_line_lm(x + peak_x, y - peak_y, x - peak_x - d_end_x, y + peak_y - d_end_y, 1, 1);
//	write_line_lm(x + peak_x, y - peak_y, x - peak_x + d_end_x, y + peak_y + d_end_y, 1, 1);
//	write_line_lm(x, y, x - peak_x - d_end_x, y + peak_y - d_end_y, 1, 1);
//	write_line_lm(x, y, x - peak_x + d_end_x, y + peak_y + d_end_y, 1, 1);
//}

void drawBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  write_line_lm(x1, y1, x2, y1, 1, 1);       // top
  write_line_lm(x1, y1, x1, y2, 1, 1);       // left
  write_line_lm(x2, y1, x2, y2, 1, 1);       // right
  write_line_lm(x1, y2, x2, y2, 1, 1);       // bottom
}

/**
 * write_pixel_lm: write the pixel on both surfaces (level and mask.)
 * Uses current draw buffer.
 *
 * @param       x               x coordinate
 * @param       y               y coordinate
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 * @param       lmode   0 = black, 1 = white, 2 = toggle
 */
void inline write_pixel_lm(int x, int y, int mmode, int lmode){
    CHECK_COORDS(x, y);
    assert(mmode < 2 && lmode < 2);

    uint8_t mode = (mmode << 1) | lmode;
    uint32_t *ptr = ((uint32_t*)video_buf) + GRAPHICS_WIDTH * (GRAPHICS_HEIGHT - y - 1) + x;

    switch(mode)
    {
    case 0: //transparent
    case 1: //transparent
        *ptr = 0u;
        break;
    case 2: //black
        *ptr = 0xff000000u;
        break;
    case 3: //white
        *ptr = 0xffffffffu;
        break;
    }
}


/**
 * write_hline_lm: write both level and mask buffers.
 *
 * @param       x0              x0 coordinate
 * @param       x1              x1 coordinate
 * @param       y               y coordinate
 * @param       lmode   0 = clear, 1 = set, 2 = toggle
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 */
void write_hline_lm(int x0, int x1, int y, int lmode, int mmode) {
    int x;
    if (x1 < x0) SWAP(x0, x1);
    for(x = x0; x <= x1; x++) write_pixel_lm(x, y, mmode, lmode);
}

/**
 * write_hline_outlined: outlined horizontal line with varying endcaps
 * Always uses draw buffer.
 *
 * @param       x0                      x0 coordinate
 * @param       x1                      x1 coordinate
 * @param       y                       y coordinate
 * @param       endcap0         0 = none, 1 = single pixel, 2 = full cap
 * @param       endcap1         0 = none, 1 = single pixel, 2 = full cap
 * @param       mode            0 = black outline, white body, 1 = white outline, black body
 * @param       mmode           0 = clear, 1 = set, 2 = toggle
 */
void write_hline_outlined(int x0, int x1, int y, int endcap0, int endcap1, int mode, int mmode) {
  int stroke, fill;

  SETUP_STROKE_FILL(stroke, fill, mode);
  if (x0 > x1) {
    SWAP(x0, x1);
  }
  // Draw the main body of the line.
  write_hline_lm(x0 + 1, x1 - 1, y - 1, stroke, mmode);
  write_hline_lm(x0 + 1, x1 - 1, y + 1, stroke, mmode);
  write_hline_lm(x0 + 1, x1 - 1, y, fill, mmode);
  // Draw the endcaps, if any.
  DRAW_ENDCAP_HLINE(endcap0, x0, y, stroke, fill, mmode);
  DRAW_ENDCAP_HLINE(endcap1, x1, y, stroke, fill, mmode);
}


/**
 * write_vline_lm: write both level and mask buffers.
 *
 * @param       x               x coordinate
 * @param       y0              y0 coordinate
 * @param       y1              y1 coordinate
 * @param       lmode   0 = clear, 1 = set, 2 = toggle
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 */
void write_vline_lm(int x, int y0, int y1, int lmode, int mmode) {
    // TODO: an optimisation would compute the masks and apply to
    // both buffers simultaneously.
    int y;
    if (y1 < y0) SWAP(y0, y1);
    for(y = y0; y <= y1; y++) write_pixel_lm(x, y, mmode, lmode);
}

/**
 * write_vline_outlined: outlined vertical line with varying endcaps
 * Always uses draw buffer.
 *
 * @param       x                       x coordinate
 * @param       y0                      y0 coordinate
 * @param       y1                      y1 coordinate
 * @param       endcap0         0 = none, 1 = single pixel, 2 = full cap
 * @param       endcap1         0 = none, 1 = single pixel, 2 = full cap
 * @param       mode            0 = black outline, white body, 1 = white outline, black body
 * @param       mmode           0 = clear, 1 = set, 2 = toggle
 */
void write_vline_outlined(int x, int y0, int y1, int endcap0, int endcap1, int mode, int mmode) {
  int stroke, fill;

  if (y0 > y1) {
    SWAP(y0, y1);
  }
  SETUP_STROKE_FILL(stroke, fill, mode);
  // Draw the main body of the line.
  write_vline_lm(x - 1, y0 + 1, y1 - 1, stroke, mmode);
  write_vline_lm(x + 1, y0 + 1, y1 - 1, stroke, mmode);
  write_vline_lm(x, y0 + 1, y1 - 1, fill, mmode);
  // Draw the endcaps, if any.
  DRAW_ENDCAP_VLINE(endcap0, x, y0, stroke, fill, mmode);
  DRAW_ENDCAP_VLINE(endcap1, x, y1, stroke, fill, mmode);
}


/**
 * write_filled_rectangle_lm: draw a filled rectangle on both draw buffers.
 *
 * @param       x               x coordinate (left)
 * @param       y               y coordinate (top)
 * @param       width   rectangle width
 * @param       height  rectangle height
 * @param       lmode   0 = clear, 1 = set, 2 = toggle
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 */
void write_filled_rectangle_lm(int x, int y, int width, int height, int lmode, int mmode) {
    int i, j;
    for(j=y; j <= y + height; j++)
    {
        for(i=x; i <= x + width; i++)
        {
            write_pixel_lm(i, j, mmode, lmode);
        }
    }
}

/**
 * write_rectangle_outlined: draw an outline of a rectangle. Essentially
 * a convenience wrapper for draw_hline_outlined and draw_vline_outlined.
 *
 * @param       x               x coordinate (left)
 * @param       y               y coordinate (top)
 * @param       width   rectangle width
 * @param       height  rectangle height
 * @param       mode    0 = black outline, white body, 1 = white outline, black body
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 */
void write_rectangle_outlined(int x, int y, int width, int height, int mode, int mmode) {
  write_hline_outlined(x, x + width, y, ENDCAP_ROUND, ENDCAP_ROUND, mode, mmode);
  write_hline_outlined(x, x + width, y + height, ENDCAP_ROUND, ENDCAP_ROUND, mode, mmode);
  write_vline_outlined(x, y, y + height, ENDCAP_ROUND, ENDCAP_ROUND, mode, mmode);
  write_vline_outlined(x + width, y, y + height, ENDCAP_ROUND, ENDCAP_ROUND, mode, mmode);
}


/**
 * write_circle_outlined: draw an outlined circle on the draw buffer.
 *
 * @param       cx              origin x coordinate
 * @param       cy              origin y coordinate
 * @param       r               radius
 * @param       dashp   dash period (pixels) - zero for no dash
 * @param       bmode   0 = 4-neighbour border, 1 = 8-neighbour border
 * @param       mode    0 = black outline, white body, 1 = white outline, black body
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 */
void write_circle_outlined(int cx, int cy, int r, int dashp, int bmode, int mode, int mmode) {
  int stroke, fill;

  CHECK_COORDS(cx, cy);
  SETUP_STROKE_FILL(stroke, fill, mode);
  // This is a two step procedure. First, we draw the outline of the
  // circle, then we draw the inner part.
  int error = -r, x = r, y = 0;
  while (x >= y) {
    if (dashp == 0 || (y % dashp) < (dashp / 2)) {
        CIRCLE_PLOT_8(cx, cy, x + 1, y, mmode, stroke);
        CIRCLE_PLOT_8(cx, cy, x, y + 1, mmode, stroke);
        CIRCLE_PLOT_8(cx, cy, x - 1, y, mmode, stroke);
        CIRCLE_PLOT_8(cx, cy, x, y - 1, mmode, stroke);

      if (bmode == 1) {
          CIRCLE_PLOT_8(cx, cy, x + 1, y + 1, mmode, stroke);
          CIRCLE_PLOT_8(cx, cy, x - 1, y - 1, mmode, stroke);
      }
    }
    error += (y * 2) + 1;
    y++;
    if (error >= 0) {
      --x;
      error -= x * 2;
    }
  }
  error = -r;
  x     = r;
  y     = 0;
  while (x >= y) {
    if (dashp == 0 || (y % dashp) < (dashp / 2)) {
        CIRCLE_PLOT_8(cx, cy, x, y, mmode, fill);
    }
    error += (y * 2) + 1;
    y++;
    if (error >= 0) {
      --x;
      error -= x * 2;
    }
  }
}


/**
 * write_line: Draw a line of arbitrary angle.
 *
 * @param       buff    pointer to buffer to write in
 * @param       x0              first x coordinate
 * @param       y0              first y coordinate
 * @param       x1              second x coordinate
 * @param       y1              second y coordinate
 * @param       mmode   0 = clear, 1 = set, 2 = toggle
 * @param       lmode   0 = clear, 1 = set, 2 = toggle
 */
void write_line_lm(int x0, int y0, int x1, int y1, int mmode, int lmode) {
  // Based on http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  int steep = abs(y1 - y0) > abs(x1 - x0);

  if (steep) {
    SWAP(x0, y0);
    SWAP(x1, y1);
  }
  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }
  int deltax     = x1 - x0;
  int deltay = abs(y1 - y0);
  int error      = deltax / 2;
  int ystep;
  int y = y0;
  int x;       // , lasty = y, stox = 0;
  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }
  for (x = x0; x <= x1; x++) {
    if (steep) {
        write_pixel_lm(y, x, mmode, lmode);
    } else {
        write_pixel_lm(x, y, mmode, lmode);
    }
    error -= deltay;
    if (error < 0) {
      y     += ystep;
      error += deltax;
    }
  }
}


/**
 * write_line_outlined: Draw a line of arbitrary angle, with an outline.
 *
 * @param       x0                      first x coordinate
 * @param       y0                      first y coordinate
 * @param       x1                      second x coordinate
 * @param       y1                      second y coordinate
 * @param       endcap0         0 = none, 1 = single pixel, 2 = full cap
 * @param       endcap1         0 = none, 1 = single pixel, 2 = full cap
 * @param       mode            0 = black outline, white body, 1 = white outline, black body
 * @param       mmode           0 = clear, 1 = set, 2 = toggle
 */
void write_line_outlined(int x0, int y0, int x1, int y1,
                         __attribute__((unused)) int endcap0, __attribute__((unused)) int endcap1,
                         int mode, int mmode) {
  // Based on http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  // This could be improved for speed.
  int omode, imode;

  if (mode == 0) {
    omode = 0;
    imode = 1;
  } else {
    omode = 1;
    imode = 0;
  }
  int steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    SWAP(x0, y0);
    SWAP(x1, y1);
  }
  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }
  int deltax     = x1 - x0;
  int deltay = abs(y1 - y0);
  int error      = deltax / 2;
  int ystep;
  int y = y0;
  int x;
  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }
  // Draw the outline.
  for (x = x0; x <= x1; x++) {
    if (steep) {
      write_pixel_lm(y - 1, x, mmode, omode);
      write_pixel_lm(y + 1, x, mmode, omode);
      write_pixel_lm(y, x - 1, mmode, omode);
      write_pixel_lm(y, x + 1, mmode, omode);
    } else {
      write_pixel_lm(x - 1, y, mmode, omode);
      write_pixel_lm(x + 1, y, mmode, omode);
      write_pixel_lm(x, y - 1, mmode, omode);
      write_pixel_lm(x, y + 1, mmode, omode);
    }
    error -= deltay;
    if (error < 0) {
      y     += ystep;
      error += deltax;
    }
  }
  // Now draw the innards.
  error = deltax / 2;
  y     = y0;
  for (x = x0; x <= x1; x++) {
    if (steep) {
      write_pixel_lm(y, x, mmode, imode);
    } else {
      write_pixel_lm(x, y, mmode, imode);
    }
    error -= deltay;
    if (error < 0) {
      y     += ystep;
      error += deltax;
    }
  }
}


/**
 * write_line_outlined_dashed: Draw a line of arbitrary angle, with an outline, potentially dashed.
 *
 * @param       x0              first x coordinate
 * @param       y0              first y coordinate
 * @param       x1              second x coordinate
 * @param       y1              second y coordinate
 * @param       endcap0         0 = none, 1 = single pixel, 2 = full cap
 * @param       endcap1         0 = none, 1 = single pixel, 2 = full cap
 * @param       mode            0 = black outline, white body, 1 = white outline, black body
 * @param       mmode           0 = clear, 1 = set, 2 = toggle
 * @param       dots			0 = not dashed, > 0 = # of set/unset dots for the dashed innards
 */
void write_line_outlined_dashed(int x0, int y0, int x1, int y1,
                                __attribute__((unused)) int endcap0, __attribute__((unused)) int endcap1,
                                int mode, int mmode, int dots) {
  // Based on http://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
  // This could be improved for speed.
  int omode, imode;

  if (mode == 0) {
    omode = 0;
    imode = 1;
  } else {
    omode = 1;
    imode = 0;
  }
  int steep = abs(y1 - y0) > abs(x1 - x0);
  if (steep) {
    SWAP(x0, y0);
    SWAP(x1, y1);
  }
  if (x0 > x1) {
    SWAP(x0, x1);
    SWAP(y0, y1);
  }
  int deltax = x1 - x0;
  int deltay = abs(y1 - y0);
  int error  = deltax / 2;
  int ystep;
  int y = y0;
  int x;
  if (y0 < y1) {
    ystep = 1;
  } else {
    ystep = -1;
  }
  // Draw the outline.
  for (x = x0; x <= x1; x++) {
    if (steep) {
      write_pixel_lm(y - 1, x, mmode, omode);
      write_pixel_lm(y + 1, x, mmode, omode);
      write_pixel_lm(y, x - 1, mmode, omode);
      write_pixel_lm(y, x + 1, mmode, omode);
    } else {
      write_pixel_lm(x - 1, y, mmode, omode);
      write_pixel_lm(x + 1, y, mmode, omode);
      write_pixel_lm(x, y - 1, mmode, omode);
      write_pixel_lm(x, y + 1, mmode, omode);
    }
    error -= deltay;
    if (error < 0) {
      y     += ystep;
      error += deltax;
    }
  }
  // Now draw the innards.
  error = deltax / 2;
  y     = y0;
  int dot_cnt = 0;
  int draw    = 1;
  for (x = x0; x <= x1; x++) {
    if (dots && !(dot_cnt++ % dots)) {
      draw++;
    }
    if (draw % 2) {
      if (steep) {
        write_pixel_lm(y, x, mmode, imode);
      } else {
        write_pixel_lm(x, y, mmode, imode);
      }
    }
    error -= deltay;
    if (error < 0) {
      y     += ystep;
      error += deltax;
    }
  }
}

void write_triangle_filled(int x0, int y0, int x1, int y1, int x2, int y2) {
}

void write_triangle_wire(int x0, int y0, int x1, int y1, int x2, int y2) {
  write_line_lm(x0, y0, x1, y1, 1, 1);
  write_line_lm(x0, y0, x2, y2, 1, 1);
  write_line_lm(x2, y2, x1, y1, 1, 1);
}



/**
 * fetch_font_info: Fetch font info structs.
 *
 * @param       ch              character
 * @param       font    font id
 */
int fetch_font_info(uint8_t ch, int font, struct FontEntry *font_info, char *lookup) {
  // First locate the font struct.
  if ((unsigned int)font > SIZEOF_ARRAY(fonts)) {
    return 0;             // font does not exist, exit.
  }
  // Load the font info; IDs are always sequential.
  *font_info = fonts[font];
  // Locate character in font lookup table. (If required.)
  if (lookup != NULL) {
    *lookup = font_info->lookup[ch];
    if (*lookup == 0xff) {
      return 0;                   // character doesn't exist, don't bother writing it.
    }
  }
  return 1;
}

/**
 * write_char16: Draw a character on the current draw buffer.
 *
 * @param       ch      character to write
 * @param       x       x coordinate (left)
 * @param       y       y coordinate (top)
 * @param       font    font to use
 */
void write_char16(char ch, int x, int y, int font) {
  struct FontEntry font_info;
  fetch_font_info(0, font, &font_info, NULL);

  // If font only supports lowercase or uppercase, make the letter lowercase or uppercase
  // if (font_info.flags & FONT_LOWERCASE_ONLY) ch = tolower(ch);
  // if (font_info.flags & FONT_UPPERCASE_ONLY) ch = toupper(ch);

  // How wide is the character? We handle characters from 8 pixels up in this function
  if (font_info.width >= 8) {
    // We can write mask words easily.
    // Level bits are more complicated. We need to set or clear level bits, but only where the mask bit is set; otherwise, we need to leave them alone.
    // To do this, for each word, we construct an AND mask and an OR mask, and apply each individually.
    for (int dy = 0; dy < font_info.height; dy++) {
        uint16_t levels, mask;
        int row = ch * font_info.height + dy;
        if (font == 3) {
            levels = font_frame12x18[row];
            mask = font_mask12x18[row];
        }else{
            levels = font_frame8x10[row];
            mask = font_mask8x10[row];
        }
        for(int dx = 0; dx < font_info.width; dx++) {
            uint16_t xshift = font_info.width - 1;
            if (mask & (1 << (xshift - dx)))
                write_pixel_lm(x + dx + 1, y + dy + 1, 1, (levels & (1 << (xshift - dx))) ? 1 : 0);
        }
    }
  }
}

/**
 * write_char: Draw a character on the current draw buffer.
 * Currently supports outlined characters and characters with a width of up to 8 pixels.
 *
 * @param       ch      character to write
 * @param       x       x coordinate (left)
 * @param       y       y coordinate (top)
 * @param       flags   flags to write with
 * @param       font    font to use
 */
void write_char(char ch, int x, int y, int flags, int font) {
  struct FontEntry font_info;
  char lookup = 0;

  fetch_font_info(ch, font, &font_info, &lookup);

  // If font only supports lowercase or uppercase, make the letter lowercase or uppercase
  // if (font_info.flags & FONT_LOWERCASE_ONLY) ch = tolower(ch);
  // if (font_info.flags & FONT_UPPERCASE_ONLY) ch = toupper(ch);

  // How wide is the character? We handle characters up to 8 pixels in this function
  if (font_info.width <= 8) {
    // Load data pointer.
    for (int dy = 0; dy < font_info.height; dy++) {
        uint16_t levels, mask;
        int row = lookup * font_info.height * 2 + dy;
        for(int dx = 0; dx < font_info.width; dx++) {
            uint16_t xshift = font_info.width - 1;
            levels = font_info.data[row + font_info.height];
            if (flags & FONT_INVERT) {
                levels = ~levels;
            }
            mask = font_info.data[row];
            if (mask & (1 << (xshift - dx)))
                write_pixel_lm(x + dx, y + dy, 1, (levels & (1 << (xshift - dx))) ? 1 : 0);
        }
    }
  }
}

/**
 * calc_text_dimensions: Calculate the dimensions of a
 * string in a given font. Supports new lines and
 * carriage returns in text.
 *
 * @param       str                     string to calculate dimensions of
 * @param       font_info       font info structure
 * @param       xs                      horizontal spacing
 * @param       ys                      vertical spacing
 * @param       dim                     return result: struct FontDimensions
 */
void calc_text_dimensions(char *str, struct FontEntry font, int xs, int ys, struct FontDimensions *dim) {
  int max_length = 0, line_length = 0, lines = 1;

  while (*str != 0) {
    line_length++;
    if (*str == '\n' || *str == '\r') {
      if (line_length > max_length) {
        max_length = line_length;
      }
      line_length = 0;
      lines++;
    }
    str++;
  }
  if (line_length > max_length) {
    max_length = line_length;
  }
  dim->width  = max_length * (font.width + xs);
  dim->height = lines * (font.height + ys);
}

/**
 * write_string: Draw a string on the screen with certain
 * alignment parameters.
 *
 * @param       str             string to write
 * @param       x               x coordinate
 * @param       y               y coordinate
 * @param       xs              horizontal spacing
 * @param       ys              horizontal spacing
 * @param       va              vertical align
 * @param       ha              horizontal align
 * @param       flags   flags (passed to write_char)
 * @param       font    font
 */
void write_string(char *str, int x, int y, int xs, int ys, int va, int ha, int flags, int font) {

  int xx = 0, yy = 0, xx_original = 0;
  struct FontEntry font_info;
  struct FontDimensions dim;

  //font = 2;
  // Determine font info and dimensions/position of the string.
  fetch_font_info(0, font, &font_info, NULL);
  calc_text_dimensions(str, font_info, xs, ys, &dim);
  switch (va) {
  case TEXT_VA_TOP:
    yy = y;
    break;
  case TEXT_VA_MIDDLE:
    yy = y - (dim.height / 2);
    break;
  case TEXT_VA_BOTTOM:
    yy = y - dim.height;
    break;
  }
  switch (ha) {
  case TEXT_HA_LEFT:
    xx = x;
    break;
  case TEXT_HA_CENTER:
    xx = x - (dim.width / 2);
    break;
  case TEXT_HA_RIGHT:
    xx = x - dim.width;
    break;
  }
  // Then write each character.
  xx_original = xx;
  while (*str != 0) {
    if (*str == '\n' || *str == '\r') {
      yy += ys + font_info.height;
      xx  = xx_original;
    } else {
      if (xx >= GRAPHICS_LEFT && xx < GRAPHICS_RIGHT) {
        if (font_info.id < 2) {
          write_char(*str, xx, yy, flags, font);
        } else {
          write_char16(*str, xx, yy, font);
        }
      }
      xx += font_info.width + xs;
    }
    str++;
  }
}

