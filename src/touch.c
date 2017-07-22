#include "touch.h"
#include "clock.h"
#include "font.h"
#include "lcd.h"

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <stddef.h>
#include <stdbool.h>

/*
  PC13      = Y-   PC0/ADC10 = X+
  PC4/ADC14 = Y+   PC5/ADC15 = X-
*/

#define TOUCH_ADC ADC1
#define TOUCH_RCC_ADC RCC_ADC1

#define TOUCH_GPIO GPIOC
#define GPIO_XN GPIO5
#define GPIO_XP GPIO0
#define GPIO_YN GPIO13
#define GPIO_YP GPIO4
#define ADC_CH_XN 15
//#define ADC_CH_XP 10  not used
#define ADC_CH_YP 14

// Monitor LV bus and emergency battery voltage also
#define ADC_CH_BAT1 1
#define ADC_CH_BAT2 2
#define ADC_BAT_SCALE (((10E6 + 100E3) / 100E3) * 3.3 / 4095)  // Mistakenly populated 10M resistors instead of 1M - still works


#define N_ADC_CONVS_BAT 2
#define N_ADC_CONVS_TOUCH 7
#define N_ADC_CONVS_TOTAL (N_ADC_CONVS_BAT + N_ADC_CONVS_TOUCH)

#define NOISE_BOUNCE_THRES 64
#define CAL_TIMEOUT 8000
#define CAL_EDGE_DIST 50

struct cal_params {
  int16_t z_thres;
  int16_t x_scale;
  int16_t x_offset;
  int16_t y_scale;
  int16_t y_offset;
};

#define X_SCALE_DEF (1.182 * LCD_W)
#define X_OFFSET_DEF (-55)
#define Y_SCALE_DEF (-1.275 * LCD_H)
#define Y_OFFSET_DEF (LCD_H + 45)
#define Z_THRES_DEF 1024

static struct cal_params cal = {.z_thres = Z_THRES_DEF,
                                .x_scale = X_SCALE_DEF, .x_offset = X_OFFSET_DEF,
                                .y_scale = Y_SCALE_DEF, .y_offset = Y_OFFSET_DEF};

static volatile uint16_t adc_results[N_ADC_CONVS_TOTAL] = {0};
static volatile enum {TOUCH_SCAN_Z1, TOUCH_SCAN_Z2, TOUCH_SCAN_X, TOUCH_SCAN_Y} touch_scan_state;

static volatile uint16_t touch_raw_z = 0;
static volatile uint16_t touch_raw_x = 0;
static volatile uint16_t touch_raw_y = 0;


volatile int isr_count = 0;


static inline uint8_t setup_y(void) {
  /*
    Find Y coordinate:
    Set Y- low, Y+ high and X+ high-impedance.
    Measure X-.
  */
  gpio_clear(TOUCH_GPIO, GPIO_YN);
  gpio_set(TOUCH_GPIO, GPIO_YP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_YN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_YP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_XN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_XP);
  return ADC_CH_XN;

}

static inline uint8_t setup_x(void) {
  /*
    Find X coordinate:
    Set X- low, X+ high and Y- high-impedance.
    Measure Y+.
  */
  gpio_clear(TOUCH_GPIO, GPIO_XN);
  gpio_set(TOUCH_GPIO, GPIO_XP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_XN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_XP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_YN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_YP);
  return ADC_CH_YP;  // scan this channel
}

static inline uint8_t setup_z(void) {
  /*
    Find pressure:
    Set X+ to ground
    Set Y- to VCC
    Sample X- and Y+
  */
  gpio_clear(TOUCH_GPIO, GPIO_XP);
  gpio_set(TOUCH_GPIO, GPIO_YN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_XP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_YN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_XN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_YP);
  return ADC_CH_XN;  // scan this channel for z1
}

static inline void start_adc_seq(uint8_t touch_ch) {
  /*
  for (int i = N_ADC_CONVS_BAT; i < N_ADC_CONVS_TOTAL; i++)
    channel_array[i] = touch_ch;
  adc_set_regular_sequence(TOUCH_ADC, N_ADC_CONVS_TOTAL, channel_array);
  adc_start_conversion_regular(TOUCH_ADC);
  */
  ADC1_SQR1 = ((N_ADC_CONVS_TOTAL - 1) << ADC_SQR1_L_LSB);
  ADC1_SQR2 = touch_ch | (touch_ch << 5) | (touch_ch << 10);
  ADC1_SQR3 = ADC_CH_BAT1 | (ADC_CH_BAT2 << 5) | (touch_ch << 10) | (touch_ch << 15) | (touch_ch << 20) | (touch_ch << 25);
  ADC1_CR2 |= ADC_CR2_SWSTART;
}


