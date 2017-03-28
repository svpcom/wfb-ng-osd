#ifndef OSD_FONT_H__
#define OSD_FONT_H__

// New fonts need a .c file (with the data) and a .h file with
// the definitions. Add the .h files here only.
#include "font_outlined8x14.h"
#include "font_outlined8x8.h"

// This number must also be incremented for each new font.
#define NUM_FONTS           4

// Flags for fonts.
#define FONT_LOWERCASE_ONLY 1
#define FONT_UPPERCASE_ONLY 2

// Font table. (Actual list of fonts in fonts.c.)
struct FontEntry {
  int id;
  unsigned char width, height;
  const char    *name;
  const char    *lookup;
  const char    *data;
  int flags;
};

extern struct FontEntry fonts[NUM_FONTS + 1];

#endif //OSD_FONT_H__

