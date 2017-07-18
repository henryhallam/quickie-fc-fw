#include "board.h"
#include "clock.h"
#include "font.h"
#include "gui_fun.h"
#include "gui_mc.h"
#include "lcd.h"
#include "leds.h"
#include "touch.h"
#include <libopencm3/cm3/systick.h>
#include <stdio.h>

int main(void)
{
	board_setup();
        //        banner();

        touch_cal();
        
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

        int i;
	while (1) {
          i++;
          if (i == 33) {
            i = 0;
            mc_data_l.state++;
            if (mc_data_l.state >= KH_N_STATES)
              mc_data_l.state = 0;
          }
          mc_data_r.age = mtime() / 1000;
          gui_mc_update(&gui_mc_l, &mc_data_l);
          gui_mc_update(&gui_mc_r, &mc_data_r);
          int touch_x = 0, touch_y = 0, touch_detected = 0;
          touch_detected = touch_get(&touch_x, &touch_y);
          lcd_textbox_prep(0, LCD_H - 18, 220, 18, LCD_BLACK);
          lcd_printf(touch_detected ? LCD_GREEN : LCD_BLUE, &FreeMono9pt7b, "%5d %5d", touch_x, touch_y);
          lcd_textbox_show();
          if (touch_detected) {
            if (lcd_fb_prep(touch_x - 3, touch_y - 3, 7, 7, LCD_PURPLE))
              lcd_fb_show();
          }
	}

	return 0;
}
