#include "gui_fun.h"
#include "clock.h"
#include "font.h"
#include "lcd.h"
#include "leds.h"

const char *kittybutt = 
  "     |\n"
  "     |\n"
  "     |\n"
  "    / \\\n"
  "___| * |___\n"
  "   |/ \\|\n"
  "   || ||\n"
  "   || ||\n"
  "   \"\" \"\"\n";

void banner(void) {
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
