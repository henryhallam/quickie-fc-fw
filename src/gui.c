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
#include <string.h>


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
  gui.page = GUI_HOME;
  gui.page_state = 0; // init
  lcd_clear();
          //        banner();
  //touch_cal();
        /*
        lcd_textbox_prep(LCD_W - 125, 22, 125, 155, LCD_BLACK);
        lcd_printf(LCD_GREEN, &FreeMono9pt7b, "%s", kittybutt);
        lcd_textbox_show();
        */
}


#define BUTTONS_N_MAX 10
#define BUTTON_COLOR_BG LCD_PURPLE
#define BUTTON_COLOR_BORDER LCD_WHITE
#define BUTTON_COLOR_TEXT LCD_WHITE
#define BUTTON_COLOR_HOVER LCD_RED
#define BUTTON_FONT FreeSans9pt7b
#define BUTTON_FONT_W (BUTTON_FONT.glyph[0].xAdvance)
#define BUTTON_FONT_H (BUTTON_FONT.yAdvance)
struct button {
  int16_t x, y, w, h;
  const char *caption;
  bool hover;
} buttons[BUTTONS_N_MAX];

int n_buttons = 0;

static void draw_buttons(void) {
  for (int i = 0; i < n_buttons; i++) {
    struct button *butt = &buttons[i];
    uint16_t *fb = lcd_textbox_prep(butt->x, butt->y, butt->w, butt->h, BUTTON_COLOR_BG);

    // Find size of caption so we can center it
    int text_w, text_h;
    lcd_fake_printf(&text_w, &text_h, &BUTTON_FONT, "%s", butt->caption);
    // Print caption
    int text_x = (butt->w - text_w) / 2;
    int text_y = (butt->h - text_h) / 2 + 3;
    lcd_textbox_move_cursor(text_x, text_y, 0);
    lcd_printf(BUTTON_COLOR_TEXT, &BUTTON_FONT, "%s", butt->caption);
    
    // Draw border
    for (int x = 0; x < butt->w; x++) {
      fb[x] = BUTTON_COLOR_BORDER;
      fb[(butt->h - 1) * butt->w + x] = BUTTON_COLOR_BORDER;
    }
    for (int y = 0; y < butt->h; y++) {
      fb[y * butt->w + 0] = BUTTON_COLOR_BORDER;
      fb[y * butt->w + (butt->w - 1)] = BUTTON_COLOR_BORDER;
    }
    lcd_textbox_show();
  }
}

static void redraw_border(const struct button *button, uint16_t color) {
  lcd_fill_rect(button->x, button->y, button->w, 1, color);
  lcd_fill_rect(button->x, button->y + button->h - 1, button->w, 1, color);
  lcd_fill_rect(button->x, button->y + 1, 1, button->h - 2, color);
  lcd_fill_rect(button->x + button->w - 1, button->y + 1, 1, button->h - 2, color);
}

static void handle_buttons(void) {
  int touch_x, touch_y, touch_z;
  touch_z = touch_get(&touch_x, &touch_y);
  for (int i = 0; i < n_buttons; i++) {
    struct button *butt = &buttons[i];
    if (touch_z) {
      bool within_borders = touch_x > butt->x && touch_x < butt->x + butt->w
        && touch_y > butt->y && touch_y < butt->y + butt->h;
      if (within_borders && !butt->hover) {
        butt->hover = 1;
        redraw_border(butt, BUTTON_COLOR_HOVER);
      } else if (!within_borders && butt->hover) {
        butt->hover = 0;
        redraw_border(butt, BUTTON_COLOR_BORDER);
      }
    } else {
      // no touch
      if (butt->hover) {
        // Click!
        lcd_textbox_prep(0, LCD_H - 14 * 2, 160, 14, LCD_BLACK);
        static int clicks = 0;
        lcd_printf(LCD_GREEN, &RobotoCondensed_Regular7pt7b, "Clicked '%s' (%d)", butt->caption, ++clicks);
        lcd_textbox_show();
        butt->hover = 0;
        redraw_border(butt, BUTTON_COLOR_BORDER);
      }
    }
  }
}

static void gui_home_update(void) {
  enum gui_home_state { INIT = 0, DRAW_LEFT, DRAW_CENTER, DRAW_RIGHT, DRAW_BOTTOM };
  switch (gui.page_state) {
  case INIT:
    gui_mc_draw_frame();
    gui_main_draw_frame();
    gui_bat_draw_frame();
    n_buttons = 2;
    buttons[0].x = 0; buttons[0].y = GUI_MC_H; buttons[0].w = GUI_MC_W / 2 + 1; buttons[0].h = 64; buttons[0].caption = "L MC"; buttons[0].hover = 0;
    buttons[1].x = GUI_MC_W / 2; buttons[1].y = GUI_MC_H; buttons[1].w = GUI_MC_W / 2; buttons[1].h = 64; buttons[1].caption = "R MC"; buttons[1].hover = 0;
    draw_buttons();
    gui.page_state = DRAW_LEFT;
    break;
  case DRAW_LEFT:
    gui_mc_draw_data();
    gui.page_state = DRAW_CENTER;
    break;
  case DRAW_CENTER:
    gui_main_draw_data();
    gui.page_state = DRAW_RIGHT;
    break;
  case DRAW_RIGHT:
    gui_bat_draw_data();
    handle_buttons();
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
    //    touch_show_debug();
    //    can_show_debug();
    break;
  default:
    // invalid state - reset to home
    gui.page = GUI_HOME;
    gui.page_state = 0; // init
    lcd_clear();
    break;
  }
  update_fps();
}
