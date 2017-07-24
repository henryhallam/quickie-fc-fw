#include "board.h"
#include "can.h"
#include "clock.h"
#include "font.h"
#include "gui.h"
#include "lcd.h"
#include "leds.h"
#include "ltc6804.h"
#include "mc_telem.h"
#include "touch.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <stdlib.h>
#include <string.h>

#define START_BUTTON_X 170
#define START_BUTTON_Y 220
#define START_BUTTON_W 140
#define START_BUTTON_H 50

static void draw_button(const char *txt, uint16_t bg) {
  lcd_textbox_prep(START_BUTTON_X, START_BUTTON_Y, START_BUTTON_W, START_BUTTON_H, bg);
  lcd_textbox_move_cursor((START_BUTTON_W - strnlen(txt, 10) * 24)/2, 38, 1);
  lcd_printf(LCD_WHITE, &FreeSans18pt7b, "%s", txt);
  lcd_textbox_show();
}

static void handle_sys(void) {
  static enum {
    SYS_INIT, SYS_OFF, SYS_PRECHG_1, SYS_PRECHG_2, SYS_ON, SYS_COOLDOWN
  } sys_state = SYS_INIT;

  static uint32_t mtime_last = 0;

  int touch_x, touch_y, button_pressed = 0;
  
  if (touch_get(&touch_x, &touch_y)) {
    if (touch_x > START_BUTTON_X && touch_x < START_BUTTON_X + START_BUTTON_W
        && touch_y > START_BUTTON_Y && touch_y < START_BUTTON_Y + START_BUTTON_H) {
      button_pressed = 1;
    }
  }

  switch(sys_state) {
  case SYS_INIT:
    draw_button("START", LCD_BLUE);
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R, 0);
    sys_state = SYS_OFF;
    break;
  case SYS_OFF:
    if (button_pressed) {
      draw_button("PRE 1", LCD_BLACK);
      sys_state = SYS_PRECHG_1;
      mtime_last = mtime();
      relay_ctl(RLY_PRECHG, 1);
    }
    break;
  case SYS_PRECHG_1:
    if (mtime() - mc_telem[MC_RIGHT].mtime_rx < 100
        && mc_telem[MC_RIGHT].bus_v > 300.0
        && !button_pressed) { // TODO: better debouncing
      sys_state = SYS_PRECHG_2;
      mtime_last = mtime();
      relay_ctl(RLY_FW_R, 1);
      draw_button("PRE 2", LCD_BLACK);
    }
    if (mtime() - mtime_last > 500) { // Precharge failed
      draw_button("FAIL1", LCD_BLACK);
      sys_state = SYS_COOLDOWN;
      mtime_last = mtime();
    }
    break;
  case SYS_PRECHG_2:
    if (mtime() - mtime_last > 50 // ensure main contactor has time to close
        && mtime() - mc_telem[MC_RIGHT].mtime_rx < 100  // recent data
        && mc_telem[MC_RIGHT].bus_v > 300.0) {
      relay_ctl(RLY_PRECHG, 0);
      draw_button("STOP", LCD_RED);
      sys_state = SYS_ON;
    }
    if (mtime() - mtime_last > 500) { // Precharge failed
      draw_button("FAIL2", LCD_BLACK);
      sys_state = SYS_COOLDOWN;
      mtime_last = mtime();
    }
    break;
  case SYS_ON:
    if (button_pressed  // told to turn off
        || mtime() - mc_telem[MC_RIGHT].mtime_rx > 100  // or lost comm
        || mc_telem[MC_RIGHT].bus_v < 300.0) {  // or low voltage
      relay_ctl(RLY_PRECHG, 0);
      relay_ctl(RLY_FW_R, 0);
      sys_state = SYS_COOLDOWN;
      mtime_last = mtime();
      draw_button(button_pressed ? "OFF" : "FAULT", LCD_DARKRED);
    }
    break;
  case SYS_COOLDOWN:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R, 0);
    if (button_pressed)
      mtime_last = mtime();
    if (mtime() - mtime_last > 1500) {
      sys_state = SYS_INIT;
    }
    break;
  }

}



int main(void)
{
	board_setup();
        //ltc6804_init();

        gui_setup();
        
	while (1) {
          can_process_rx();
          //          handle_sys();
          gui_update();
	}

	return 0;
}
