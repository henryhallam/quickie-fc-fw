#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void spi2_dma_setup(void);
void spi2_write_dma_16(const uint16_t *data, size_t n, bool increment);
void spi2_xfer_dma_8(const uint8_t *data_write, uint8_t *data_read, size_t n);
void spi2_set_clock(void);
