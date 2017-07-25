#include "gui_bat.h"
#include "lcd.h"
#include "font.h"
#include "ltc6804.h"
#include <stdio.h>
#include <math.h>

#define TITLE_H 14
#define TITLE_FONT Roboto_Bold7pt7b
#define DATA_FONT RobotoCondensed_Regular7pt7b
#define TITLE_W 52 // (GUI_MC_W * 0.36)
#define DATA_W 46 //(GUI_MC_W * 0.32)

int gui_bat_draw_frame(void) {
  if (GUI_BAT_X < 0 || GUI_BAT_Y < 0 || GUI_BAT_X + GUI_BAT_W > LCD_W || GUI_BAT_Y + GUI_BAT_H > LCD_H)
    return -1;
  lcd_textbox_prep(GUI_BAT_X, GUI_BAT_Y, GUI_BAT_W, TITLE_H, LCD_DARKGREY);
  lcd_textbox_move_cursor(TITLE_W, 0, 1);
  lcd_printf(LCD_WHITE, &TITLE_FONT, "Fuel  FW  Dist");
  lcd_textbox_show();
  lcd_textbox_prep(GUI_BAT_X, GUI_BAT_Y + TITLE_H, TITLE_W, GUI_BAT_H - TITLE_H, LCD_DARKGREY);
  lcd_printf(LCD_WHITE, &TITLE_FONT, "Vtot\nMax\nP90\nMean\nP10\nMin\nOOFam\nTmax\nFeed\ntOT\ntUV\nPwir");
  lcd_textbox_show();
  lcd_fill_rect(GUI_BAT_X + TITLE_W, GUI_BAT_Y + TITLE_H, DATA_W * 2, GUI_BAT_H - TITLE_H, LCD_DARKGREY);
  return 0;
}

void gui_bat_draw_data(void) {
}
