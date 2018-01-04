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

float fake_cell_vs[3*9*12] = {
 3.72152725,  3.70739718,  3.73211931,  3.69695631,  3.6880425 ,
 3.70438583,  3.68669677,  3.7204485 ,  3.70540629,  3.69243301,
 3.70823778,  3.69448366,  3.69154265,  3.68079651,  3.67968894,
 3.69797964,  3.6760198 ,  3.6833918 ,  3.71009828,  3.69245487,
 3.70235021,  3.67901321,  3.69637716,  3.69784802,  3.71031206,
 3.70347439,  3.68449869,  3.67600262,  3.715044  ,  3.70401877,
 3.69776376,  3.69376691,  3.68355683,  3.66828319,  3.70791542,
 3.69428889,  3.70035317,  3.70287276,  3.70628392,  3.683091  ,
 3.71735824,  3.70531096,  3.69219178,  3.68536491,  3.71402219,
 3.70333368,  3.68422024,  3.71348634,  3.70594451,  3.71571214,
 3.68536682,  3.70792825,  3.69643382,  3.69320465,  3.71557348,
 3.67761642,  3.69012626,  3.67001395,  3.71550052,  3.69570786,
 3.68101109,  3.71216362,  3.67102038,  3.71617301,  3.69272611,
 3.71328509,  3.69250293,  3.69174231,  3.70961051,  3.71440396,
 3.71002832,  3.68691666,  3.65407595,  3.71117926,  3.71793204,
 3.71702018,  3.71196694,  3.6708248 ,  3.73241778,  3.7166356 ,
 3.70779109,  3.71100037,  3.67546826,  3.70338612,  3.70395713,
 3.67997504,  3.70948949,  3.69039826,  3.70563758,  3.71097914,
 3.68144663,  3.69588665,  3.70113616,  3.70562801,  3.70225666,
 3.68707091,  3.68621145,  3.67326303,  3.6933207 ,  3.72392398,
 3.70015775,  3.67779663,  3.68952366,  3.68474961,  3.71656875,
 3.69608285,  3.70681998,  3.69801853,  3.72941919,  3.70817894,
 3.70960692,  3.68197261,  3.68854601,  3.69826303,  3.70192473,
 3.70638251,  3.68908411,  3.69125119,  3.71719324,  3.6887269 ,
 3.70196789,  3.67245288,  3.69129175,  3.70635565,  3.67997244,
 3.67616607,  3.70039042,  3.69611061,  3.68169568,  3.70520703,
 3.71400949,  3.69964315,  3.71655355,  3.71734032,  3.69851525,
 3.70402198,  3.67430598,  3.68787227,  3.70850199,  3.71050851,
 3.72126824,  3.71494289,  3.70718717,  3.7083956 ,  3.67116284,
 3.72935574,  3.71417239,  3.71448891,  3.69759041,  3.71580324,
 3.71425253,  3.69062762,  3.6855235 ,  3.71097404,  3.71151423,
 3.70196008,  3.69602724,  3.71366075,  3.69468716,  3.68140146,
 3.68644334,  3.70756135,  3.69804583,  3.7000263 ,  3.6799845 ,
 3.6905137 ,  3.71121318,  3.70826636,  3.68119692,  3.69803686,
 3.69835379,  3.70939348,  3.70715558,  3.69083307,  3.7072639 ,
 3.68596666,  3.67734453,  3.70019853,  3.69807927,  3.7045145 ,
 3.7095142 ,  3.71283092,  3.70784443,  3.69292999,  3.71672436,
 3.7095535 ,  3.67628125,  3.67841163,  3.70353973,  3.70114543,
 3.72370018,  3.68084659,  3.68522149,  3.70529659,  3.69137561,
 3.70571202,  3.69837241,  3.71277349,  3.6959368 ,  3.70383303,
 3.70181672,  3.72045704,  3.71740247,  3.74314212,  3.69217507,
 3.68761608,  3.71422407,  3.68359224,  3.70511767,  3.7095    ,
 3.70591957,  3.69381939,  3.70818391,  3.71217779,  3.70232284,
 3.70863051,  3.69711787,  3.70843633,  3.70163401,  3.7003573 ,
 3.68185824,  3.69029002,  3.7176008 ,  3.70894589,  3.70112083,
 3.74599626,  3.72865682,  3.71752405,  3.71537562,  3.69025079,
 3.71759135,  3.68856543,  3.73264733,  3.69736765,  3.71568563,
 3.68965388,  3.71542146,  3.69397972,  3.69951657,  3.71488194,
 3.71329404,  3.70593957,  3.70174667,  3.71149092,  3.72727091,
 3.70901317,  3.69901298,  3.71083701,  3.70809773,  3.70565702,
 3.69694228,  3.68480209,  3.704597  ,  3.71545061,  3.70535327,
 3.71418199,  3.71315449,  3.70654004,  3.6857349 ,  3.71336972,
 3.70804332,  3.67800111,  3.69310693,  3.71193609,  3.7218006 ,
 3.70399474,  3.69711004,  3.70884788,  3.73346536,  3.6801428 ,
 3.72594052,  3.67943933,  3.70737307,  3.74248204,  3.7199339 ,
 3.73784097,  3.66673706,  3.69554502,  3.67879158,  3.68975103,
 3.7032993 ,  3.6960417 ,  3.69493012,  3.67977907,  3.69494016,
 3.71575153,  3.7153045 ,  3.70062571,  3.70978081,  3.68259875,
 3.71526762,  3.6669681 ,  3.68013514,  3.70831525,  3.69529702,
 3.67675314,  3.72708257,  3.67861479,  3.69794106,  3.72280853,
 3.90384505,  3.90070816,  3.90884372,  3.8891619 ,  3.90197318,
 3.90104429,  3.90512356,  3.90454529,  3.8714543 ,  3.89131281,
 3.90858658,  3.90140436,  3.90386343,  4.08361063,  3.54737353,
 2.9521586 ,  3.81884047,  3.87425326,  3.94342013,  3.61097572,
 0.        ,  6.5535    ,  6.5535    ,  6.5535
};


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
  ltc6804_get_voltages(packs_talking);

  static int hist[HIST_N_BINS];
  bat_stats_t stats;
  bat_calc_stats(hist, fake_cell_vs, 324, &stats);
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
