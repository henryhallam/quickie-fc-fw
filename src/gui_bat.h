#ifndef GUI_BAT__H
#define GUI_BAT__H

#include "lcd.h"

#define GUI_BAT_W 144
#define GUI_BAT_H 216

#define GUI_BAT_X (LCD_W - GUI_BAT_W)
#define GUI_BAT_Y 0

typedef struct {
  float min;
  float max;
  float mean;
  float median;
  float std;
  float p10;
  float p90;
  int n_unbal;
  int n_error;
} bat_stats_t;


int gui_bat_draw_frame(void);
void gui_bat_draw_data(void);
void debug_bat(int page);

void bat_calc_stats(int *hist_bins, const float *cell_vs, int n_cells, bat_stats_t *stats);

extern int packs_talking;

#endif // GUI_BAT__H