/* An optimized median filter tree, from http://ndevilla.free.fr/median/median/ */
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { uint16_t temp=(a);(a)=(b);(b)=temp; }
static inline uint16_t opt_med7(uint16_t * p)
{
    PIX_SORT(p[0], p[5]) ; PIX_SORT(p[0], p[3]) ; PIX_SORT(p[1], p[6]) ;
    PIX_SORT(p[2], p[4]) ; PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[5]) ;
    PIX_SORT(p[2], p[6]) ; PIX_SORT(p[2], p[3]) ; PIX_SORT(p[3], p[6]) ;
    PIX_SORT(p[4], p[5]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[1], p[3]) ;
    PIX_SORT(p[3], p[4]) ; return (p[3]) ;
}


void dma2_stream0_isr(void)
{
  // All the main logic happens in this ISR.  Better keep it fast!
  gpio_set(GPIOA, GPIO8); // LED / scope probe

  static uint16_t z, x, y;
  
  if (dma_get_interrupt_flag(DMA2, DMA_STREAM0, DMA_TCIF)) {
    isr_count++;
    dma_clear_interrupt_flags(DMA2, DMA_STREAM0, DMA_TCIF);

    uint16_t median = opt_med7((uint16_t *)&adc_results[N_ADC_CONVS_BAT]);
    uint8_t next_ch;
    switch (touch_scan_state) {  // which channel did we just convert?
    case TOUCH_SCAN_Z1:
      z = median; // actually just z1
      touch_scan_state = TOUCH_SCAN_Z2;
      next_ch = ADC_CH_YP;
      break;
    case TOUCH_SCAN_Z2:
      z = 4095 - (median - z);
      if (z >= cal.z_thres) {
        // touch detected
        next_ch = setup_x();
        touch_scan_state = TOUCH_SCAN_X;
      } else {
        // no touch, keep looking at Z.  We're already set up, don't need to do that again.
        touch_raw_z = z;
        touch_scan_state = TOUCH_SCAN_Z1;
        next_ch = ADC_CH_XN;
      }
      break;
    case TOUCH_SCAN_X:
      x = median;
      next_ch = setup_y();
      touch_scan_state = TOUCH_SCAN_Y;
      break;
    case TOUCH_SCAN_Y:
      y = median;
      touch_raw_x = y;  // swap X and Y due to panel rotation
      touch_raw_y = x;
      touch_raw_z = z;
    default:
      next_ch = setup_z();
      touch_scan_state = TOUCH_SCAN_Z1;
    }
    (void)next_ch;
    start_adc_seq(next_ch);
  }
  gpio_clear(GPIOA, GPIO8); // LED / scope probe
}

void touch_setup(void) {

/* To reject noise coupled from the OLED etc, we'll take a bunch of
   measurements in a row and find the median.  To do that, we'll set
   up the ADC in scan mode, non-continuous, with DMA and an interrupt
   at the end of the sequence.  While we're at it, we'll sample the
   battery voltages in the same scan.  Doing that at the start of the
   scan allows some time for the touch signals to settle after
   switching the excitation. */

  rcc_periph_clock_enable(TOUCH_RCC_ADC);

  dma_stream_reset(DMA2, DMA_STREAM0);
  dma_channel_select(DMA2, DMA_STREAM0, DMA_SxCR_CHSEL_0);
  dma_set_peripheral_address(DMA2, DMA_STREAM0, (uint32_t) &ADC1_DR);
  dma_set_priority(DMA2, DMA_STREAM0, DMA_SxCR_PL_HIGH);
  dma_set_memory_size(DMA2, DMA_STREAM0, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA2, DMA_STREAM0, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA2, DMA_STREAM0);
  dma_set_memory_address(DMA2, DMA_STREAM0, (uint32_t) &adc_results);
  dma_set_number_of_data(DMA2, DMA_STREAM0, N_ADC_CONVS_TOTAL);
  dma_set_transfer_mode(DMA2, DMA_STREAM0,
                        DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM0);
  dma_enable_circular_mode(DMA2, DMA_STREAM0);
  dma_clear_interrupt_flags(DMA2, DMA_STREAM0, DMA_ISR_FLAGS);
  nvic_enable_irq(NVIC_DMA2_STREAM0_IRQ);
  dma_enable_stream(DMA2, DMA_STREAM0);
  
  adc_power_off(TOUCH_ADC);
  rcc_periph_reset_pulse(RST_ADC);
  usleep(22);
  adc_enable_scan_mode(TOUCH_ADC);
  adc_disable_eoc_interrupt(TOUCH_ADC);  // We'll use the DMA interrupt instead, don't need both
  //adc_eoc_after_group(TOUCH_ADC);
  //adc_set_continuous_conversion_mode(TOUCH_ADC);
  adc_set_dma_continue(TOUCH_ADC);
  adc_set_clk_prescale(ADC_CCR_ADCPRE_BY8);  // gives f_adc = 10.5 MHz
  adc_set_sample_time_on_all_channels(TOUCH_ADC, ADC_SMPR_SMP_480CYC); // Long cycles not so much for accuracy as to reduce interrupt frequency
  adc_enable_dma(TOUCH_ADC);
  ADC1_SR = 0;
  adc_power_on(TOUCH_ADC);
  usleep(22);

  // The whole sequence is interrupt driven, we have to bootstrap it.
  touch_scan_state = TOUCH_SCAN_Z1;
  start_adc_seq(setup_z());
}


