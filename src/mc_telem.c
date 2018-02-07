#include "mc_telem.h"
#include "clock.h"
#include "can.h"

mc_telem_t mc_telem[N_MCS];

static const uint8_t can_id_by_mc_id[2] = {
  [MC_LEFT] = CAN_ID_SIC_L,
  [MC_RIGHT] = CAN_ID_SIC_R};

void mc_telem_parse_pdo1(const uint8_t *data, mc_id_e mc_id) {
  mc_telem_t *mct = &mc_telem[mc_id];
  mct->rpm = *(float *)&data[0] * 1000;  // sent in krpm
  mct->temp_SiC = *(int16_t *)&data[6] * 0.1;  // Sent in deci-degC; not sure what happens when negative
  mct->faults.all = *(uint16_t *)&data[4];
  mct->mtime_rx = mtime(); // TODO: how to handle age of multi-part messages?
}

void mc_telem_parse_pdo2(const uint8_t *data, mc_id_e mc_id) {
  mc_telem_t *mct = &mc_telem[mc_id];
  mct->Iq = *(float *)&data[0];
  mct->bus_v = *(uint32_t *)&data[4] * 0.1; // sent in decivolts
  mct->torque = mct->Iq;  // TODO: Actual torque
}

void mc_telem_parse_pdo3(const uint8_t *data, mc_id_e mc_id) {
  mc_telem_t *mct = &mc_telem[mc_id];
  mct->temp_motor = *(int16_t *)&data[6] * 0.1;  // Sent in deci-degC; not sure what happens when negative
  mct->state      = data[4];
  mct->cmd        = *(float *)&data[0];
}

void mc_cmd_torque(int enable, float torque, mc_id_e mc_id) {
  // Command to override torque and system enable
  struct motor_command {
    uint8_t     enable;
    uint8_t     speedctrl;
    uint16_t    PAD16;
    float       command_ref;
  } cmd;
  cmd.enable = enable;
  cmd.speedctrl = 0;
  cmd.PAD16 = 0xbeef;
  cmd.command_ref = torque;
  can_tx(0x200 | can_id_by_mc_id[mc_id], 8, (uint8_t *) &cmd);
}
