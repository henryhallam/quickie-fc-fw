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
void update_touch_filter(void);
int touch_get_filtered(int *x, int *y);
void touch_show_debug(void);
#endif // TOUCH_H
