#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include <stdbool.h>

#define CAN_ID_SIC_L 0x12
#define CAN_ID_SIC_R 0x11

int can_setup(void);
void can_process_rx(void);
void can_tx(uint32_t id, uint8_t len, uint8_t *data);
void can_show_debug(void);

typedef struct {
  uint32_t rx_time;
  uint32_t id, fmi;
  uint8_t data[8] __attribute__ ((aligned (4)));
  uint8_t length;
  bool ext, rtr, full;
} can_sw_mbx_t;

typedef struct {
  int rx_count;
  int emerg_count;
} can_stats_t;

extern volatile can_stats_t can_stats;
#endif // CAN_H
