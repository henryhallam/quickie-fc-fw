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


enum bringup_stage { SYS_OFF, SYS_PRECHARGE_LR, SYS_MAIN_CLOSING, SYS_ON, SYS_COOLDOWN };
enum fault_status { NORMAL, FAILED };
enum fault_action { CLEAR, FAULT, PROPAGATE };

extern int bringup_dirty, bringup_button_pressed;

void raise_fault(void);

extern struct bringup_ctx {
    enum bringup_stage  current_stage;

    enum fault_status   fault;
    enum bringup_stage  failed_stage;

    uint32_t            mtime_last_transition;
} bringup;

const char * bringup_error(const struct bringup_ctx *bup);
const char * bringup_ctx_string(const struct bringup_ctx *bup);

#endif //BOARD_H
