#pragma once
#include <stdint.h>

int ltc6804_init(void);
uint32_t ltc6804_get_voltages(int n_chain, float *voltages);
int ltc6804_get_temps(int n_chain, float *temps);
void ltc6804_demo(void);
