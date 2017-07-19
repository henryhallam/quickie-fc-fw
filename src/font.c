#include "font.h"
#include "lcd.h"
#include <stdio.h>

static uint16_t *fb;
static int fb_w = 0, fb_h = 0;
static int cursor_x = 0, cursor_y = 0;

#define PROGMEM
#include "Fonts/Roboto_Bold7pt7b.h"
#include "Fonts/RobotoCondensed_Regular7pt7b.h"
#include "Fonts/FreeSans24pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSansBold9pt7b.h"
#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansOblique9pt7b.h"
#include "Fonts/FreeMono9pt7b.h"
#include "Fonts/FreeMono12pt7b.h"
#include "Fonts/FreeMonoBold12pt7b.h"
#include "Fonts/FreeMonoBold9pt7b.h"

#define X_MARGIN 2

int lcd_textbox_prep(int x, int y, int w, int h, uint16_t bg) {
  fb = lcd_fb_prep(x, y, w, h, bg);
  if (!fb)
    return -1;
  fb_w = w;
  fb_h = h;
  cursor_x = X_MARGIN;
  cursor_y = -1;
  return 0;
}

void lcd_textbox_show(void) {
  lcd_fb_show();
}

void lcd_textbox_move_cursor(int x, int y, int rel) {
  if (rel) {
    cursor_x += x;
    cursor_y += y;
  } else {
    cursor_x = x;
    cursor_y = y;
  }
}

int lcd_printf(uint16_t color, const GFXfont *font, const char *format, ...) {
  char str[256];
  int n_chars;
  va_list args;
  va_start (args, format);
  n_chars = vsnprintf (str, sizeof(str), format, args);
  va_end (args);

  if (cursor_y == -1)
    cursor_y = font->y_init;
  
  for (int i = 0; i < n_chars; i++) {
    int c = str[i];
    if (c == '\n') {
      cursor_y += font->yAdvance;
      cursor_x = X_MARGIN;
      continue;
    }
    if (c < font->first || c > font->last)
      continue;
    c -= font->first;
    const GFXglyph *glyph  = &font->glyph[c];
    const uint8_t *bitmap = font->bitmap;
    
    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width, h  = glyph->height;
    int8_t   xo = glyph->xOffset, yo = glyph->yOffset;
    uint8_t  xx, yy, bits = 0, bit = 0;
    
    for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = bitmap[bo++];
        }
        if(bits & 0x80) {
          int fb_offset = cursor_x + xx + xo + (cursor_y + yy + yo) * fb_w;
          if (fb_offset >= 0 && fb_offset < fb_w * fb_h)
            fb[fb_offset] = color;
        }
        bits <<= 1;
      }
    }
    cursor_x += glyph->xAdvance;
  }
  return n_chars;
}
