#ifndef FONT_H
#define FONT_H

#include <stdarg.h>
#include <stdint.h>

typedef struct { // Data stored PER GLYPH
  uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
  uint8_t  width, height;    // Bitmap dimensions in pixels
  uint8_t  xAdvance;         // Distance to advance cursor (x axis)
  int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
  const uint8_t  *bitmap;      // Glyph bitmaps, concatenated
  const GFXglyph *glyph;       // Glyph array
  uint8_t   first, last; // ASCII extents
  uint8_t   yAdvance;    // Newline distance (y axis)
  uint8_t  y_init;           // Starting cursor Y offset
} GFXfont;

extern const GFXfont Roboto_Bold7pt7b;
extern const GFXfont RobotoCondensed_Regular7pt7b;
extern const GFXfont Roboto_Bold8pt7b;
extern const GFXfont Roboto_Regular8pt7b;
extern const GFXfont Roboto_Regular12pt7b;

extern const GFXfont FreeMono12pt7b;
extern const GFXfont FreeMono18pt7b;
extern const GFXfont FreeMono24pt7b;
extern const GFXfont FreeMono9pt7b;
extern const GFXfont FreeMonoBold12pt7b;
extern const GFXfont FreeMonoBold18pt7b;
extern const GFXfont FreeMonoBold24pt7b;
extern const GFXfont FreeMonoBold9pt7b;
extern const GFXfont FreeMonoBoldOblique12pt7b;
extern const GFXfont FreeMonoBoldOblique18pt7b;
extern const GFXfont FreeMonoBoldOblique24pt7b;
extern const GFXfont FreeMonoBoldOblique9pt7b;
extern const GFXfont FreeMonoOblique12pt7b;
extern const GFXfont FreeMonoOblique18pt7b;
extern const GFXfont FreeMonoOblique24pt7b;
extern const GFXfont FreeMonoOblique9pt7b;
extern const GFXfont FreeSans12pt7b;
extern const GFXfont FreeSans18pt7b;
extern const GFXfont FreeSans24pt7b;
extern const GFXfont FreeSans9pt7b;
extern const GFXfont FreeSansBold12pt7b;
extern const GFXfont FreeSansBold18pt7b;
extern const GFXfont FreeSansBold24pt7b;
extern const GFXfont FreeSansBold9pt7b;
extern const GFXfont FreeSansBoldOblique12pt7b;
extern const GFXfont FreeSansBoldOblique18pt7b;
extern const GFXfont FreeSansBoldOblique24pt7b;
extern const GFXfont FreeSansBoldOblique9pt7b;
extern const GFXfont FreeSansOblique12pt7b;
extern const GFXfont FreeSansOblique18pt7b;
extern const GFXfont FreeSansOblique24pt7b;
extern const GFXfont FreeSansOblique9pt7b;
extern const GFXfont FreeSerif12pt7b;
extern const GFXfont FreeSerif18pt7b;
extern const GFXfont FreeSerif24pt7b;
extern const GFXfont FreeSerif9pt7b;
extern const GFXfont FreeSerifBold12pt7b;
extern const GFXfont FreeSerifBold18pt7b;
extern const GFXfont FreeSerifBold24pt7b;
extern const GFXfont FreeSerifBold9pt7b;
extern const GFXfont FreeSerifBoldItalic12pt7b;
extern const GFXfont FreeSerifBoldItalic18pt7b;
extern const GFXfont FreeSerifBoldItalic24pt7b;
extern const GFXfont FreeSerifBoldItalic9pt7b;
extern const GFXfont FreeSerifItalic12pt7b;
extern const GFXfont FreeSerifItalic18pt7b;
extern const GFXfont FreeSerifItalic24pt7b;
extern const GFXfont FreeSerifItalic9pt7b;
extern const GFXfont Org_01;
extern const GFXfont Picopixel;
extern const GFXfont Tiny3x3a2pt;
extern const GFXfont TomThumb;



uint16_t * lcd_textbox_prep(int x, int y, int w, int h, uint16_t bg);
int lcd_printf(uint16_t color, const GFXfont *font, const char *format, ...) __attribute__ ((format (printf, 3, 4)));
void lcd_fake_printf(int *w, int *h, const GFXfont *font, const char *format, ...);
void lcd_textbox_move_cursor(int x, int y, int rel);
void lcd_textbox_get_cursor(int *x, int *y);
void lcd_textbox_show(void);

#endif // FONT_H
