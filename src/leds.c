#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>

#include "leds.h"

#define PERIOD 65535
#define DUTY_YELLOW 0.91  // The red is a lot brighter so we have to bias heavily toward green
void leds_setup(void)
{
  /* Timer 8 for LEDs */
  rcc_periph_clock_enable(RCC_TIM8);

  timer_reset(TIM8);

  /* Set Timer global mode:
   * - No division
   * - Alignment centre mode 1 (up/down counting, interrupt on downcount only)
   * - Direction up (when centre mode is set it is read only, changes by hardware)
   */
  timer_set_mode(TIM8, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_CENTER_1, TIM_CR1_DIR_UP);

  /* Set Timer output compare mode:
   * - Channel 1
   * - PWM mode 2 (output low when CNT < CCR1, high otherwise)
   */
  timer_enable_break_main_output(TIM8);

  /* The ARR (auto-preload register) sets the PWM period to 62.5kHz from the
     72 MHz clock.*/
  timer_enable_preload(TIM8);
  timer_set_period(TIM8, PERIOD);

  /* The CCR1 (capture/compare register 1) sets the PWM duty cycle to default 50% */
  timer_enable_oc_preload(TIM8, TIM_OC1);
  timer_set_oc_value(TIM8, TIM_OC1, PERIOD*DUTY_YELLOW);
  timer_enable_oc_preload(TIM8, TIM_OC2);
  timer_set_oc_value(TIM8, TIM_OC2, PERIOD*DUTY_YELLOW);
  timer_enable_oc_preload(TIM8, TIM_OC3);
  timer_set_oc_value(TIM8, TIM_OC3, PERIOD*DUTY_YELLOW);

  /* Force an update to load the shadow registers */
  timer_generate_event(TIM8, TIM_EGR_UG);

  /* Start the Counter. */
  timer_enable_counter(TIM8);
}

void led_set(led_id_e led, led_color_e color)
{
  static const enum tim_oc_id led_tim_oc_ids[] = {TIM_OC1, TIM_OC2, TIM_OC3};
  static const enum tim_oc_id led_tim_oc_n_ids[] = {TIM_OC1N, TIM_OC2N, TIM_OC3N};
  
  if (led >= LED_N) return;

  enum tim_oc_id tim_oc = led_tim_oc_ids[led];
  enum tim_oc_id tim_oc_n = led_tim_oc_n_ids[led];

  if (color == LED_OFF) {
    timer_disable_oc_output(TIM8, tim_oc);
    timer_disable_oc_output(TIM8, tim_oc_n);
  } else {
    timer_enable_oc_output(TIM8, tim_oc);
    timer_enable_oc_output(TIM8, tim_oc_n);
    switch(color) {
    case LED_RED:
      timer_set_oc_mode(TIM8, tim_oc, TIM_OCM_FORCE_HIGH);
      break;
    case LED_YELLOW:
      timer_set_oc_mode(TIM8, tim_oc, TIM_OCM_PWM2);
      break;
    case LED_GREEN:
      timer_set_oc_mode(TIM8, tim_oc, TIM_OCM_FORCE_LOW);
      break;
    default:
      break;
    }
  }
}
