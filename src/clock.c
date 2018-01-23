/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2014 Chuck McManis <cmcmanis@mcmanis.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Now this is just the clock setup code from systick-blink as it is the
 * transferrable part.
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>

/* Common function descriptions */
#include "clock.h"

static const int sysclock_hz = 168000000;
static const int sysclock_mhz = 168;
static const uint32_t systick_reload = 168000;  // To get 1 kHz systick interrupt rate

/* milliseconds since boot */
static volatile uint32_t system_millis;

/* Called when systick fires */
void sys_tick_handler(void)
{
	system_millis++;
}


/* For measurement of short intervals: call tick to get a token, pass it to tock */
int utime_tick(void) {
  return (int)systick_get_value();
}

int utime_tock(int tick) {
  int t2 = systick_get_value();
  if (t2 < tick)
    return (tick - t2) / sysclock_mhz;
  return (tick - t2 + systick_reload) / sysclock_mhz;
}

/* busywait for delay microseconds */
void usleep(uint32_t delay)
{
  while (delay) {
    uint32_t delay_segment = (delay >= (systick_reload / 2 / sysclock_mhz)) ? (systick_reload / 2 / sysclock_mhz - 1) : delay;
    delay -= delay_segment;
    int32_t wake = - delay_segment * sysclock_mhz + 60;
    wake += systick_get_value();
    while (((systick_reload + (int32_t)systick_get_value() - wake) % systick_reload) < systick_reload / 2);
  }
}
/* simple sleep for delay milliseconds */
void msleep(uint32_t delay)
{
	uint32_t wake = system_millis + delay;
	while (wake > system_millis);
}

/* Getter function for the current time */
uint32_t mtime(void)
{
	return system_millis;
}

/*
 * clock_setup(void)
 *
 * This function sets up both the base board clock rate
 * and a 1khz "system tick" count. The SYSTICK counter is
 * a standard feature of the Cortex-M series.
 */
void clock_setup(void)
{
	/* Base board frequency (AHB), set to 168MHz.  APB1 = 42 MHz, APB2 = 84 MHz. */
	rcc_clock_setup_hse_3v3(&rcc_hse_25mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
        
	systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    STK_CVR = 0;
	systick_set_reload(systick_reload);
	systick_counter_enable();

	/* this done last */
        systick_interrupt_enable();
}
