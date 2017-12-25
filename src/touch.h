/*
  4-wire resistive touchscreen driver.
  Interrupt driven. Uses ADC and DMA.
  Somewhat confusingly, also monitors the low voltage batteries.
 */
#ifndef TOUCH_H
#define TOUCH_H

#include <stdint.h>

typedef struct {
  int16_t x, y, z;
  uint32_t mtime;
} touch_event_t;

void touch_setup(void);
int touch_get(int *x, int *y);
int touch_cal(void);
void touch_show_debug(void);
float get_lv_bus_v(void);
float get_9v_v(void);
#endif // TOUCH_H
