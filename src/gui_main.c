#include "gui_main.h"
#include "lcd.h"
#include "font.h"
#include "mc_telem.h"
#include <stdio.h>
#include <math.h>

#define TITLE_H 22
#define TITLE_FONT FreeSans9pt7b
#define DATA_H 21
#define RPM_FONT FreeSans12pt7b
#define TORQUE_FONT FreeSans9pt7b
#define FOOTER_H 14
#define FOOTER_FONT RobotoCondensed_Regular7pt7b

#define BAR_BASE_Y (GUI_MAIN_Y + GUI_MAIN_H - FOOTER_H)
#define BAR_EXTENT_Y (GUI_MAIN_H - TITLE_H - DATA_H - FOOTER_H)
#define BAR_W 20

#define BAR_RED_LEVEL 0.8
#define BAR_YELLOW_LEVEL 0.6

#define RPM_RED 6000.0
#define RPM_YELLOW 4800.0
#define RPM_BASE_COLOR LCD_BLUE
#define RPM_SCALE ((BAR_RED_LEVEL - BAR_YELLOW_LEVEL) / (RPM_RED - RPM_YELLOW))
#define RPM_OFFSET (BAR_RED_LEVEL - (RPM_RED * RPM_SCALE))

#define TORQUE_RED 4.0
#define TORQUE_YELLOW 3.0
#define TORQUE_BASE_COLOR LCD_PURPLE
#define TORQUE_SCALE ((BAR_RED_LEVEL - BAR_YELLOW_LEVEL) / (TORQUE_RED - TORQUE_YELLOW))
#define TORQUE_OFFSET (BAR_RED_LEVEL - (TORQUE_RED * TORQUE_SCALE))

#define TEMP_RED 120.0
#define TEMP_YELLOW 90.0
#define TEMP_BASE_COLOR LCD_MEDIUMGREEN
#define TEMP_SCALE ((BAR_RED_LEVEL - BAR_YELLOW_LEVEL) / (TEMP_RED - TEMP_YELLOW))
#define TEMP_OFFSET (BAR_RED_LEVEL - (TEMP_RED * TEMP_SCALE))

int gui_main_draw_frame(void) {
  if (GUI_MAIN_X < 0 || GUI_MAIN_Y < 0 || GUI_MAIN_X + GUI_MAIN_W > LCD_W || GUI_MAIN_Y + GUI_MAIN_H > LCD_H)
    return -1;

  // Header
  lcd_textbox_prep(GUI_MAIN_X, GUI_MAIN_Y, GUI_MAIN_W, TITLE_H, LCD_BLACK);
  lcd_textbox_move_cursor(28, 14, 0);
  lcd_printf(LCD_LIGHTGREY, &TITLE_FONT, "Torq  RPM  Torq");
  lcd_textbox_show();

  // The region where the data and bars will go
  lcd_fill_rect(GUI_MAIN_X, GUI_MAIN_Y + TITLE_H, GUI_MAIN_W, GUI_MAIN_H - TITLE_H - FOOTER_H, LCD_BLACK);
  // Threshold markers
  lcd_fill_rect(GUI_MAIN_X, BAR_BASE_Y - BAR_EXTENT_Y * BAR_YELLOW_LEVEL, GUI_MAIN_W, 1, LCD_DARKYELLOW);
  lcd_fill_rect(GUI_MAIN_X, BAR_BASE_Y - BAR_EXTENT_Y * BAR_RED_LEVEL, GUI_MAIN_W, 1, LCD_DARKRED);

  // Footer ("Temp    Temp")
  lcd_textbox_prep(GUI_MAIN_X, GUI_MAIN_Y + GUI_MAIN_H - FOOTER_H, GUI_MAIN_W, FOOTER_H, LCD_BLACK);
  lcd_textbox_move_cursor(0, 10, 0);
  lcd_printf(LCD_LIGHTGREY, &FOOTER_FONT, "Temp");
  lcd_textbox_move_cursor(GUI_MAIN_W - 32, 10, 0);
  lcd_printf(LCD_LIGHTGREY, &FOOTER_FONT, "Temp");
  lcd_textbox_show();
  return 0;
}

