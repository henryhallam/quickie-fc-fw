#ifndef LTC6803_H
#define LTC6803_H

int ltc6804_init(void);
int ltc6804_get_voltages(int n_chain, float *voltages);
int ltc6804_get_temps(int n_chain, float *temps);
void ltc6804_demo(void);

#endif // LTC6804_H
