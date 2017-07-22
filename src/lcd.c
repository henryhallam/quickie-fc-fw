#include "lcd.h"
#include "font.h"
#include "clock.h"
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/spi.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_LCD_CS gpio_clear(GPIOB, GPIO12)
#define RELEASE_LCD_CS gpio_set(GPIOB, GPIO12)

#define ASSERT_LCD_DATA gpio_set(GPIOD, GPIO2)
#define RELEASE_LCD_DATA gpio_clear(GPIOD, GPIO2)

#define LCD_SPI SPI2

#define HX8357D 0xD
#define HX8357B 0xB

#define HX8357_TFTWIDTH  LCD_W
#define HX8357_TFTHEIGHT LCD_H

#define HX8357_NOP     0x00
#define HX8357_SWRESET 0x01
#define HX8357_RDDID   0x04
#define HX8357_RDDST   0x09

#define HX8357_RDPOWMODE  0x0A
#define HX8357_RDMADCTL  0x0B
#define HX8357_RDCOLMOD  0x0C
#define HX8357_RDDIM  0x0D
#define HX8357_RDDSDR  0x0F

#define HX8357_SLPIN   0x10
#define HX8357_SLPOUT  0x11

#define HX8357_INVOFF  0x20
#define HX8357_INVON   0x21
#define HX8357_DISPOFF 0x28
#define HX8357_DISPON  0x29

#define HX8357_CASET   0x2A
#define HX8357_PASET   0x2B
#define HX8357_RAMWR   0x2C
#define HX8357_RAMRD   0x2E

#define HX8357_TEON  0x35
#define HX8357_TEARLINE  0x44
#define HX8357_MADCTL  0x36
#define HX8357_COLMOD  0x3A

#define HX8357_SETOSC 0xB0
#define HX8357_SETPWR1 0xB1
#define HX8357_SETRGB 0xB3
#define HX8357D_SETCOM  0xB6

#define HX8357D_SETCYC  0xB4
#define HX8357D_SETC 0xB9

#define HX8357D_SETSTBA 0xC0
#define HX8357_SETPANEL  0xCC

#define HX8357D_SETGAMMA 0xE0

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

#define FB_MAX_PIXELS 40000
static uint16_t fb[FB_MAX_PIXELS];
static int fb_x = 0, fb_y = 0, fb_w = 0, fb_h = 0;

static void writecommand(uint8_t cmd) {
  ASSERT_LCD_CS;
  (void) spi_xfer(LCD_SPI, cmd);
  while (SPI2_SR & SPI_SR_BSY);  // DMA done doesn't mean it's safe to release CS
  RELEASE_LCD_CS;
}

static void writedata(uint8_t data) {
  ASSERT_LCD_DATA;
  ASSERT_LCD_CS;
  (void) spi_xfer(LCD_SPI, data);
  while (SPI2_SR & SPI_SR_BSY);  // DMA done doesn't mean it's safe to release CS
  RELEASE_LCD_CS;
  RELEASE_LCD_DATA;
}

