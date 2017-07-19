#include "board.h"
#include "can.h"
#include "clock.h"
#include "font.h"
#include "gui_fun.h"
#include "gui_mc.h"
#include "lcd.h"
#include "leds.h"
#include "touch.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <stdlib.h>

int main(void)
{
	board_setup();
        //        banner();
        //        touch_cal();

        lcd_textbox_prep(170, 220, 140, 50, LCD_BLUE);
        lcd_textbox_move_cursor(11, 38, 1);
        lcd_printf(LCD_WHITE, &FreeSans18pt7b, "START");
        lcd_textbox_show();

        lcd_textbox_prep(180, 22, 125, 155, LCD_BLACK);
        lcd_printf(LCD_GREEN, &FreeMono9pt7b, "%s", kittybutt);
        lcd_textbox_show();
        
        gui_mc_draw_frame();

        int do_relay_flip = 0;

        int view_mode = 0;
        
	while (1) {
          can_process_rx();
          switch(view_mode) {
          case 0:
            gui_mc_draw_data();
            break;
          case 1:  // CAN debug
            can_show_debug();
            break;
          }

          int touch_x, touch_y;
          if (touch_get(&touch_x, &touch_y)) {
            if (lcd_fb_prep(touch_x - 3, touch_y - 3, 7, 7, LCD_PURPLE))
              lcd_fb_show();
            while(touch_get(NULL, NULL))
              msleep(200);
            do_relay_flip = !do_relay_flip;
          }

          relay_ctl(RLY_PRECHG, do_relay_flip);

	}

	return 0;
}
