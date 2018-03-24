#include "sdcard.h"
#include "spi2_dma.h"
#include "clock.h"
#include <string.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>

#define ASSERT_SD_CS gpio_clear(GPIOB, GPIO5)
#define RELEASE_SD_CS gpio_set(GPIOB, GPIO5)


static uint8_t sd_crc7(uint8_t* chr,uint8_t cnt,uint8_t crc){ 
     uint8_t i, a; 
     uint8_t data; 

     for(a = 0; a < cnt; a++){           
          data = chr[a];           
          for(i = 0; i < 8; i++){                
               crc <<= 1; 
               if( (data & 0x80) ^ (crc & 0x80) ) {crc ^= 0x09;}                
               data <<= 1; 
          } 
     }      
     return crc & 0x7F; 
} 

static uint8_t sd_command(uint8_t cmd, uint32_t param) {
  uint8_t buf[7] = {0xFF, cmd | 0x40, param >> 24, param >> 16, param >> 8, param};
  buf[6] = (sd_crc7(&buf[1], 5, 0) << 1) | 1;

  ASSERT_SD_CS;
  spi2_xfer_dma_8(buf, buf, 7);

  // Wait for non-FF response
  buf[0] = 0xFF;
  int timeout = 128;
  while (buf[0] == 0xFF && timeout--)
    spi2_xfer_dma_8(buf, buf, 1);

  RELEASE_SD_CS;
  return buf[0];
}

//void spi2_xfer_dma_8(const uint8_t *data_write, uint8_t *data_read, size_t n)



int sdcard_setup(void) {
  spi_disable(SPI2);
  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_256,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_8BIT,
                  SPI_CR1_MSBFIRST);
  spi_enable(SPI2);

    // Send some slow clocks *without* asserting CS, to give card time to init
  uint8_t buf[20];
  memset(buf, 0xFF, sizeof(buf));
  spi2_xfer_dma_8(buf, buf, sizeof(buf));

  int ret = sd_command(0x0, 0);  // hopefully 1

  spi_disable(SPI2);
  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_2,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_8BIT,
                  SPI_CR1_MSBFIRST);
  spi_enable(SPI2);
  return ret;
}
