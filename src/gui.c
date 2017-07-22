#include "gui.h"
#include "can.h"
#include "clock.h"
#include "lcd.h"
#include "font.h"
#include "gui_mc.h"
#include "gui_main.h"
#include "gui_msgs.h"
#include "gui_bat.h"
#include "gui_fun.h"
#include "touch.h"
#include <stdint.h>
#include <stdbool.h>


/* Convention: To move to a new page:
   1. Clear the screen
   2. Set gui.page = GUI_WHATEVER
   3. Set gui.page_state = 0 - this is INIT for all pages.
*/

typedef enum {
  GUI_HOME, GUI_MENU, GUI_MSGS, GUI_DEBUG,
  GUI_N_PAGES
} gui_page_e;

static struct gui_state {
  gui_page_e page;
  int page_state;
  uint32_t ticks;
} gui;

void gui_setup(void) {
  gui.page = GUI_DEBUG;
  gui.page_state = 0; // init
  lcd_clear();
          //        banner();
        //        touch_cal();
        /*
        lcd_textbox_prep(LCD_W - 125, 22, 125, 155, LCD_BLACK);
        lcd_printf(LCD_GREEN, &FreeMono9pt7b, "%s", kittybutt);
        lcd_textbox_show();
        */
}

static void gui_home_update(void) {
  enum gui_home_state { INIT = 0, DRAW_LEFT, DRAW_CENTER, DRAW_RIGHT, DRAW_BOTTOM };
  switch (gui.page_state) {
  case INIT:
    gui_mc_draw_frame();
    gui_main_draw_frame();
    gui.page_state = DRAW_LEFT;
    break;
  case DRAW_LEFT:
    gui_mc_draw_data();
    gui.page_state = DRAW_CENTER;
    break;
  case DRAW_CENTER:
    gui_main_draw_data();
    gui.page_state = DRAW_LEFT;
    break;
  case DRAW_RIGHT:
    gui.page_state = DRAW_BOTTOM;
    break;
  case DRAW_BOTTOM:
    gui.page_state = DRAW_LEFT;
    break;
  }
}

static void update_fps(void) {
  static uint32_t t_prev = 0;
  static int frames = 0;
  frames++;
  uint32_t t = mtime();
  if (t - t_prev > 1000 && t_prev != 0) {
    lcd_textbox_prep(0, LCD_H - 14, 80, 14, LCD_BLACK);
    lcd_printf(LCD_GREEN, &RobotoCondensed_Regular7pt7b, "%lu fps", frames * 1000 / (t - t_prev));
    lcd_textbox_show();
    frames = 0;
    t_prev = t;
  }
  if (t_prev == 0) t_prev = t;
}

void gui_update(void) {
  switch (gui.page) {
  case GUI_HOME:
    gui_home_update();
    break;
  case GUI_DEBUG:
    touch_show_debug();
    //    can_show_debug();
    break;
  default:
    // invalid state - reset to home
    gui.page = GUI_HOME;
    gui.page_state = 0; // init
    lcd_clear();
    break;
  }

  //update_fps();
}
