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
        
        gui_mc gui_mc_l, gui_mc_r;
        gui_mc_init(&gui_mc_l, 0, 0);
        gui_mc_init(&gui_mc_r, LCD_W - GUI_MC_W, 0);

        mc_gui_data_t mc_data_l = {320.0, 22.2, 30, 4850, 0.4, 0.77, {.all = 0}, KH_Disabled, 0};
        mc_gui_data_t mc_data_r = {321.0, 21.3, 33, 4850, 0.39, 0.76, {.all = 0x3D}, KH_Torque, 0};

        int i = 0;
        int do_relay_flip = 0;

        int view_mode = 1;
        lcd_clear();
        
	while (1) {

          i++;
          if (i == 33) {
            i = 0;
            mc_data_l.state++;
            if (mc_data_l.state >= KH_N_STATES)
              mc_data_l.state = 0;
          }
          mc_data_r.age = mtime() / 1000;

          //uint32_t t0 = mtime();
          switch(view_mode) {
          case 0:
            gui_mc_update(&gui_mc_l, &mc_data_l);
            gui_mc_update(&gui_mc_r, &mc_data_r);
            //          uint32_t t_refresh = mtime() - t0;

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
