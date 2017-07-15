#include "board.h"
#include "leds.h"
#include "lcd.h"

int main(void)
{
	board_setup();
        
        led_set(LED_BATT_L, LED_RED);
        led_set(LED_BATT_C, LED_YELLOW);
        led_set(LED_BATT_R, LED_GREEN);

	while (1) {
          for (int i = 0; i < 2000000; i++) { /* Wait a bit. */
            __asm__("nop");
          }
          //          lcd_demo();
	}

	return 0;
}
