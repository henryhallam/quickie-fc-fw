/* Battery GUI
 Ideas:
  - Translucent histogram behind text
  - Separate full-page histogram
  - Quick ID of out-of-family packs
    - Text description
    - Aircraft schematic
  - Internal resistance calculation, compensation and reporting
 */
#include "gui_bat.h"
#include "lcd.h"
#include "font.h"
#include "ltc6804.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define TITLE_H 14
#define TITLE_FONT Roboto_Bold7pt7b
#define DATA_FONT RobotoCondensed_Regular7pt7b
#define DEBUG_FONT RobotoCondensed_Regular7pt7b
#define TITLE_W 52 // (GUI_MC_W * 0.36)
#define DATA_W 46 //(GUI_MC_W * 0.32)

#define HIST_BIN_DRAW_CLIP (GUI_BAT_W - 2)

#define HIST_N_BINS 151
#define HIST_LO_BIN_V 2.8f
#define HIST_HI_BIN_V 4.3f
#define HIST_BIN_V_SPAN ((HIST_HI_BIN_V - HIST_LO_BIN_V) / (HIST_N_BINS - 1))

#define CELL_V_LO_RED 3.0f
#define CELL_V_LO_YEL 3.2f
#define CELL_V_MID 3.6f
#define CELL_V_HI_YEL 4.0f
#define CELL_V_HI_RED 4.15f
#define CELL_V_UNBAL_THRES 0.05f  // More than this far from the median = "unbalanced"

#define CELL_V_INFINITY 6.5536f // Larger than the largest value LTC6804 can return 

int packs_talking = 0;

#define N_BAT_STRINGS 3
#define N_PACKS_PER_STRING 9
#define N_CELLS_PER_PACK 12
#define N_TEMPS_PER_PACK 6  // 5 external + 1 internal
#define N_VOLTAGES (N_BAT_STRINGS * N_PACKS_PER_STRING * N_CELLS_PER_PACK)
#define N_TEMPS (N_BAT_STRINGS * N_PACKS_PER_STRING * N_TEMPS_PER_PACK)

static float voltages[N_VOLTAGES];
static float temperatures[N_TEMPS];

int gui_bat_draw_frame(void) {
  if (GUI_BAT_X < 0 || GUI_BAT_Y < 0 || GUI_BAT_X + GUI_BAT_W > LCD_W || GUI_BAT_Y + GUI_BAT_H > LCD_H)
    return -1;
  lcd_textbox_prep(GUI_BAT_X, GUI_BAT_Y, GUI_BAT_W, TITLE_H, LCD_DARKGREY);
  lcd_textbox_move_cursor(TITLE_W, 0, 1);
  lcd_printf(LCD_WHITE, &TITLE_FONT, "Fuel  FW  Dist");
  lcd_textbox_show();
  lcd_textbox_prep(GUI_BAT_X, GUI_BAT_Y + TITLE_H, TITLE_W, GUI_BAT_H - TITLE_H, LCD_DARKGREY);
  // lcd_printf(LCD_WHITE, &TITLE_FONT, "Vtot\nMax\nP90\nMean\nP10\nMin\nOOFam\nTmax\nFeed\ntOT\ntUV\nPwir");
  lcd_textbox_show();
  lcd_fill_rect(GUI_BAT_X + TITLE_W, GUI_BAT_Y + TITLE_H, DATA_W * 2, GUI_BAT_H - TITLE_H, LCD_DARKGREY);
  return 0;
}

void bat_calc_stats(int *hist_bins, const float *cell_vs, int n_cells, bat_stats_t *stats) {

  /* Zero the histogram and initialize stats accumulators */
  memset(hist_bins, 0, sizeof(*hist_bins) * HIST_N_BINS);
  memset(stats, 0, sizeof(*stats));
  float min_v = CELL_V_INFINITY;
  float max_v = 0;
  float sum_v = 0;
  float sumsq_v = 0;
    
  for (int i = 0; i < n_cells; i++) {
    /* Histogram: Which bin does it fall in? */
    int bin_index = (cell_vs[i] - HIST_LO_BIN_V) / (HIST_BIN_V_SPAN);
    /* First and last bins are special - they include out-of-range cells */
    if (bin_index < 0)
      bin_index = 0;
    else if (bin_index >= HIST_N_BINS)
      bin_index = HIST_N_BINS - 1;
    /* Count it. */
    hist_bins[bin_index]++;

    /* Other stat accumulators */
    if (cell_vs[i] > max_v)
      max_v = cell_vs[i];
    if (cell_vs[i] < min_v)
      min_v = cell_vs[i];
    sum_v += cell_vs[i];
    sumsq_v += cell_vs[i] * cell_vs[i];
  }
  stats->min = min_v;
  stats->max = max_v;
  stats->mean = sum_v / n_cells;
  stats->std = sqrtf(sumsq_v / n_cells - (stats->mean * stats->mean));
  /* Find approximations of the percentiles and median from the histogram */
  int hist_count_cum = 0;
  for (int i = 0; i < HIST_N_BINS; i++) {
    int hcc_prev = hist_count_cum;
    hist_count_cum += hist_bins[i];
    if (stats->p10 == 0 && hist_count_cum >= n_cells * 0.10)
      stats->p10 = HIST_LO_BIN_V + HIST_BIN_V_SPAN * (i + 0.5f);
    if (stats->median == 0 && hist_count_cum >= n_cells / 2) {
      /* Better approximate the median by interpolating to estimate
	 its position within the bin */
      stats->median = HIST_LO_BIN_V + HIST_BIN_V_SPAN * i +
	(((n_cells / 2) - hcc_prev) / hist_bins[i]) * HIST_BIN_V_SPAN;
    }
    if (stats->p90 == 0 && hist_count_cum >= n_cells * 0.90) {
      stats->p90 = HIST_LO_BIN_V + HIST_BIN_V_SPAN * (i + 0.5f);
      // If we found the 90th %ile we must have already found the 10th and median
      break;
    }
  }
  /* Count number of "unbalanced" cells */
  for (int i = 0; i < n_cells; i++)
    if (fabsf(cell_vs[i] - stats->median) > CELL_V_UNBAL_THRES)
      stats->n_unbal++;
}

