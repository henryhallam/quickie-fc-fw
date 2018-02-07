#include "board.h"
#include "can.h"
#include "clock.h"
#include "font.h"
#include "gui.h"
#include "gui_bat.h"
#include "lcd.h"
#include "leds.h"
#include "ltc6804.h"
#include "mc_telem.h"
#include "touch.h"
#include "usb.h"
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/gpio.h>
#include <stdlib.h>
#include <string.h>


#define BUS_VOLTAGE_GOOD (300.0)

struct bringup_ctx bringup;

const char * bringup_error(const struct bringup_ctx *bup) {
    if (NORMAL == bup->fault) {
        return "NO FAULT";
    }

    switch (bup->failed_stage) {
        case SYS_PRECHARGE_LR:
            return "PRECHARGE";
            break;
        case SYS_MAIN_CLOSING:
            return "MAIN SWITCH";
            break;
        case SYS_ON:
            return "RUNTIME";
            break;
        default:
            return "XXX";
            break;
    }
}

const char * bringup_ctx_string(const struct bringup_ctx *bup) {
    switch (bup->current_stage) {
        case SYS_OFF:
            return "START";
            break;
        case SYS_PRECHARGE_LR:
            return "PRECHARGING";
            break;
        case SYS_MAIN_CLOSING:
            return "MAIN SWITCH";
            break;
        case SYS_ON:
            return "STOP";
            break;
        case SYS_COOLDOWN:
            return "COOLDOWN";
            break;
        default:
            return NULL;
            break;
    }
}

static void advance(struct bringup_ctx *ctx, enum bringup_stage new_stage, enum fault_action action) {
    switch (action) {
        case CLEAR:
            ctx->fault = NORMAL;
            break;
        case FAULT:
            ctx->fault= FAILED;
            ctx->failed_stage = ctx->current_stage;
            break;
        case PROPAGATE:
            break;
    }

    ctx->current_stage          = new_stage;
    ctx->mtime_last_transition  = mtime();

    bringup_dirty = 1;
}

void raise_fault() {
    advance(&bringup, SYS_COOLDOWN, FAULT);
}

static void handle_sys(void) {

  switch(bringup.current_stage) {
  case SYS_OFF:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R,   0);
    relay_ctl(RLY_FW_L,   0);

    relay_ctl(RLY_DIST_L,   0);
    relay_ctl(RLY_FUEL_R,   0);

    if (bringup_button_pressed) {
      bringup_button_pressed = 0;
      advance(&bringup, SYS_PRECHARGE_LR, CLEAR);
    }
    break;

  case SYS_PRECHARGE_LR:
    relay_ctl(RLY_PRECHG, 1);
    relay_ctl(RLY_FW_R,   0);
    relay_ctl(RLY_FW_L,   0);
    relay_ctl(RLY_DIST_L,   0);
    relay_ctl(RLY_FUEL_R,   0);

    if (mtime() - mc_telem[MC_RIGHT].mtime_rx < 100
        && mc_telem[MC_RIGHT].bus_v > BUS_VOLTAGE_GOOD
        && mtime() - mc_telem[MC_LEFT].mtime_rx < 100
        && mc_telem[MC_LEFT].bus_v > BUS_VOLTAGE_GOOD) {
      advance(&bringup, SYS_MAIN_CLOSING, CLEAR);
    }


    if (mtime() - bringup.mtime_last_transition > 50000) { // Precharge failed
      advance(&bringup, SYS_COOLDOWN, FAULT);
    }

    break;
  case SYS_MAIN_CLOSING:
    relay_ctl(RLY_PRECHG, 1);
    relay_ctl(RLY_FW_R,   1);
    relay_ctl(RLY_FW_L,   1);
    relay_ctl(RLY_DIST_L,   1);
    relay_ctl(RLY_FUEL_R,   1);

    if (mtime() - bringup.mtime_last_transition > 50 // ensure main contactor has time to close
        && mtime() - mc_telem[MC_RIGHT].mtime_rx < 100  // recent data
        && mc_telem[MC_RIGHT].bus_v > BUS_VOLTAGE_GOOD           // bus still hot
        && mtime() - mc_telem[MC_LEFT].mtime_rx < 100  // recent data
        && mc_telem[MC_LEFT].bus_v > BUS_VOLTAGE_GOOD) {          // bus still hot
      advance(&bringup, SYS_ON, CLEAR);
    }



    if (mtime() - bringup.mtime_last_transition > 500) { // Precharge failed
      advance(&bringup, SYS_COOLDOWN, FAULT);
    }



    break;
  case SYS_ON:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R,   1);
    relay_ctl(RLY_FW_L,   1);
    relay_ctl(RLY_DIST_L,   1);
    relay_ctl(RLY_FUEL_R,   1);

    if ( mtime() - mc_telem[MC_RIGHT].mtime_rx > 500        // lost comm
        || mc_telem[MC_RIGHT].bus_v < BUS_VOLTAGE_GOOD   // or low voltage
        || mtime() - mc_telem[MC_LEFT].mtime_rx > 500        // lost comm
        || mc_telem[MC_LEFT].bus_v < BUS_VOLTAGE_GOOD) {   // or low voltage
	    advance(&bringup, SYS_COOLDOWN, FAULT);
    }
    if(bringup_button_pressed == 1) {
	    bringup_button_pressed = 0;
	    advance(&bringup, SYS_COOLDOWN, CLEAR);
    }

    break;

  case SYS_COOLDOWN:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R,   0);
    relay_ctl(RLY_FW_L,   0);
    relay_ctl(RLY_DIST_L,   0);
    relay_ctl(RLY_FUEL_R,   0);

    if (mtime() - bringup.mtime_last_transition > 1500) {
      advance(&bringup, SYS_OFF, PROPAGATE);
    }
    break;
  }

}

struct motor_command_t {
    uint8_t     enable;
    uint8_t     speedctrl;
    uint16_t    PAD16;
    float       command_ref;
};

void can_torque_r(int enable, float torque) {
    struct motor_command_t foo;
    foo.enable = enable;
    foo.speedctrl = 0;
    foo.PAD16 = 0xbeef;
    foo.command_ref = torque;
    can_transmit(CAN1, 0x211, 0, 0, 8, (uint8_t *) &foo);

}

void can_torque_l(int enable, float torque) {
    struct motor_command_t foo;
    foo.enable = enable;
    foo.speedctrl = 0;
    foo.PAD16 = 0xbeef;
    foo.command_ref = torque;
    can_transmit(CAN1, 0x212, 0, 0, 8, (uint8_t *) &foo);

}

int main(void)
{
	board_setup();
        packs_talking = ltc6804_init();

        gui_setup();
	while (1) {
          can_process_rx();
          usb_poll();
          handle_sys();
          gui_update();
	}

	return 0;
}