static void lcd_init_seq(void) {
  writecommand(HX8357_SWRESET);
  msleep(10);

  // setextc
  writecommand(HX8357D_SETC);
  writedata(0xFF);
  writedata(0x83);
  writedata(0x57);
  //  msleep(300);

  // setRGB which also enables SDO
  writecommand(HX8357_SETRGB); 
  writedata(0x80);  //enable SDO pin!
  //    writedata(0x00);  //disable SDO pin!
  writedata(0x0);
  writedata(0x06);
  writedata(0x06);

  writecommand(HX8357D_SETCOM);
  writedata(0x25);  // -1.52V
    
  writecommand(HX8357_SETOSC);
  writedata(0x68);  // Normal mode 70Hz, Idle mode 55 Hz
    
  writecommand(HX8357_SETPANEL); //Set Panel
  writedata(0x05);  // BGR, Gate direction swapped
    
  writecommand(HX8357_SETPWR1);
  writedata(0x00);  // Not deep standby
  writedata(0x15);  //BT
  writedata(0x1C);  //VSPR
  writedata(0x1C);  //VSNR
  writedata(0x83);  //AP
  writedata(0xAA);  //FS

  writecommand(HX8357D_SETSTBA);  
  writedata(0x50);  //OPON normal
  writedata(0x50);  //OPON idle
  writedata(0x01);  //STBA
  writedata(0x3C);  //STBA
  writedata(0x1E);  //STBA
  writedata(0x08);  //GEN
    
  writecommand(HX8357D_SETCYC);  
  writedata(0x02);  //NW 0x02
  writedata(0x40);  //RTN
  writedata(0x00);  //DIV
  writedata(0x2A);  //DUM
  writedata(0x2A);  //DUM
  writedata(0x0D);  //GDON
  writedata(0x78);  //GDOFF

  /*
  writecommand(HX8357D_SETGAMMA); 
  writedata(0x02);
  writedata(0x0A);
  writedata(0x11);
  writedata(0x1d);
  writedata(0x23);
  writedata(0x35);
  writedata(0x41);
  writedata(0x4b);
  writedata(0x4b);
  writedata(0x42);
  writedata(0x3A);
  writedata(0x27);
  writedata(0x1B);
  writedata(0x08);
  writedata(0x09);
  writedata(0x03);
  writedata(0x02);
  writedata(0x0A);
  writedata(0x11);
  writedata(0x1d);
  writedata(0x23);
  writedata(0x35);
  writedata(0x41);
  writedata(0x4b);
  writedata(0x4b);
  writedata(0x42);
  writedata(0x3A);
  writedata(0x27);
  writedata(0x1B);
  writedata(0x08);
  writedata(0x09);
  writedata(0x03);
  writedata(0x00);
  writedata(0x01);
  */
  
  writecommand(HX8357_COLMOD);
  writedata(0x55);  // 16 bit

  // Set up for appropriate landscape orientation
  writecommand(HX8357_MADCTL);  
  writedata(MADCTL_MX | MADCTL_MV | MADCTL_RGB);
    
  writecommand(HX8357_TEON);  // TE off
  writedata(0x00); 
    
  writecommand(HX8357_TEARLINE);  // tear line
  writedata(0x00); 
  writedata(0x02);

  writecommand(HX8357_SLPOUT); //Exit Sleep
  msleep(150);
    
  writecommand(HX8357_DISPON);  // display on
  msleep(50);
}

static void lcd_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

  writecommand(HX8357_CASET); // Column addr set
  writedata(x0 >> 8);
  writedata(x0 & 0xFF);     // XSTART 
  writedata(x1 >> 8);
  writedata(x1 & 0xFF);     // XEND

  writecommand(HX8357_PASET); // Row addr set
  writedata(y0>>8);
  writedata(y0);     // YSTART
  writedata(y1>>8);
  writedata(y1);     // YEND

  writecommand(HX8357_RAMWR); // write to RAM
}

void lcd_clear(void) {
  lcd_fill_rect(0, 0, LCD_W, LCD_H, LCD_BLACK);
}

