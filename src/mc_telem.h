#ifndef MC_TELEM_H
#define MC_TELEM_H

#include "can.h"
#include "gui_mc.h"

typedef enum {
  MC_LEFT,
  MC_RIGHT,
  N_MCS} mc_id_e;

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
  float bus_v;
  float bus_i;
  float torque;
  float rpm;
  float mod_index;
  float cmd;
  float Iq;
  float temp_SiC, temp_motor;
  motor_fault_t faults;
  motor_state_e state;
  uint32_t mtime_rx;
} mc_telem_t;

extern mc_telem_t mc_telem[N_MCS];

void mc_telem_parse_pdo1(const uint8_t *data, mc_id_e mc_id);
void mc_telem_parse_pdo2(const uint8_t *data, mc_id_e mc_id);
void mc_telem_parse_pdo3(const uint8_t *data, mc_id_e mc_id);


#endif // MC_TELEM_H
