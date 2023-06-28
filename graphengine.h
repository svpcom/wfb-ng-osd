#ifndef GRAPH_ENGINE_H__
#define GRAPH_ENGINE_H__

#include <pthread.h>
#include "fonts.h"

typedef enum
{
    OSD_RENDER_AUTO = 0,
    OSD_RENDER_XV,
    OSD_RENDER_GL,
} osd_render_t;

// Size of an array (num items.)
#define SIZEOF_ARRAY(x) (sizeof(x) / sizeof((x)[0]))


#define SETUP_STROKE_FILL(stroke, fill, mode)   \
    stroke = 0; fill = 0;                       \
    if (mode == 0) { stroke = 0; fill = color; }    \
    if (mode == 1) { stroke = color; fill = 0; }    \

// Line endcaps (for horizontal and vertical lines.)
#define ENDCAP_NONE  0
#define ENDCAP_ROUND 1
#define ENDCAP_FLAT  2

#define DRAW_ENDCAP_HLINE(e, x, y, s, f, l) \
    if ((e) == ENDCAP_ROUND)       /* single pixel endcap */    \
    { write_pixel_lm(x, y, f, l); }                                     \
    else if ((e) == ENDCAP_FLAT)       /* flat endcap: FIXME, quicker to draw a vertical line(?) */ \
    { write_pixel_lm(x, y - 1, s, l); write_pixel_lm(x, y, s, l); write_pixel_lm(x, y + 1, s, l); }

#define DRAW_ENDCAP_VLINE(e, x, y, s, f, l) \
    if ((e) == ENDCAP_ROUND)       /* single pixel endcap */    \
    { write_pixel_lm(x, y, f, l); }                                     \
    else if ((e) == ENDCAP_FLAT)       /* flat endcap: FIXME, quicker to draw a horizontal line(?) */ \
    { write_pixel_lm(x - 1, y, s, l); write_pixel_lm(x, y, s, l); write_pixel_lm(x + 1, y, s, l); }

// Macros for writing pixels in a midpoint circle algorithm.
#define CIRCLE_PLOT_8(cx, cy, x, y, color, mode)        \
    CIRCLE_PLOT_4(cx, cy, x, y, mode, color);           \
    if ((x) != (y)) { CIRCLE_PLOT_4(cx, cy, y, x, mode, color); }

#define CIRCLE_PLOT_4(cx, cy, x, y, color, mode)   \
    write_pixel_lm((cx) + (x), (cy) + (y), mode, color); \
    write_pixel_lm((cx) - (x), (cy) + (y), mode, color); \
    write_pixel_lm((cx) + (x), (cy) - (y), mode, color); \
    write_pixel_lm((cx) - (x), (cy) - (y), mode, color);


// Font flags.
#define FONT_BOLD      1               // bold text (no outline)
#define FONT_INVERT    2               // invert: border white, inside black
// Text alignments.
#define TEXT_VA_TOP    0
#define TEXT_VA_MIDDLE 1
#define TEXT_VA_BOTTOM 2
#define TEXT_HA_LEFT   0
#define TEXT_HA_CENTER 1
#define TEXT_HA_RIGHT  2

// Text dimension structures.
struct FontDimensions {
  int width, height;
};

// to convert metric -> imperial
// for speeds see http://en.wikipedia.org/wiki/Miles_per_hour
typedef struct {                    // from		metric			imperial
  float m_to_m_feet;                    // m		m		1.0		feet	3.280840
  float ms_to_ms_fts;                   // m/s		m/s		1.0		ft/s	3.280840
  float ms_to_kmh_mph;                  // m/s		km/h	3.6		mph		2.236936
  uint8_t char_m_feet;                  // char		'm'				'f'
  uint8_t char_ms_fts;                  // char		'm/s'			'ft/s'
} Unit;

// Home position for calculations
typedef struct {
  int32_t Latitude;
  int32_t Longitude;
  float Altitude;
  uint8_t GotHome;
  uint32_t Distance;
  uint16_t Direction;
} HomePosition;

// ADC values filtered
typedef struct {
  double rssi;
  double flight;
  double video;
  double volt;
  double curr;
} ADCfiltered;

