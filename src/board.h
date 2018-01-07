#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>
#define SYSCLK_HZ 168000000

void board_setup(void);

typedef enum {
  RLY_DIST_L = 1,
  RLY_FW_L = 2,
  RLY_PRECHG = 3,
  RLY_FW_R = 4,
  RLY_FUEL_R = 5
} relay_e;

void relay_ctl(relay_e relay, int state);


enum bringup_stage { SYS_OFF, SYS_PRECHG_1, SYS_PRECHG_2, SYS_ON, SYS_COOLDOWN };

extern int bringup_dirty, bringup_button_pressed;

extern struct bringup_ctx {
        enum bringup_stage stage;
	int                ok;
	uint32_t           mtime_last;
} bringup;

#endif //BOARD_H