int touch_get(int *x, int *y) {
  if (touch_raw_z < cal.z_thres)
    return 0;  // no touch

  if (x && y) {  // You can pass null ptrs if you don't care where the touch is
    *x = ((touch_raw_x * cal.x_scale) >> 12) + cal.x_offset;
    *y = ((touch_raw_y * cal.y_scale) >> 12) + cal.y_offset;
  }

  // Clip to screen borders, just in case
  if (*x < 0)
    *x = 0;
  else if (*x >= LCD_W)
    *x = LCD_W - 1;
  if (*y < 0)
    *y = 0;
  else if (*y >= LCD_H)
    *y = LCD_H - 1;
  return 1;
}

/*
// Return 0 on success, -1 on timeout or parameter error.
static int touch_cal_point(int16_t tgt_x, int16_t tgt_y, int16_t raw_xyz[3]) {

  lcd_fill_rect(tgt_x - 3, tgt_y - 3, 7, 7, LCD_PURPLE);
  
  uint32_t t_start = mtime();
  int pressed = 0, count = 0, done = 0;
  while (mtime() - t_start < CAL_TIMEOUT) {
    int16_t x_raw, y_raw, z_raw;
    z_raw = touch_get_raw(&x_raw, &y_raw);
    if (z_raw >= cal.z_thres) { // excludes noise error cases when touch_get_raw returns -1
      if (!pressed) {
        count = 0;
        pressed = 1;
      } else {
        count++;
      }
    } else {
      if (pressed) {
        pressed = 0;
        count = 0;
      } else {
        count++;
      }
    }
    if (count == 22) { // stabilized
      if (pressed) {
        // lock in the values
        raw_xyz[0] = x_raw;
        raw_xyz[1] = y_raw;
        raw_xyz[2] = z_raw;
        // ack to the user
        lcd_fill_rect(tgt_x - 40, tgt_y - 40, 81, 81, LCD_YELLOW);
        done = 1;
      } else {
        if (done) {
          lcd_fill_rect(tgt_x - 40, tgt_y - 40, 81, 81, LCD_BLACK);
          return 0;
        }
      }
    }
    msleep(10);
  }
  return -1;
}

int touch_cal(void) {
  lcd_clear();
  lcd_textbox_prep(LCD_W / 2 - 180, 90, 360, 50, LCD_BLACK);
  lcd_printf(LCD_WHITE, &FreeSans12pt7b, "Please touch the indicated points,\n");
  lcd_textbox_move_cursor(30, 0, 1);
  lcd_printf(LCD_WHITE, &FreeSans12pt7b, "or wait to cancel calibration.");
  lcd_textbox_show();

  // Choose a conservatively low pressure threshold for the calibration
  cal.z_thres = 500;

  // Display 5 target points
  const int16_t tgt_xy[5][2] = {{CAL_EDGE_DIST, CAL_EDGE_DIST},
                                {LCD_W - CAL_EDGE_DIST, CAL_EDGE_DIST},
                                {LCD_W - CAL_EDGE_DIST, LCD_H - CAL_EDGE_DIST},
                                {CAL_EDGE_DIST, LCD_H - CAL_EDGE_DIST},
                                {LCD_W / 2, LCD_H / 2}};
                                
  int16_t raw_xyz[5][3];
  for (int i = 0; i < 5; i++) {
    if (touch_cal_point(tgt_xy[i][0], tgt_xy[i][1], raw_xyz[i]) != 0) {
      lcd_clear();
      cal.z_thres = Z_THRES_DEF;
      return -1;
    }
  }

  // Easy start: calculate the pressure threshold
  cal.z_thres = 0;
  for (int i = 0; i < 5; i++)
    cal.z_thres += raw_xyz[i][2];
  cal.z_thres /= 15;  // 1/3 of the average
  
  // OK, now the tricky part.  This is probably not optimal..
  cal.x_scale = (2 * (LCD_W - 2 * CAL_EDGE_DIST)  * (1<<12)) / (raw_xyz[1][0] + raw_xyz[2][0] - raw_xyz[0][0] - raw_xyz[3][0]);
  cal.y_scale = (2 * (LCD_H - 2 * CAL_EDGE_DIST)  * (1<<12)) / (raw_xyz[2][1] + raw_xyz[3][1] - raw_xyz[0][1] - raw_xyz[1][1]);

  int32_t tgt_sum_x = 0, tgt_sum_y = 0, obs_sum_x = 0, obs_sum_y = 0;
  for (int i = 0; i < 5; i++) {
    obs_sum_x += raw_xyz[i][0];
    obs_sum_y += raw_xyz[i][1];
    tgt_sum_x += tgt_xy[i][0];
    tgt_sum_y += tgt_xy[i][1];
  }
  cal.x_offset = (tgt_sum_x - ((obs_sum_x * cal.x_scale) >> 12)) / 5;
  cal.y_offset = (tgt_sum_y - ((obs_sum_y * cal.y_scale) >> 12)) / 5;
  
  // Display results so they can be hard-coded into firmware
  lcd_clear();
  lcd_textbox_prep(0, 0, 300, 90, LCD_DARKGREY);
  lcd_printf(LCD_WHITE, &FreeMono9pt7b, "z_thres = %d\n"
             "x_scale = %.3f * LCD_W\n"
             "x_offset = %d px\n"
             "y_scale = %.3f * LCD_H\n"
             "x_offset = LCD_H %+d px\n",
             cal.z_thres,
             cal.x_scale * 1.0 / LCD_W,
             cal.x_offset,
             cal.y_scale * 1.0 / LCD_H,
             cal.y_offset - LCD_H);
  
  lcd_textbox_show();
  lcd_textbox_prep(300, 0, 180, 90, LCD_BLACK);
  for (int i = 0; i < 5; i++)
    lcd_printf(LCD_GREEN, &FreeMono9pt7b, "%5d%5d%5d\n",
               raw_xyz[i][0], raw_xyz[i][1], raw_xyz[i][2]);
  lcd_textbox_show();


  lcd_textbox_prep(LCD_W / 2 - 155, LCD_H/2 - 12, 310, 50, LCD_BLACK);
  // Sanity check
  if (cal.x_scale < X_SCALE_DEF - 80 || cal.x_scale > X_SCALE_DEF + 80
      || cal.y_scale < Y_SCALE_DEF - 60 || cal.y_scale > Y_SCALE_DEF + 60
      || cal.x_offset < X_OFFSET_DEF - 50 || cal.x_offset > X_OFFSET_DEF + 50
      || cal.y_offset < Y_OFFSET_DEF - 50 || cal.y_offset > Y_OFFSET_DEF + 50) {
    cal.x_scale = X_SCALE_DEF;
    cal.x_offset = X_OFFSET_DEF;
    cal.y_scale = Y_SCALE_DEF;
    cal.y_offset = Y_OFFSET_DEF;
    cal.z_thres = Z_THRES_DEF;
    lcd_printf(LCD_RED, &FreeSansBold12pt7b, "Cal failed; using defaults.\n");
  }
  lcd_textbox_move_cursor(55, 0, 1);
  lcd_printf(LCD_WHITE, &FreeSans12pt7b, "Touch to continue.");
  lcd_textbox_show();  

  while (!touch_get(NULL, NULL));
  lcd_clear();
  while (touch_get(NULL, NULL));
  msleep(222);
  return 0;
}
*/


void touch_show_debug(void) {
  int t;
  static int no_touch = 0, touch = 0;

  const int draw_rad = 1;
  static int x=-1, y=-1;
  if (x >= 0 && y >= 0)
    lcd_fill_rect(x - draw_rad, y - draw_rad, 2 * draw_rad + 1, 2 * draw_rad + 1, LCD_DARKGREY);
  t = utime_tick();
  int r = touch_get(&x, &y);
  t = utime_tock(t);
  if (r) {
    touch++;
    lcd_fill_rect(x - draw_rad, y - draw_rad, 2 * draw_rad + 1, 2 * draw_rad + 1, LCD_PURPLE);
  } else {
    no_touch++;
    x = y = -1;
  }
  
  lcd_textbox_prep(0, LCD_H - 14, 222, 14, LCD_BLACK);
  lcd_printf(LCD_GREEN, &RobotoCondensed_Regular7pt7b, "%d %d %d %.1fV %.1fV", isr_count, no_touch, touch,
             adc_results[0] * ADC_BAT_SCALE, adc_results[1] * ADC_BAT_SCALE);
  lcd_textbox_show();
  
  
}