void gui_bat_draw_data(void) {
  lcd_textbox_prep(GUI_BAT_X, GUI_BAT_Y + GUI_BAT_H - TITLE_H,
		   GUI_BAT_W - TITLE_W, TITLE_H, LCD_DARKGREY);
  lcd_printf(LCD_WHITE, &DATA_FONT, "%d packs", packs_talking);
  lcd_textbox_show();
  
  //  lcd_textbox_prep(0, LCD_H - 60, 480, 60, LCD_DARKGREY);
  ltc6804_get_voltages(packs_talking, voltages);

  static int hist[HIST_N_BINS];
  bat_stats_t stats;
  bat_calc_stats(hist, voltages, packs_talking * N_CELLS_PER_PACK, &stats);
  float v_bin_max = HIST_LO_BIN_V;
  for (int i = 0; i < HIST_N_BINS; i++) {
    float v_bin_min = v_bin_max;
    v_bin_max += HIST_BIN_V_SPAN;
    uint16_t color = LCD_GREEN;
    if (v_bin_min <= CELL_V_LO_RED || v_bin_max >= CELL_V_HI_RED)
      color = LCD_RED;
    else if (v_bin_min <= CELL_V_LO_YEL || v_bin_max >= CELL_V_HI_YEL)
      color = LCD_YELLOW;
    lcd_fill_rect(GUI_BAT_X + 2, GUI_BAT_Y + GUI_BAT_H - TITLE_H - (1 + i), hist[i], 1, color);
    //    lcd_fill_rect(GUI_BAT_X + 2, GUI_BAT_Y + TITLE_H + 1 + i, hist[i], 1, LCD_DARKGREY);
  }
  
  //  lcd_textbox_show();
}

void debug_bat() {

  //  ltc6804_wakeup();

  int pec_fail = ltc6804_get_voltages(packs_talking, voltages) << 8;
  pec_fail |= ltc6804_get_temps(packs_talking, temperatures);
  for (int i = 0; i < packs_talking; i++) {
    lcd_textbox_prep(0, i*30, 480, 30, LCD_BLACK);
    lcd_printf(LCD_WHITE, &DEBUG_FONT,
	       "Pack %d: %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
	       i,
	       voltages[i*N_CELLS_PER_PACK + 0],
	       voltages[i*N_CELLS_PER_PACK + 1],
	       voltages[i*N_CELLS_PER_PACK + 2],
	       voltages[i*N_CELLS_PER_PACK + 3],
	       voltages[i*N_CELLS_PER_PACK + 4],
	       voltages[i*N_CELLS_PER_PACK + 5],
	       voltages[i*N_CELLS_PER_PACK + 6],
	       voltages[i*N_CELLS_PER_PACK + 7],
	       voltages[i*N_CELLS_PER_PACK + 8],
	       voltages[i*N_CELLS_PER_PACK + 9],
	       voltages[i*N_CELLS_PER_PACK + 10],
	       voltages[i*N_CELLS_PER_PACK + 11]
	       );
    
    lcd_printf(LCD_WHITE, &DEBUG_FONT,
	       " Temps: %2d %2d %2d %2d %2d %2d  PEC 0x%04X",
	       (int)temperatures[i*N_TEMPS_PER_PACK + 0],
	       (int)temperatures[i*N_TEMPS_PER_PACK + 1],
	       (int)temperatures[i*N_TEMPS_PER_PACK + 2],
	       (int)temperatures[i*N_TEMPS_PER_PACK + 3],
	       (int)temperatures[i*N_TEMPS_PER_PACK + 4],
	       (int)temperatures[i*N_TEMPS_PER_PACK + 5],
	       pec_fail);
    lcd_textbox_show();
  }
}
