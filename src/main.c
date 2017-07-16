#include "board.h"
#include "clock.h"
#include "leds.h"
#include "font.h"
#include "gui_mc.h"
#include "lcd.h"
#include <libopencm3/cm3/systick.h>
#include <stdio.h>


static void banner(void) {
  lcd_textbox_prep(36, 80, 408, 80, LCD_PURPLE);
  lcd_printf(LCD_WHITE, &FreeSans18pt7b, "          Henrytronics");
  lcd_printf(LCD_WHITE, &FreeMonoBold9pt7b, "\nFlight logistical situation enhancer");
  lcd_printf(LCD_WHITE, &FreeSansOblique9pt7b, "\n                   (for his or her pleasure)");
  lcd_textbox_show();
  
  led_set(LED_BATT_L, LED_RED);
  led_set(LED_BATT_C, LED_YELLOW);
  led_set(LED_BATT_R, LED_GREEN);
  msleep(666);
  led_set(LED_BATT_L, LED_GREEN);
  led_set(LED_BATT_C, LED_RED);
  led_set(LED_BATT_R, LED_YELLOW);
  msleep(666);
  led_set(LED_BATT_L, LED_YELLOW);
  led_set(LED_BATT_C, LED_GREEN);
  led_set(LED_BATT_R, LED_RED);
  msleep(666);
  led_set(LED_BATT_L, LED_OFF);
  led_set(LED_BATT_C, LED_OFF);
  led_set(LED_BATT_R, LED_OFF);
  lcd_clear();
}

int main(void)
{
	board_setup();
        banner();
        
        lcd_textbox_prep(170, 220, 140, 50, LCD_BLUE);
        lcd_textbox_move_cursor(11, 39, 1);
        lcd_printf(LCD_WHITE, &FreeSans18pt7b, "START");
        lcd_textbox_show();

        gui_mc gui_mc_l, gui_mc_r;
        gui_mc_init(&gui_mc_l, 0, 0);
        gui_mc_init(&gui_mc_r, LCD_W - GUI_MC_W, 0);

        mc_gui_data_t mc_data_l = {320.0, 22.2, 30, 4850, 0.4, 0.77, {.all = 0}, KH_Disabled, 0};
        mc_gui_data_t mc_data_r = {321.0, 21.3, 33, 4850, 0.39, 0.76, {.all = 0x3D}, KH_Torque, 222};

	while (1) {
          gui_mc_update(&gui_mc_l, &mc_data_l);
          gui_mc_update(&gui_mc_r, &mc_data_r);
	}

	return 0;
}
