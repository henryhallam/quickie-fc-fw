#include "touch.h"
#include "clock.h"
#include "font.h"
#include "lcd.h"

#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <stddef.h>

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
#define ADC_CH_XP 10
#define ADC_CH_YP 14

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
#define Z_THRES_DEF 944

static struct cal_params cal = {.z_thres = Z_THRES_DEF,
                                .x_scale = X_SCALE_DEF, .x_offset = X_OFFSET_DEF,
                                .y_scale = Y_SCALE_DEF, .y_offset = Y_OFFSET_DEF};

void touch_setup(void) {
  rcc_periph_clock_enable(TOUCH_RCC_ADC);
  adc_power_off(TOUCH_ADC);
  adc_disable_scan_mode(TOUCH_ADC);
  adc_set_sample_time_on_all_channels(TOUCH_ADC, ADC_SMPR_SMP_480CYC);
  adc_power_on(TOUCH_ADC);
}

// Reads twice to reduce bounce / noise.  Returns avg if consistent, -1 otherwise.
static int16_t adc_read(uint8_t channel) {
  uint8_t channel_array[16];
  channel_array[0] = channel;
  adc_set_regular_sequence(TOUCH_ADC, 1, channel_array);
  int16_t result[2];
  for (int i = 0; i < 2; i++) {
    adc_start_conversion_regular(TOUCH_ADC);
    while (!adc_eoc(TOUCH_ADC));
    result[i] = adc_read_regular(TOUCH_ADC);
  }
  int16_t diff = result[0] - result[1];
  if (diff > NOISE_BOUNCE_THRES || diff < -NOISE_BOUNCE_THRES)
    return -1;
  return (result[0] + result[1]) / 2;
}

static int16_t touch_get_raw(int16_t *x, int16_t *y) {
  /*
    1. Find X coordinate:
       Set X- low, X+ high and Y- high-impedance.
       Measure Y+.
  */
  gpio_clear(TOUCH_GPIO, GPIO_XN);
  gpio_set(TOUCH_GPIO, GPIO_XP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_XN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_XP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_YN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_YP);
  usleep(20);
  *y = adc_read(ADC_CH_YP);  // Swap X and Y due to screen rotation.  Also, Y's flipped (but scale + offset take care of that)
  
  /*
    2. Find Y coordinate:
       Set Y- low, Y+ high and X+ high-impedance.
       Measure X-.
  */
  gpio_clear(TOUCH_GPIO, GPIO_YN);
  gpio_set(TOUCH_GPIO, GPIO_YP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_YN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_YP);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_XN);
  gpio_mode_setup(TOUCH_GPIO, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_XP);
  usleep(20);
  *x = adc_read(ADC_CH_XN);  // Swap X and Y due to screen rotation
  
  /*
    3. Find pressure:
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
  usleep(20);
  
  int z1 = adc_read(ADC_CH_XN);
  int z2 = adc_read(ADC_CH_YP);

  if (*x == -1 || *y == -1 || z1 == -1 || z2 == -1)
    return -1;
  
  return 4095 - (z2 - z1); // z
}


int touch_get(int *x, int *y) {
  int16_t x_raw, y_raw, z_raw;
  z_raw = touch_get_raw(&x_raw, &y_raw);
  if (z_raw < cal.z_thres) // includes noise error cases when touch_get_raw returns -1
    return 0;  // no touch

  if (x && y) {  // You can pass null ptrs if you don't care where the touch is
    *x = ((x_raw * cal.x_scale) >> 12) + cal.x_offset;
    *y = ((y_raw * cal.y_scale) >> 12) + cal.y_offset;
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


// Return 0 on success, -1 on timeout or parameter error.
static int touch_cal_point(int16_t tgt_x, int16_t tgt_y, int16_t raw_xyz[3]) {
  if (lcd_fb_prep(tgt_x - 3, tgt_y - 3, 7, 7, LCD_PURPLE))
    lcd_fb_show();
  else
    return -1;
  
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
        lcd_fb_prep(tgt_x - 40, tgt_y - 40, 81, 81, LCD_YELLOW);
        lcd_fb_show();
        done = 1;
      } else {
        if (done) {
          lcd_fb_prep(tgt_x - 40, tgt_y - 40, 81, 81, LCD_BLACK);
          lcd_fb_show();
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
