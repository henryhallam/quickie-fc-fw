#ifndef LEDS_H
#define LEDS_H

typedef enum {LED_OFF, LED_RED, LED_YELLOW, LED_GREEN} led_color_e;
typedef enum {LED_BATT_L, LED_BATT_C, LED_BATT_R, LED_N} led_id_e;

void leds_setup(void);
void led_set(led_id_e led, led_color_e color);

#endif // LEDS_H
