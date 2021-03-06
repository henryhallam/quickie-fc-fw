/*
 * This include file describes the functions exported by clock.c
 */
#ifndef __CLOCK_H
#define __CLOCK_H

#include <stdint.h>

/*
 * Definitions for functions being abstracted out
 */
void msleep(uint32_t);
void usleep(uint32_t);
uint32_t mtime(void);
int utime_tock(int tick);
int utime_tick(void);
void clock_setup(void);

#endif /* generic header protector */

