#ifndef LCD__H
#define LCD__H

#include <stdint.h>

#define LCD_W 480
#define LCD_H 320

// Color definitions
#define	LCD_BLACK   0x0000
#define	LCD_BLUE    0x001F
#define	LCD_RED     0xF800
#define	LCD_GREEN   0x07E0
#define LCD_CYAN    0x07FF
#define LCD_MAGENTA 0xF81F
#define LCD_YELLOW  0xFFE0  
#define LCD_WHITE   0xFFFF
#define LCD_RGB565(r,g,b) ((r) << 11 | (g) << 5 | (b))
#define LCD_PURPLE  LCD_RGB565(13,13,19)
#define LCD_DARKGREY  LCD_RGB565(4,8,4)
void lcd_setup(void);
void lcd_demo(void);
void lcd_clear(void);
void lcd_backlight_set(int level);
void lcd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void lcd_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);

uint16_t *lcd_fb_prep(int x, int y, int w, int h, uint16_t bg);
void lcd_fb_show(void);

#endif // LCD__H
