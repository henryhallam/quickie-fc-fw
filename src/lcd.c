#include "lcd.h"
#include "clock.h"
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/gpio.h>

#define ASSERT_LCD_CS gpio_clear(GPIOB, GPIO12)
#define RELEASE_LCD_CS gpio_set(GPIOB, GPIO12)

#define ASSERT_LCD_DATA gpio_set(GPIOD, GPIO2)
#define RELEASE_LCD_DATA gpio_clear(GPIOD, GPIO2)

#define LCD_SPI SPI2

#define HX8357D 0xD
#define HX8357B 0xB

#define HX8357_TFTWIDTH  320
#define HX8357_TFTHEIGHT 480

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
#define HX8357B_PTLON   0x12
#define HX8357B_NORON   0x13

#define HX8357_INVOFF  0x20
#define HX8357_INVON   0x21
#define HX8357_DISPOFF 0x28
#define HX8357_DISPON  0x29

#define HX8357_CASET   0x2A
#define HX8357_PASET   0x2B
#define HX8357_RAMWR   0x2C
#define HX8357_RAMRD   0x2E

#define HX8357B_PTLAR   0x30
#define HX8357_TEON  0x35
#define HX8357_TEARLINE  0x44
#define HX8357_MADCTL  0x36
#define HX8357_COLMOD  0x3A

#define HX8357_SETOSC 0xB0
#define HX8357_SETPWR1 0xB1
#define HX8357B_SETDISPLAY 0xB2
#define HX8357_SETRGB 0xB3
#define HX8357D_SETCOM  0xB6

#define HX8357B_SETDISPMODE  0xB4
#define HX8357D_SETCYC  0xB4
#define HX8357B_SETOTP 0xB7
#define HX8357D_SETC 0xB9

#define HX8357B_SET_PANEL_DRIVING 0xC0
#define HX8357D_SETSTBA 0xC0
#define HX8357B_SETDGC  0xC1
#define HX8357B_SETID  0xC3
#define HX8357B_SETDDB  0xC4
#define HX8357B_SETDISPLAYFRAME 0xC5
#define HX8357B_GAMMASET 0xC8
#define HX8357B_SETCABC  0xC9
#define HX8357_SETPANEL  0xCC

#define HX8357B_SETPOWER 0xD0
#define HX8357B_SETVCOM 0xD1
#define HX8357B_SETPWRNORMAL 0xD2

#define HX8357B_RDID1   0xDA
#define HX8357B_RDID2   0xDB
#define HX8357B_RDID3   0xDC
#define HX8357B_RDID4   0xDD

#define HX8357D_SETGAMMA 0xE0

#define HX8357B_SETGAMMA 0xC8
#define HX8357B_SETPANELRELATED  0xE9

// Color definitions
#define	HX8357_BLACK   0x0000
#define	HX8357_BLUE    0x001F
#define	HX8357_RED     0xF800
#define	HX8357_GREEN   0x07E0
#define HX8357_CYAN    0x07FF
#define HX8357_MAGENTA 0xF81F
#define HX8357_YELLOW  0xFFE0  
#define HX8357_WHITE   0xFFFF

/*
 * This structure defines the sequence of commands to send
 * to the Display in order to initialize it. The AdaFruit
 * folks do something similar, it helps when debugging the
 * initialization sequence for the display.
 */
struct tft_command {
  uint16_t delay;		/* If you need a delay after */
  uint8_t cmd;		/* command to send */
  uint8_t n_args;		/* How many arguments it has */
};


/* prototype for lcd_command */
static void lcd_command(uint8_t cmd, int delay, int n_args,
                        const uint8_t *args);

/*
 * void lcd_command(cmd, delay, args, arg_ptr)
 *
 * All singing all dancing 'do a command' feature. Basically it
 * sends a command, and if args are present it sets 'data' and
 * sends those along too.
 */
static void
lcd_command(uint8_t cmd, int delay, int n_args, const uint8_t *args)
{
  int i;

  ASSERT_LCD_CS;
  (void) spi_xfer(LCD_SPI, cmd);
  if (n_args) {
    ASSERT_LCD_DATA;
    for (i = 0; i < n_args; i++) {
      (void) spi_xfer(LCD_SPI, *(args+i));
    }
  }
  RELEASE_LCD_CS;
  RELEASE_LCD_DATA;
  if (delay) {
    msleep(delay);		/* wait, if called for */
  }
}

static void writecommand(uint8_t cmd) {
  ASSERT_LCD_CS;
  (void) spi_xfer(LCD_SPI, cmd);
  RELEASE_LCD_CS;
}

static void writedata(uint8_t data) {
  ASSERT_LCD_DATA;
  ASSERT_LCD_CS;
  (void) spi_xfer(LCD_SPI, data);
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
  msleep(300);

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
    
  writecommand(HX8357_COLMOD);
  writedata(0x55);  // 16 bit
    
  writecommand(HX8357_MADCTL);  
  writedata(0xC0); 
    
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

void lcd_setup(void) {
  lcd_backlight_set(1);

  // Toggle LCD_RESET# high-low-high
  gpio_set(GPIOA, GPIO3);
  msleep(100);
  gpio_clear(GPIOA, GPIO3);
  msleep(100);
  gpio_set(GPIOA, GPIO3);
  msleep(150);

  lcd_init_seq();
}

void lcd_demo(void) {
  static int i = 0;
  writecommand(i ? HX8357_INVON : HX8357_INVOFF);
  i = !i;
}

void lcd_backlight_set(int level) {
  if (level > 0)
    gpio_set(GPIOB, GPIO0);
  else
    gpio_clear(GPIOB, GPIO0);
}
