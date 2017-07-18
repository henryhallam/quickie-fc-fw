#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>

#include "board.h"
#include "clock.h"
#include "lcd.h"
#include "touch.h"
#include "leds.h"

/* All pins are allocated in this file, though they may be re-setup by peripheral drivers. 
   We'll also book-keep shared resources such as DMA below:
    - DMA1 Channel 0 Stream 4: LCD SPI
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

  // Port B
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);  // LCD_BACKLIGHT_PWM
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);  // RELAY2_CMD
  gpio_set(GPIOB, GPIO5);
  gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);  // SD_CS#
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);      // FONA_TX
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO7);      // FONA_RX
  gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);      // CAN_RX
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
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2);      // LCD_MISO
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
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);     // IsoSPI_SCLK
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);     // IsoSPI_MISO
  gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12);     // IsoSPI_MOSI
  gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO13); // LCD_TOUCH_X+

  // Port D
  gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO2);  // LCD_D/C#
}

static void spi_setup(void) {
  /* SPI2 for LCD and SD Card */
  rcc_periph_clock_enable(RCC_SPI2);
  spi_init_master(SPI2, SPI_CR1_BAUDRATE_FPCLK_DIV_2,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_8BIT,
                  SPI_CR1_MSBFIRST);
  //spi_enable_ss_output(SPI2);
  spi_enable(SPI2);
}

static void dma_setup(void) {
  // Just enable the clock to the DMA controllers in use; the peripheral drivers will configure it.
  rcc_periph_clock_enable(RCC_DMA1);
}

void board_setup(void) {
  clock_setup();
  dma_setup();
  pins_setup();
  spi_setup();
  leds_setup();
  lcd_setup();
  touch_setup();
}
