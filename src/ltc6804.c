#include "ltc6804.h"
#include "clock.h"
#include "font.h"
#include "lcd.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>

#define CHAIN_MAX 27

#define V_TARGET 3.502

// Command codes from datasheet Table 34
#define WRCFG   0x001
#define RDCFG   0x002
#define RDCVA   0x004
#define RDCVB   0x006
#define RDCVC   0x008
#define RDCVD   0x00A
#define RDAUXA  0x00C
#define RDAUXB  0x00E
#define RDSTATA 0x010
#define RDSTATB 0x012
#define ADCV    0x260
#define ADOW    0x228
#define CVST    0x207
#define ADAX    0x460
#define AXST    0x407
#define ADSTAT  0x468
#define STATST  0x40F
#define ADCVAX  0x46F
#define CLRCELL 0x711
#define CLRAUX  0x712
#define CLRSTAT 0x713
#define PLADC   0x714
#define DIAGN   0x715
#define WRCOMM  0x721
#define RDCOMM  0x722
#define STCOMM  0x723
#define AD_DCP  0x010
#define AD_PUP  0x040
#define AD_MD01 0x080
#define AD_MD10 0x100
#define AD_MD11 0x180
#define ST01    0x020
#define ST10    0x040
#define CH_ALL  0
#define CH_1_7  1
#define CH_2_8  2
#define CH_3_9  3
#define CH_4_10 4
#define CH_5_11 5
#define CH_6_12 6
#define CHG_ALL 0
#define CHG_GPIO1 1
#define CHG_GPIO2 2
#define CHG_GPIO3 3
#define CHG_GPIO4 4
#define CHG_GPIO5 5
#define CHG_REF2  6
#define CHST_ALL  0
#define CHST_SOC  1
#define CHST_ITMP 2
#define CHST_VA   3
#define CHST_VD   4

#define ASSERT_LTC6820_CS gpio_clear(GPIOA, GPIO4)
#define RELEASE_LTC6820_CS gpio_set(GPIOA, GPIO4)
#define LOG_INFO(fmt, ...) lcd_printf(LCD_WHITE, &RobotoCondensed_Regular7pt7b, fmt "\n", __VA_ARGS__)
#define LOG_WARN(fmt, ...) lcd_printf(LCD_RED, &RobotoCondensed_Regular7pt7b, fmt "\n", __VA_ARGS__)

typedef struct {
  union {
    uint8_t reg8[6];
    uint16_t reg16[3];
  };
  bool pec_ok;
} reg_group_t;

static uint16_t pec15Table[256];
static uint16_t CRC15_POLY = 0x4599;
static void init_PEC15_Table(void) {
  for (int i = 0; i < 256; i++) {
      uint16_t remainder = i << 7;
      for (int bit = 8; bit > 0; --bit)
        {
          if (remainder & 0x4000)
            {
              remainder = ((remainder << 1));
              remainder = (remainder ^ CRC15_POLY);
            }
          else
            {
              remainder = ((remainder << 1));
            }
        }
      pec15Table[i] = remainder&0xFFFF;
    }
}
static uint16_t pec15 (uint8_t *data , int len)
{
  uint16_t remainder, address;
  remainder = 16; //PEC seed
  for (int i = 0; i < len; i++)
    {
      address = ((remainder >> 7) ^ data[i]) & 0xff; //calculate PEC table address
      remainder = (remainder << 8 ) ^ pec15Table[address];
    }
  return (remainder*2); // The CRC15 has a 0 in the LSB so the final value must be multiplied by 2
}

static void ltc6804_wakeup(void) {
  for (int i = 0; i < CHAIN_MAX; i++) {
    ASSERT_LTC6820_CS;
    usleep(2);
    RELEASE_LTC6820_CS;
    usleep(22);
    msleep(1);
  }
  
  
}

static void ltc_spi_dma_setup(void)
{
  /* SPI3 uses DMA controller 1 Stream 5 Channel 0 for TX, Stream 2 Channel 0 for RX */
  dma_stream_reset(DMA1, DMA_STREAM5);
  nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
  dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_MEDIUM);
  dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
  dma_set_transfer_mode(DMA1, DMA_STREAM5,
                        DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  /* The register to target is the SPI3 data register */
  dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t) &SPI3_DR);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
  dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_0);

  dma_stream_reset(DMA1, DMA_STREAM2);
  nvic_enable_irq(NVIC_DMA1_STREAM2_IRQ);
  dma_set_priority(DMA1, DMA_STREAM2, DMA_SxCR_PL_MEDIUM);
  dma_set_memory_size(DMA1, DMA_STREAM2, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM2, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM2);
  dma_set_transfer_mode(DMA1, DMA_STREAM2,
                        DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  dma_set_peripheral_address(DMA1, DMA_STREAM2, (uint32_t) &SPI3_DR);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM2);
  dma_channel_select(DMA1, DMA_STREAM2, DMA_SxCR_CHSEL_0);

}

