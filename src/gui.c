#include "gui.h"
#include "board.h"
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
#include <libopencm3/cm3/scb.h>

/* Convention: To move to a new page:
   1. Clear the screen
   2. Set gui.page = GUI_WHATEVER
   3. Set gui.page_state = 0 - this is INIT for all pages.
*/

typedef enum {
  GUI_HOME, GUI_MENU, GUI_MSGS,
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

int bringup_button_pressed, bringup_dirty;


#define DIM(arr) (sizeof(arr) / sizeof(arr[0]))

#define BUTTON_COLOR_BORDER LCD_WHITE
#define BUTTON_COLOR_TEXT LCD_WHITE
#define BUTTON_COLOR_HOVER LCD_RED
#define BUTTON_FONT FreeSans9pt7b
#define BUTTON_DEBOUNCE_MS 100
#define BUTTON_FONT_W (BUTTON_FONT.glyph[0].xAdvance)
#define BUTTON_FONT_H (BUTTON_FONT.yAdvance)
typedef struct {
  int16_t x, y, w, h;
  uint16_t bg_color;
  const char *caption;
  bool hover;
  bool clicked;
} button_t;

enum buttons_home_e {
  BUTT_HOME_LMC, BUTT_HOME_RMC, BUTT_HOME_MODE1, BUTT_HOME_MODE2, BUTT_HOME_MENU
};
button_t buttons_home[] = {[BUTT_HOME_LMC] = {0, GUI_MC_H, GUI_MC_W / 2 + 1, 64, LCD_PURPLE, "L MC", 0, 0},
                           [BUTT_HOME_RMC] = {GUI_MC_W / 2, GUI_MC_H, GUI_MC_W / 2, 64, LCD_PURPLE, "R MC", 0, 0},
                           [BUTT_HOME_MODE1] = {GUI_MC_W, GUI_MC_H, GUI_MAIN_W, 64, LCD_BLUE, "Start", 0, 0},
                           [BUTT_HOME_MODE2] = {GUI_MC_W + GUI_MAIN_W, GUI_MC_H, GUI_BAT_W / 2 + 1, 64, LCD_GREY, "Chg", 0, 0},
                           [BUTT_HOME_MENU] = {GUI_MC_W + GUI_MAIN_W + GUI_BAT_W / 2, GUI_MC_H, GUI_BAT_W / 2, 64, LCD_GREY, "Menu", 0, 0},
};


static void draw_buttons(button_t *buttons, int n_buttons) {
  for (int i = 0; i < n_buttons; i++) {
    button_t *butt = &buttons[i];
    butt->hover = 0;
    uint16_t *fb = lcd_textbox_prep(butt->x, butt->y, butt->w, butt->h, butt->bg_color);

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

static void redraw_border(const button_t *button, uint16_t color) {
  lcd_fill_rect(button->x, button->y, button->w, 1, color);
  lcd_fill_rect(button->x, button->y + button->h - 1, button->w, 1, color);
  lcd_fill_rect(button->x, button->y + 1, 1, button->h - 2, color);
  lcd_fill_rect(button->x + button->w - 1, button->y + 1, 1, button->h - 2, color);
}

static void handle_buttons(button_t *buttons, int n_buttons) {
  static uint32_t t_down = 0;
  int touch_x, touch_y, touch_z;
  touch_z = touch_get(&touch_x, &touch_y);
  for (int i = 0; i < n_buttons; i++) {
    button_t *butt = &buttons[i];
    butt->clicked = 0;
    if (touch_z) {
      // Touching
      bool within_borders = touch_x > butt->x && touch_x < butt->x + butt->w
        && touch_y > butt->y && touch_y < butt->y + butt->h;
      if (within_borders && !butt->hover) {
        butt->hover = 1;
        redraw_border(butt, BUTTON_COLOR_HOVER);
        t_down = mtime();
      } else if (!within_borders && butt->hover) {
        butt->hover = 0;
        redraw_border(butt, BUTTON_COLOR_BORDER);
      }
    } else {
      // No touching
      if (butt->hover) {
        redraw_border(butt, BUTTON_COLOR_BORDER);
        // Click!
        if (mtime() - t_down >= BUTTON_DEBOUNCE_MS) {
          butt->clicked = 1;
          butt->hover = 0;
        }
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
    draw_buttons(buttons_home, DIM(buttons_home));
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
    gui.page_state = DRAW_BOTTOM;
    break;
  case DRAW_BOTTOM:
    lcd_textbox_prep(0, LCD_H - 40 , LCD_W, 40, LCD_DARKGREY);
    lcd_printf(LCD_GREEN, &Roboto_Regular8pt7b,
	       "LV bus: %.1f V  Backup: %.1f V",
	       get_lv_bus_v(), get_9v_v());
    lcd_textbox_show();
    gui.page_state = DRAW_LEFT;
    handle_buttons(buttons_home, DIM(buttons_home));

    if (buttons_home[BUTT_HOME_MODE1].clicked) {
      bringup_button_pressed = 1;
    }



    if (bringup_dirty == 1) {
        buttons_home[BUTT_HOME_MODE1].caption = bringup_ctx_string(&bringup);
      draw_buttons(&buttons_home[BUTT_HOME_MODE1], 1);
      bringup_dirty = 0;
    }

    if (buttons_home[BUTT_HOME_MENU].clicked) {
    gui.page = GUI_MENU;
    gui.page_state = 0; // init
    lcd_clear();
    }
    break;
  }
}

enum buttons_menu_e {
  BUTT_MENU_HOME, BUTT_MENU_CAN, BUTT_MENU_TOUCH, BUTT_MENU_BATT, BUTT_MENU_REBOOT
};
button_t buttons_menu[] = {[BUTT_MENU_HOME] = {0, 20, LCD_W / 2, 64, LCD_BLUE, "Home", 0, 0},
                           [BUTT_MENU_CAN] = {LCD_W/2, 20, LCD_W / 2, 64, LCD_GREY, "Debug CAN", 0, 0},
                           [BUTT_MENU_TOUCH] = {0, 84, LCD_W / 2, 64, LCD_MEDIUMGREEN, "Touch Cal / Debug", 0, 0},
                           [BUTT_MENU_BATT] = {LCD_W/2, 84, LCD_W / 2, 64, LCD_PURPLE, "Debug Batteries", 0, 0},
                           [BUTT_MENU_REBOOT] = {0, 148, LCD_W / 2, 64, LCD_RED, "Reboot", 0, 0},
};

static void gui_menu_update(void) {
  static bool debug_touch = 0;
  enum gui_menu_state { INIT = 0, WAIT = 1 };
  switch (gui.page_state) {
  case INIT:
    lcd_textbox_prep(0, 0 , LCD_W, 20, LCD_BLACK);
    lcd_printf(LCD_WHITE, &Roboto_Regular8pt7b, "Main menu");
    lcd_textbox_show();
    draw_buttons(buttons_menu, DIM(buttons_menu));
    gui.page_state = WAIT;
    break;
  case WAIT:
    break;
  }
  handle_buttons(buttons_menu, DIM(buttons_menu));
  if (buttons_menu[BUTT_MENU_HOME].clicked) {
    gui.page = GUI_HOME; gui.page_state = 0; lcd_clear();
  }
  if (buttons_menu[BUTT_MENU_CAN].clicked) {
    lcd_clear();
    while (!touch_get(NULL, NULL))
      can_show_debug();
    gui.page = GUI_HOME; gui.page_state = 0; lcd_clear();
    while (touch_get(NULL, NULL));
  }
  if (buttons_menu[BUTT_MENU_TOUCH].clicked) {
    // Dual-purpose button: Do a touch cal, also toggle showing a line of touch debug info (this page only)
    debug_touch = !debug_touch;
    touch_cal();
    gui.page_state = INIT; // need to redraw
  }
  if (buttons_menu[BUTT_MENU_BATT].clicked) {
    lcd_clear();
    debug_bat();
    msleep(4000);
    gui.page_state = INIT;
    lcd_clear();
  }
  if (buttons_menu[BUTT_MENU_REBOOT].clicked)
    scb_reset_system();
  if (debug_touch) {
    touch_show_debug();
    // Redraw menu every so often
    static uint8_t redraw_ctr = 0;
    if (redraw_ctr++ == 0)
      gui.page_state = INIT;
  }
}

static void update_fps(void) {
  static uint32_t t_prev = 0;
  static int frames = 0;
  frames++;
  uint32_t t = mtime();
  if (t - t_prev > 1000 && t_prev != 0) {
    lcd_textbox_prep(0, 0, 50, 14, LCD_DARKGREY);
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
  case GUI_MENU:
    gui_menu_update();
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