static void draw_bar(int x, float v, uint16_t color) {
  if (v < 0.0 || !isnormal(v)) {
    v = 0.0;
  } else if (v > 1.0) {
    v = 1.0;
  }
  // Overall bar height
  int h_px = BAR_EXTENT_Y * v;
  
  // Fill the remainder with black
  lcd_fill_rect(GUI_MAIN_X + x, BAR_BASE_Y - BAR_EXTENT_Y, BAR_W, BAR_EXTENT_Y - h_px, LCD_BLACK);
  // Threshold markers  TODO: fix flicker
  lcd_fill_rect(GUI_MAIN_X + x, BAR_BASE_Y - BAR_EXTENT_Y * BAR_YELLOW_LEVEL, BAR_W, 1, LCD_DARKYELLOW);
  lcd_fill_rect(GUI_MAIN_X + x, BAR_BASE_Y - BAR_EXTENT_Y * BAR_RED_LEVEL, BAR_W, 1, LCD_DARKRED);
  
  // The base section
  int h_px_seg = (v < BAR_YELLOW_LEVEL) ? h_px : (BAR_EXTENT_Y * BAR_YELLOW_LEVEL - 1);
  lcd_fill_rect(GUI_MAIN_X + x, BAR_BASE_Y - h_px_seg - 1, BAR_W, h_px_seg, color);
  // Any in the yellow
  if (v >= BAR_YELLOW_LEVEL) {
    h_px_seg = (v < BAR_RED_LEVEL) ? h_px - (BAR_EXTENT_Y * BAR_YELLOW_LEVEL) : (BAR_EXTENT_Y * BAR_RED_LEVEL - BAR_EXTENT_Y * BAR_YELLOW_LEVEL);
    lcd_fill_rect(GUI_MAIN_X + x, BAR_BASE_Y - BAR_YELLOW_LEVEL * BAR_EXTENT_Y - h_px_seg, BAR_W, h_px_seg, LCD_YELLOW);
    if (v >= BAR_RED_LEVEL) {
      h_px_seg = h_px - (BAR_EXTENT_Y * BAR_RED_LEVEL);
      lcd_fill_rect(GUI_MAIN_X + x, BAR_BASE_Y - BAR_RED_LEVEL * BAR_EXTENT_Y - h_px_seg, BAR_W, h_px_seg, LCD_RED);
    }
  }
  
}

void gui_main_draw_data(void) {
  lcd_textbox_prep(GUI_MAIN_X, GUI_MAIN_Y + TITLE_H, GUI_MAIN_W, DATA_H, LCD_BLACK);
  lcd_textbox_move_cursor(22, 14, 0);
  lcd_printf(LCD_LIGHTGREY, &TORQUE_FONT, "%5.1f", mc_telem[MC_LEFT].torque);
  lcd_textbox_move_cursor(GUI_MAIN_W / 2 - 25, 16, 0);
  lcd_printf(LCD_WHITE, &RPM_FONT, "%4.0f", mc_telem[MC_RIGHT].rpm);  // TODO: Fuse left and right RPM estimates
  lcd_textbox_move_cursor(GUI_MAIN_W - 68, 14, 0);
  lcd_printf(LCD_LIGHTGREY, &TORQUE_FONT, "%5.1f", mc_telem[MC_RIGHT].torque);
  lcd_textbox_show();
  draw_bar(10, mc_telem[MC_LEFT].temp_SiC * TEMP_SCALE + TEMP_OFFSET, TEMP_BASE_COLOR);
  draw_bar(GUI_MAIN_W / 2 - 2 * BAR_W - 5, mc_telem[MC_LEFT].torque * TORQUE_SCALE + TORQUE_OFFSET, TORQUE_BASE_COLOR);
  draw_bar(GUI_MAIN_W / 2 - BAR_W, mc_telem[MC_LEFT].rpm * RPM_SCALE + RPM_OFFSET, RPM_BASE_COLOR);
  draw_bar(GUI_MAIN_W / 2, mc_telem[MC_RIGHT].rpm * RPM_SCALE + RPM_OFFSET, RPM_BASE_COLOR);
  draw_bar(GUI_MAIN_W / 2 + BAR_W + 5, mc_telem[MC_RIGHT].torque * TORQUE_SCALE + TORQUE_OFFSET, TORQUE_BASE_COLOR);
  draw_bar(GUI_MAIN_W - 10 - BAR_W, mc_telem[MC_RIGHT].temp_SiC * TEMP_SCALE + TEMP_OFFSET, TEMP_BASE_COLOR);
}