static volatile bool dma_tx_done = 0;
static volatile bool dma_rx_done = 0;
void dma1_stream5_isr(void)  // Transmit
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
    dma_tx_done = 1;
    dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
  }
}
void dma1_stream2_isr(void)  // Receive
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM2, DMA_TCIF)) {
    dma_rx_done = 1;
    dma_clear_interrupt_flags(DMA1, DMA_STREAM2, DMA_TCIF);
  }
}

static void spi_xfer_dma(uint16_t n, const uint8_t *tx_buf, uint8_t *rx_buf) {
  dma_set_memory_address(DMA1, DMA_STREAM2, (uint32_t) rx_buf);
  dma_set_number_of_data(DMA1, DMA_STREAM2, n);
  dma_rx_done = 0;
  dma_enable_stream(DMA1, DMA_STREAM2);
  spi_enable_rx_dma(SPI3);

  dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t) tx_buf);
  dma_set_number_of_data(DMA1, DMA_STREAM5, n);
  dma_tx_done = 0;
  dma_enable_stream(DMA1, DMA_STREAM5);
  spi_enable_tx_dma(SPI3);
  while (!dma_tx_done || !dma_rx_done || (SPI3_SR & SPI_SR_BSY));  // TODO: Async
  spi_disable_tx_dma(SPI3);
  spi_disable_rx_dma(SPI3);
}


static int ltc6804_chat(uint16_t cc, uint8_t is_write, uint8_t n_chain, reg_group_t *data) {
  /* Static buffer NOT in CCM */
  static uint8_t dma_buffer[2+2+(6+2)*CHAIN_MAX] = {0};


  dma_buffer[0] = (cc & 0x0700) >> 8;
  dma_buffer[1] = cc & 0xFF;
  uint16_t pec = pec15(dma_buffer, 2);
  dma_buffer[2] = pec >> 8;
  dma_buffer[3] = pec & 0xFF;
  if (is_write) {
    for (int i = 0; i < n_chain; i++) {
      memcpy(&dma_buffer[4+i*(6+2)], &data[i].reg8, 6);
      pec = pec15(&dma_buffer[4+i*(6+2)], 6);
      dma_buffer[4+i*(6+2)+6] = pec >> 8;
      dma_buffer[4+i*(6+2)+6+1] = pec & 0xFF;
    }
  }

#ifdef DEBUG_CHAT
  LOG_INFO("tx: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           dma_buffer[0], dma_buffer[1], dma_buffer[2], dma_buffer[3], dma_buffer[4], 
           dma_buffer[5], dma_buffer[6], dma_buffer[7], dma_buffer[8], dma_buffer[9]);
#endif
  ASSERT_LTC6820_CS;
  usleep(1);
  spi_xfer_dma(4+n_chain*(6+2), dma_buffer, dma_buffer);
  usleep(1);
  RELEASE_LTC6820_CS;
#ifdef DEBUG_CHAT
  LOG_INFO("rx: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           dma_buffer[0], dma_buffer[1], dma_buffer[2], dma_buffer[3], dma_buffer[4], 
           dma_buffer[5], dma_buffer[6], dma_buffer[7], dma_buffer[8], dma_buffer[9]);
#endif
  int pec_fail = 0;
  if (!is_write) {
    for (int i = 0; i < n_chain; i++) {
      if (data)
        memcpy(&data[i].reg8, &dma_buffer[4+i*(6+2)], 6);
      pec = pec15(&dma_buffer[4+i*(6+2)], 6);
      if (pec == ((dma_buffer[4+i*(6+2)+6] << 8) | dma_buffer[4+i*(6+2)+6+1])) {
        data[i].pec_ok = 1;
      } else {
        data[i].pec_ok = 0;
        pec_fail = 1;
      }
    }
  }
#ifdef DEBUG_CHAT
  if (pec_fail)
    LOG_WARN("PEC fail %d", pec_fail);
#endif
  return pec_fail;
}

/*
static void lightup_chain(uint8_t mask) {
  uint8_t regs[3][6] = {0};
  for (int i = 0; i < 3; i++)
    if (mask & (1<<i))
      regs[i][0] = 0x02;
    else
      regs[i][0] = 0xFA;
  
  ltc6804_wakeup();
  ltc6804_wakeup();
  ltc6804_wakeup();
  ltc6804_chat(1, 1, 3, (uint8_t *)regs);
}
*/