// Max/Min macros
#define MAX3(a, b, c)                MAX(a, MAX(b, c))
#define MIN3(a, b, c)                MIN(a, MIN(b, c))
#define LIMIT(x, l, h)               MAX(l, MIN(x, h))


extern int screen_width, screen_height;

// PAL
#define GRAPHICS_WIDTH         640
#define GRAPHICS_HEIGHT        360
#define GRAPHICS_LEFT          0
#define GRAPHICS_TOP           0
#define GRAPHICS_RIGHT         (GRAPHICS_WIDTH - 1)
#define GRAPHICS_BOTTOM        (GRAPHICS_HEIGHT - 1)

#define GRAPHICS_X_MIDDLE      (GRAPHICS_WIDTH  / 2)
#define GRAPHICS_Y_MIDDLE      (GRAPHICS_HEIGHT / 2)

// Check if coordinates are valid. If not, return. Assumes signed coordinates for working correct also with values lesser than 0.
#define CHECK_COORDS(x, y)           if (x < GRAPHICS_LEFT || x > GRAPHICS_RIGHT || y < GRAPHICS_TOP || y > GRAPHICS_BOTTOM) { return; }
#define CHECK_COORD_X(x)             if (x < GRAPHICS_LEFT || x > GRAPHICS_RIGHT) { return; }
#define CHECK_COORD_Y(y)             if (y < GRAPHICS_TOP  || y > GRAPHICS_BOTTOM) { return; }

// Clip coordinates out of range. Assumes signed coordinates for working correct also with values lesser than 0.
#define CLIP_COORDS(x, y)            { CLIP_COORD_X(x); CLIP_COORD_Y(y); }
#define CLIP_COORD_X(x)              { x = x < GRAPHICS_LEFT ? GRAPHICS_LEFT : x > GRAPHICS_RIGHT ? GRAPHICS_RIGHT : x; }
#define CLIP_COORD_Y(y)              { y = y < GRAPHICS_TOP ? GRAPHICS_TOP : y > GRAPHICS_BOTTOM ? GRAPHICS_BOTTOM : y; }

// Macro to swap two variables using XOR swap.
#define SWAP(a, b)                   { a ^= b; b ^= a; a ^= b; }

uint8_t getCharData(uint16_t charPos);

void render_init(int shift_x, int shift_y, float scale_x, float scale_y);
void clearGraphics(void);
void displayGraphics(void);

//void drawArrow(uint16_t x, uint16_t y, uint16_t angle, uint16_t size);
void drawBox(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

void write_pixel_lm(int x, int y, int opaq, int color);

void write_hline_lm(int x0, int x1, int y, int color, int opaq);

void write_hline_outlined(int x0, int x1, int y, int endcap0, int endcap1, int mode, int opaq, int color);

void write_vline_lm(int x, int y0, int y1, int color, int opaq);
void write_vline_outlined(int x, int y0, int y1, int endcap0, int endcap1, int mode, int opaq, int color);

void write_filled_rectangle_lm(int x, int y, int width, int height, int color, int opaq);
void write_rectangle_outlined(int x, int y, int width, int height, int mode, int opaq);

void write_circle_outlined(int cx, int cy, int r, int dashp, int bmode, int mode, int opaq, int color);

void write_line_lm(int x0, int y0, int x1, int y1, int opaq, int color);
void write_line_outlined(int x0, int y0, int x1, int y1, int endcap0, int endcap1, int mode, int opaq);
void write_line_outlined_dashed(int x0, int y0, int x1, int y1, int endcap0, int endcap1, int mode, int opaq, int dots);

void write_triangle_filled(int x0, int y0, int x1, int y1, int x2, int y2);
void write_triangle_wire(int x0, int y0, int x1, int y1, int x2, int y2);

void write_char16(char ch, int x, int y, int font, int color);
void write_char(char ch, int x, int y, int flags, int font, int color);

void write_string(char *str, int x, int y, int xs, int ys, int va, int ha, int flags, int font);
void write_color_string(char *str, int x, int y, int xs, int ys, int va, int ha, int flags, int font, int color);

int fetch_font_info(uint8_t ch, int font, struct FontEntry *font_info, char *lookup);
void calc_text_dimensions(char *str, struct FontEntry font, int xs, int ys, struct FontDimensions *dim);

extern uint8_t* video_buf_ext;
extern pthread_mutex_t video_mutex;

#endif


