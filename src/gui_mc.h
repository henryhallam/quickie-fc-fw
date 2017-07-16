#ifndef GUI_MC__H
#define GUI_MC__H

#include <stdint.h>

struct motor_fault_bits {
  uint16_t uvlo:1;
  uint16_t ovlo:1;
  uint16_t over_temp:1;
  uint16_t hw_fault:1;
  uint16_t oclo:1;
  uint16_t overspeed:1;
  uint16_t rsvd:4; //Warnings below
  uint16_t comms_warn:1; //At the moment this is a non-latching indication of deadman error
  uint16_t deadman:1;
  uint16_t can:4;
}; 
typedef union motor_fault_t {
  uint16_t all;
  struct motor_fault_bits flags;
} motor_fault_t;

typedef enum motor_state_e {
  KH_Init = 0, //Init HW, Auto to disabled
  KH_Fault, //Die a slow painful death
  KH_Disabled, //Enable COMMs, Wait for Enable command
  KH_Ready, //Run Observer, Wait for non-zero Torque command, check RPM state
  KH_SoftStart, //Enable PWM at Zero, Auto to ISR wait
  KH_SoftStartISR, //Ramp PWM to 50%, Auto to Torque
  KH_FlyingStart, //Do I need this with a Torque controller?
  KH_Torque, //Commanded Torque
  KH_Offsets, //Do an offset calibration
  KH_Speed, //Run simple PID speed controller
  KH_N_STATES
} motor_state_e;

typedef struct {
  // TODO: Temperatures
  float bus_v;
  float bus_i;
  float torque;
  float rpm;
  float mod_index;
  float cmd;
  motor_fault_t faults;
  motor_state_e state;
  uint32_t age;
} mc_gui_data_t;

typedef struct {
  int x;
  int y;
} gui_mc;

#define GUI_MC_W 140
#define GUI_MC_H 210

int gui_mc_init(gui_mc *handle, int x, int y);
void gui_mc_update(gui_mc *handle, const mc_gui_data_t *data);

#endif // GUI_MC__H