static void lcd_dma_setup(void)
{
  /* SPI2 TX uses DMA controller 1 Stream 4 Channel 0. */
  /* Enable DMA1 clock and IRQ */
  dma_stream_reset(DMA1, DMA_STREAM4);
  nvic_enable_irq(NVIC_DMA1_STREAM4_IRQ);
  dma_set_priority(DMA1, DMA_STREAM4, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
  dma_set_transfer_mode(DMA1, DMA_STREAM4,
                        DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  /* The register to target is the SPI2 data register */
  dma_set_peripheral_address(DMA1, DMA_STREAM4, (uint32_t) &SPI2_DR);
  dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM4);
  dma_channel_select(DMA1, DMA_STREAM4, DMA_SxCR_CHSEL_0);
}

static volatile int dma_done = 0;
void dma1_stream4_isr(void)
{
  if (dma_get_interrupt_flag(DMA1, DMA_STREAM4, DMA_TCIF)) {
    dma_done = 1;
    dma_clear_interrupt_flags(DMA1, DMA_STREAM4, DMA_TCIF);
  }
}

void lcd_setup(void) {
  lcd_backlight_set(0);
  lcd_dma_setup();
  // Toggle LCD_RESET#
  gpio_clear(GPIOA, GPIO3);
  msleep(160);  // Longer than the 2 us the datasheet says, mostly to allow for power stabilization.
  gpio_set(GPIOA, GPIO3);
  msleep(10);

  lcd_init_seq();
  
  lcd_clear();
  lcd_backlight_set(1);
}

static void lcd_blit_dma(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data, int onecolor) {
  lcd_set_window(x, y, x+w-1, y+h-1);
  ASSERT_LCD_DATA;
  ASSERT_LCD_CS;
  size_t n = w * h;

  if (onecolor)
    dma_disable_memory_increment_mode(DMA1, DMA_STREAM4);
  else
    dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
  spi_set_dff_16bit(LCD_SPI);
  // DMA can only transfer up to 65535 words at a time
  do {
    uint16_t n_seg = (n > 65535) ? 65535 : n;
    dma_set_memory_address(DMA1, DMA_STREAM4, (uint32_t) data);
    dma_set_number_of_data(DMA1, DMA_STREAM4, n_seg);
    dma_done = 0;
    dma_enable_stream(DMA1, DMA_STREAM4);
    spi_enable_tx_dma(LCD_SPI);
    while (!dma_done);  // TODO: Async
    spi_disable_tx_dma(LCD_SPI);
    if (!onecolor)
      data += n_seg;
    n -= n_seg;
  } while (n > 0);
  while (SPI2_SR & SPI_SR_BSY);  // DMA done doesn't mean it's safe to release CS
  spi_set_dff_8bit(LCD_SPI);
  RELEASE_LCD_DATA;
  RELEASE_LCD_CS;
}

void lcd_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data) {
  lcd_blit_dma(x, y, w, h, data, 0);
}

void lcd_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {

  // rudimentary clipping
  //  if((x >= LCD_W) || (y >= LCD_H) || x < 0 || y < 0 || w <= 0 || h <= 0) return;
  if (x >= LCD_W)
    x = LCD_W - 1;
  else if (x < 0)
    x = 0;
  if (y >= LCD_H)
    y = LCD_H - 1;
  else if (y < 0)
    y = 0;
  if((x + w) > LCD_W) w = LCD_W  - x;
  if((y + h) > LCD_H) h = LCD_H - y;
  lcd_blit_dma(x, y, w, h, &color, 1);
}

void lcd_demo(void) {
  uint16_t x, y, w, h, c;

  do {
    x = rand();
    y = rand();
    x %= HX8357_TFTWIDTH;
    y %= HX8357_TFTHEIGHT;
    w = rand();
    h = rand();
    w %= HX8357_TFTWIDTH - x;
    h %= HX8357_TFTHEIGHT - y;
    c = rand();
  } while (lcd_fb_prep(x, y, w, h, c) == NULL);
  
  lcd_fb_show();
}

void lcd_backlight_set(int level) {
  if (level > 0)
    gpio_set(GPIOB, GPIO0);
  else
    gpio_clear(GPIOB, GPIO0);
}

uint16_t *lcd_fb_prep(int x, int y, int w, int h, uint16_t bg) {
  if (w * h > FB_MAX_PIXELS)
    return NULL;
  if (x < 0 || y < 0)
    return NULL;
  if (x + w > LCD_W || y + h > LCD_H)
    return NULL;
  fb_x = x;
  fb_y = y;
  fb_w = w;
  fb_h = h;

  // Fill with background color
  if (bg >> 8 == (bg & 0xFF))  // Optimization when high byte = low byte (e.g. black, white)
    memset(fb, (bg & 0xFF), w * h * sizeof(uint16_t));
  else
    for (int i = 0; i < w * h; i++)
      fb[i] = bg;
  
  return fb;
}

void lcd_fb_show(void) {
  if (fb_w && fb_h)
    lcd_blit(fb_x, fb_y, fb_w, fb_h, fb);
}
