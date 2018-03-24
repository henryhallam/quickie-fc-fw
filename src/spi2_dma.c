#include "spi2_dma.h"
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>

void spi2_dma_setup(void)
{
  /* SPI2 TX uses DMA controller 1 Stream 4 Channel 0. */
  /* SPI2 RX uses DMA controller 1 Stream 3 Channel 0. */
  /* Enable DMA1 clock and IRQ */
  dma_stream_reset(DMA1, DMA_STREAM4);
  nvic_enable_irq(NVIC_DMA1_STREAM4_IRQ);
  dma_set_priority(DMA1, DMA_STREAM4, DMA_SxCR_PL_MEDIUM);
  /*
    Need to configure memory and peripheral size (8 or 16 bit) and increment mode on a per-xfer basis, for transmit
  dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
  */
  dma_set_transfer_mode(DMA1, DMA_STREAM4,
                        DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  /* The register to target is the SPI2 data register */
  dma_set_peripheral_address(DMA1, DMA_STREAM4, (uint32_t) &SPI2_DR);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM4);
  dma_channel_select(DMA1, DMA_STREAM4, DMA_SxCR_CHSEL_0);

  dma_stream_reset(DMA1, DMA_STREAM3);
  nvic_enable_irq(NVIC_DMA1_STREAM3_IRQ);
  dma_set_priority(DMA1, DMA_STREAM3, DMA_SxCR_PL_MEDIUM);
  dma_set_memory_size(DMA1, DMA_STREAM3, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM3, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM3);
  dma_set_transfer_mode(DMA1, DMA_STREAM3,
                        DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  dma_set_peripheral_address(DMA1, DMA_STREAM3, (uint32_t) &SPI2_DR);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM3);
  dma_channel_select(DMA1, DMA_STREAM3, DMA_SxCR_CHSEL_0);

}

static volatile bool dma_tx_done = 0;
static volatile bool dma_rx_done = 0;
void dma1_stream4_isr(void) // Transmit
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM4, DMA_TCIF)) {
    dma_tx_done = 1;
    dma_clear_interrupt_flags(DMA1, DMA_STREAM4, DMA_TCIF);
  }
}

void dma1_stream3_isr(void) // Receive
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM3, DMA_TCIF)) {
    dma_rx_done = 1;
    dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_TCIF);
  }
}

void spi2_xfer_dma_8(const uint8_t *data_write, uint8_t *data_read, size_t n) {
  dma_disable_stream(DMA1, DMA_STREAM4);
  dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_8BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_8BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
  spi_set_dff_8bit(SPI2);
  // DMA can only transfer up to 65535 words at a time
  do {
    uint16_t n_seg = (n > 65535) ? 65535 : n;
    // Set up receive first
    dma_set_memory_address(DMA1, DMA_STREAM3, (uint32_t) data_read);
    dma_set_number_of_data(DMA1, DMA_STREAM3, n_seg);
    dma_rx_done = 0;
    dma_enable_stream(DMA1, DMA_STREAM3);
    spi_enable_rx_dma(SPI2);

    // Then set up transmit, which will start right away
    dma_set_memory_address(DMA1, DMA_STREAM4, (uint32_t) data_write);
    dma_set_number_of_data(DMA1, DMA_STREAM4, n_seg);
    dma_tx_done = 0;
    dma_enable_stream(DMA1, DMA_STREAM4);
    spi_enable_tx_dma(SPI2);
    
    while (!dma_tx_done || !dma_rx_done);  // TODO: Async
    spi_disable_tx_dma(SPI2);
    spi_disable_rx_dma(SPI2);
    data_read += n_seg;
    data_write += n_seg;
    n -= n_seg;
  } while (n > 0);
  while (SPI2_SR & SPI_SR_BSY);  // DMA done doesn't mean it's safe to release CS
}

void spi2_write_dma_16(const uint16_t *data, size_t n, bool increment) {
  dma_disable_stream(DMA1, DMA_STREAM4);
  if (increment)
    dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
  else
    dma_disable_memory_increment_mode(DMA1, DMA_STREAM4);
  dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_16BIT);
  spi_set_dff_16bit(SPI2);
  // DMA can only transfer up to 65535 words at a time
  do {
    uint16_t n_seg = (n > 65535) ? 65535 : n;
    dma_set_memory_address(DMA1, DMA_STREAM4, (uint32_t) data);
    dma_set_number_of_data(DMA1, DMA_STREAM4, n_seg);
    dma_tx_done = 0;
    dma_enable_stream(DMA1, DMA_STREAM4);
    spi_enable_tx_dma(SPI2);
    while (!dma_tx_done);  // TODO: Async
    spi_disable_tx_dma(SPI2);
    if (increment)
      data += n_seg;
    n -= n_seg;
  } while (n > 0);
  while (SPI2_SR & SPI_SR_BSY);  // DMA done doesn't mean it's safe to release CS
  spi_set_dff_8bit(SPI2);
}


