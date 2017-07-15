#include "board.h"
#include "clock.h"
#include "leds.h"
#include "lcd.h"
#include <libopencm3/cm3/systick.h>
#include <stdio.h>


int main(void)
{
	board_setup();

	while (1) {
          led_set(LED_BATT_L, LED_RED);
          led_set(LED_BATT_C, LED_RED);
          led_set(LED_BATT_R, LED_RED);
          lcd_demo();
          led_set(LED_BATT_L, LED_OFF);
          led_set(LED_BATT_C, LED_OFF);
          led_set(LED_BATT_R, LED_OFF);
          lcd_demo();
	}

	return 0;
}
