#ifndef CAN_H
#define CAN_H

int can_setup(void);
void can_show_debug(void);

extern volatile int can_rx_count;
extern volatile int can_rx_emerg;
#endif // CAN_H