int ltc6804_get_voltages(int n_chain) {
  uint16_t mask = 0;
  ltc6804_wakeup();

  ltc6804_chat(ADCV | AD_MD10 | CH_ALL, 0, n_chain, NULL);
  msleep(3);
  reg_group_t cv_regs[4][CHAIN_MAX];  // Each chain has 4 register groups, each with 3 16-bit values
  ltc6804_chat(RDCVA, 0, n_chain, cv_regs[0]);
  ltc6804_chat(RDCVB, 0, n_chain, cv_regs[1]);
  ltc6804_chat(RDCVC, 0, n_chain, cv_regs[2]);
  ltc6804_chat(RDCVD, 0, n_chain, cv_regs[3]);

  float v_sum = 0;
  float v_min = INFINITY;
  float v_max = 0;
  
  for (int i = 0; i < n_chain; i++) {
    char s[222];
    char *p = s;
    p += sprintf(p, "Pack %d:\n", i);
    for (int j = 0; j < 12; j++) {
      float v = cv_regs[j/3][i].reg16[j%3] * 100e-6;
      v_sum += v;
      if (v < v_min)
        v_min = v;
      if (v > v_max)
        v_max = v;
      p += sprintf(p, "%.3f ", v);

      // Crappy autobalance
      if (v > V_TARGET)
        mask |= 1<<j;
        
    }
    LOG_INFO("%s", s);
  }
  LOG_INFO("Sum = %.3f   Min, Mean, Max = %.3f, %.3f, %.3f",
           v_sum, v_min, v_sum/(12*n_chain), v_max);

  return mask;
}

int ltc6804_get_temps(int n_chain) {
  ltc6804_chat(ADAX | AD_MD10 | CHG_ALL, 0, n_chain, NULL);
  msleep(3);
  reg_group_t av_regs[3][CHAIN_MAX];  // Each chain has 4 register groups, each with 3 16-bit values
  ltc6804_chat(RDAUXA, 0, n_chain, av_regs[0]);
  ltc6804_chat(RDAUXB, 0, n_chain, av_regs[1]);
  ltc6804_wakeup();
  ltc6804_chat(ADSTAT | AD_MD10 | CHST_ITMP, 0, n_chain, NULL);
  msleep(1);
  ltc6804_wakeup();
  ltc6804_chat(RDSTATA, 0, n_chain, av_regs[2]);

  for (int i = 0; i < n_chain; i++) {
    char s[222];
    char *p = s;
    p += sprintf(p, "Pack %d Temps: ", i);
    for (int j = 0; j < 5; j++) {
      float v = av_regs[j/3][i].reg16[j%3] * 100e-6;
      float r = 3.0*10e3 / v - 10e3;
      float t = 1/(1/(273.0+25) + 1/3428.0 * logf(r/10e3)) - 273;
      p += sprintf(p, "%.1f ", t);
    }
    float t = av_regs[2][i].reg16[1] * 100e-6 / 7.5e-3 - 273;
    p += sprintf(p, "%.1f", t);
    LOG_INFO("%s", s);
  }

  return 0;
}

static void ltc6804_dischg(uint16_t mask, uint8_t led) {
  int n_chain = 1;
  reg_group_t cfgr = {.reg8={
      0xF0 | (led ? 0x00 : 0x08), // Enable GPIO1 pulldown to light LED
                0, 0, 0, // Don't care about overvolt/undervolt flags
                mask & 0x00FF,
      (mask & 0x0F00) >> 8}};
  ltc6804_chat(WRCFG, 1, n_chain, &cfgr);
}


int ltc6804_init(void) {
  init_PEC15_Table();
  ltc_spi_dma_setup();

  // Probe to find out how many devices are in the chain
  ltc6804_wakeup();
  reg_group_t statb[CHAIN_MAX];
  ltc6804_chat(RDSTATB, 0, CHAIN_MAX, statb);
  int n_pec_ok = 0;
  for (int i = 0; i < CHAIN_MAX; i++) {
    n_pec_ok += statb[i].pec_ok;
  }
  return n_pec_ok;
}


void ltc6804_demo() {

  lcd_textbox_prep(0, 0, 480, 83, LCD_BLACK); 

  ltc6804_wakeup();
  ltc6804_get_temps(1);
  ltc6804_wakeup();
  uint16_t mask = ltc6804_get_voltages(1);
  ltc6804_wakeup();
  ltc6804_dischg(0x000,0);
  msleep(200);
  ltc6804_wakeup();
  LOG_INFO("Discharge: 0x%03X", mask);
  if (mask)
    ltc6804_dischg(mask, 1);
  msleep(500);

  lcd_textbox_show();
  /*
  ltc6804_chat(2, 0, 1, regs);
  LOG_INFO("response = %02X %02X %02X %02X %02X %02X",
           regs[0], regs[1], regs[2], regs[3], regs[4], regs[5]);
  */
}
