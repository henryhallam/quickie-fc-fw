#include "mc_telem.h"
#include "clock.h"

mc_telem_t mc_telem[N_MCS];

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
