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
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/gpio.h>
#include <stdlib.h>
#include <string.h>

#define OK 1
#define NG 0


struct bringup_ctx bringup;

static void advance(struct bringup_ctx *ctx, enum bringup_stage new_stage, int status) {
    if (status == NG) {
        ctx->failed_stage = ctx->current_stage;
    }
    ctx->current_stage          = new_stage;
    ctx->failed                 = status;
    ctx->mtime_last             = mtime();

    bringup_dirty = 1;
}

static void handle_sys(void) {

  switch(bringup.stage) {
  case SYS_OFF:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R,   0);
    if (bringup_button_pressed) {
      bringup_button_pressed = 0;
      advance(&bringup, SYS_PRECHG_1, OK);
    }
    break;

  case SYS_PRECHG_1:
    relay_ctl(RLY_PRECHG, 1);
    relay_ctl(RLY_FW_R,   0);

    if (mtime() - mc_telem[MC_RIGHT].mtime_rx < 100
        && mc_telem[MC_RIGHT].bus_v > 300.0) {
      advance(&bringup, SYS_PRECHG_2, OK);
    }


    if (mtime() - bringup.mtime_last > 500) { // Precharge failed
      advance(&bringup, SYS_COOLDOWN, NG);
    }

    break;
  case SYS_PRECHG_2:
    relay_ctl(RLY_PRECHG, 1);
    relay_ctl(RLY_FW_R,   1);

    if (mtime() - bringup.mtime_last > 50 // ensure main contactor has time to close
        && mtime() - mc_telem[MC_RIGHT].mtime_rx < 100  // recent data
        && mc_telem[MC_RIGHT].bus_v > 300.0) {
      advance(&bringup, SYS_ON, OK);
    }



    if (mtime() - bringup.mtime_last > 500) { // Precharge failed
      advance(&bringup, SYS_COOLDOWN, NG);
    }



    break;
  case SYS_ON:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R,   1);
    if ( mtime() - mc_telem[MC_RIGHT].mtime_rx > 100  // or lost comm
        || mc_telem[MC_RIGHT].bus_v < 300.0) {  // or low voltage
	    advance(&bringup, SYS_COOLDOWN, NG);
    }
    if(bringup_button_pressed == 1) {
	    bringup_button_pressed = 0;
	    advance(&bringup, SYS_COOLDOWN, OK);
    }

    break;

  case SYS_COOLDOWN:
    relay_ctl(RLY_PRECHG, 0);
    relay_ctl(RLY_FW_R,   0);
    if (mtime() - bringup.mtime_last > 1500) {
      advance(&bringup, SYS_OFF, OK);
    }
    break;
  }

}



int main(void)
{
	board_setup();
        packs_talking = ltc6804_init();

        gui_setup();
        
	while (1) {
          can_process_rx();
          handle_sys();
          gui_update();
	}

	return 0;
}
