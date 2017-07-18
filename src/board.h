#ifndef BOARD_H
#define BOARD_H

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


#endif //BOARD_H
