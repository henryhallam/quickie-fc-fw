#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

#include "board.h"
#include "can.h"
#include "clock.h"
#include "font.h"
#include "lcd.h"
#include "leds.h"
#include "sdcard.h"
#include "spi2_dma.h"
#include "touch.h"
#include "usb.h"


/* All pins are allocated in this file, though they may be re-setup by peripheral drivers. 
   We'll also book-keep shared resources such as DMA below:
    - DMA1 stream 2: IsoSPI RX (ch0)
    - DMA1 stream 3: LCD/SD SPI RX (ch0)
    - DMA1 stream 4: LCD/SD SPI TX (ch0)
    - DMA1 stream 5: IsoSPI TX (ch0)
    - DMA1 stream 6: DAC2 (ch7)  TODO
    - DMA2 stream 0: ADC1 (ch0)
*/

static void pins_setup(void)
{
  /* Enable GPIO clocks. */
  rcc_periph_clock_enable(RCC_GPIOA);
  rcc_periph_clock_enable(RCC_GPIOB);
  rcc_periph_clock_enable(RCC_GPIOC);
  rcc_periph_clock_enable(RCC_GPIOD);
  
  // Port A
  gpio_mode_setup(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO0);   // FONA_RI
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO1);  // BAT1_MON_DIV
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO2);  // BAT2_MON_DIV
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO3);  // LCD_RESET#
  gpio_set(GPIOA, GPIO4);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO4);  // IsoSPI_CS#
  gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);  // ALERT_AUDIO 
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO6);  // RELAY1_CMD
  gpio_set_af(GPIOA, GPIO_AF3, GPIO7);
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);      // LED1_G/R#
  gpio_set(GPIOA, GPIO8);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);  // FONA_PWRKEY
  
  gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9 | GPIO11 | GPIO12); // USB pins
  gpio_set_af(GPIOA, GPIO_AF10, GPIO9 | GPIO11 | GPIO12);

  // Port B
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);  // LCD_BACKLIGHT_PWM
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);  // RELAY2_CMD
  gpio_set(GPIOB, GPIO5);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);  // SD_CS#
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);      // FONA_TX
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);      // FONA_RX
  gpio_set_af(GPIOB, GPIO_AF9, GPIO8);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO8);    // CAN_RX
  gpio_set_af(GPIOB, GPIO_AF9, GPIO9);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9);      // CAN_TX
  gpio_set_af(GPIOB, GPIO_AF5, GPIO10);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);     // LCD_SCLK
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11); // RELAY3_CMD
  gpio_set(GPIOB, GPIO12);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12); // LCD_CS#
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13); // RELAY4_CMD
  gpio_set_af(GPIOB, GPIO_AF3, GPIO14);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO14);     // LED2_G/R#
  gpio_set_af(GPIOB, GPIO_AF3, GPIO15);
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO15);     // LED3_G/R#
  
  // Port C
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);  // LCD_TOUCH_Y-
  gpio_set_af(GPIOC, GPIO_AF5, GPIO2);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO2);    // LCD_MISO
  gpio_set_af(GPIOC, GPIO_AF5, GPIO3);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3);      // LCD_MOSI
  gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);  // LCD_TOUCH_Y+
  gpio_mode_setup(GPIOC, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);  // LCD_TOUCH_X-
  gpio_set_af(GPIOC, GPIO_AF3, GPIO6);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);      // LED1_R/G#
  gpio_set_af(GPIOC, GPIO_AF3, GPIO7);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);      // LED2_R/G#
  gpio_set_af(GPIOC, GPIO_AF3, GPIO8);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);      // LED3_R/G#
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO9);  // RELAY5_CMD
  gpio_set_af(GPIOC, GPIO_AF6, GPIO10);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);     // IsoSPI_SCLK
  gpio_set_af(GPIOC, GPIO_AF6, GPIO11);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);     // IsoSPI_MISO
  gpio_set_af(GPIOC, GPIO_AF6, GPIO12);
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12);     // IsoSPI_MOSI
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13); // LCD_TOUCH_X+

  // Port D
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);  // LCD_D/C#
}

static void spi_setup(void) {
  /* SPI2 for LCD and SD Card */
  rcc_periph_clock_enable(RCC_SPI2);
  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_2,  // As fast as possible, i.e. 21 MHz
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_8BIT,
                  SPI_CR1_MSBFIRST);
  //spi_enable_ss_output(SPI2);
  spi_enable(SPI2);
  
  /* SPI3 for isoSPI */
  rcc_periph_clock_enable(RCC_SPI3);
  spi_init_master(SPI3, SPI_CR1_BAUDRATE_FPCLK_DIV_64,  // 656.25 kHz.  LTC6820 says 1 MHz max.
                  SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,      // CPOL = 1
                  SPI_CR1_CPHA_CLK_TRANSITION_2,        // CPHA = 1
                  SPI_CR1_DFF_8BIT,
                  SPI_CR1_MSBFIRST);
  //spi_enable_ss_output(SPI3);
  spi_enable(SPI3);
}

static void dma_setup(void) {
  // Just enable the clock to the DMA controllers in use; the peripheral drivers will configure the streams.
  rcc_periph_clock_enable(RCC_DMA1);
  rcc_periph_clock_enable(RCC_DMA2);
}


void board_setup(void) {
  clock_setup();
  dma_setup();
  pins_setup();
  leds_setup();
  spi_setup();
  spi2_dma_setup();
  lcd_setup();
  touch_setup();
  int r = can_setup();
  if (r) {
    lcd_textbox_prep(0, 0, 480, 45, LCD_BLACK);
    lcd_printf(LCD_GREEN, &FreeMono9pt7b, "can_setup() = %d", r);
    lcd_textbox_show();
    msleep(1000);
  }
  /*
  int ret = sdcard_setup();
  lcd_textbox_prep(0, 0 , LCD_W, 20, LCD_BLACK);
  lcd_printf(LCD_WHITE, &Roboto_Regular8pt7b, "SD: 0x%02X", ret);
  lcd_textbox_show();
  msleep(2222);
  */
}

void relay_ctl(relay_e relay, int state) {
  switch(relay) {
  case 1:
    if (state)
      gpio_set(GPIOA, GPIO6);
    else
      gpio_clear(GPIOA, GPIO6);
    break;
  case 2:
    if (state)
      gpio_set(GPIOB, GPIO1);
    else
      gpio_clear(GPIOB, GPIO1);
    break;
  case 3:
    if (state)
      gpio_set(GPIOB, GPIO11);
    else
      gpio_clear(GPIOB, GPIO11);
    break;
  case 4:
    if (state)
      gpio_set(GPIOB, GPIO13);
    else
      gpio_clear(GPIOB, GPIO13);
    break;
  case 5:
    if (state)
      gpio_set(GPIOC, GPIO9);
    else
      gpio_clear(GPIOC, GPIO9);
    break;
  default:
    return;
  }
}
