#include "gui_mc.h"
#include "mc_telem.h"
#include "font.h"
#include "lcd.h"
#include <stdio.h>
#include <math.h>

#define TITLE_H 14
#define TITLE_FONT Roboto_Bold7pt7b
#define DATA_FONT RobotoCondensed_Regular7pt7b

#define TITLE_W 52 // (GUI_MC_W * 0.36)
#define DATA_W 46 //(GUI_MC_W * 0.32)

int gui_mc_draw_frame(void) {
  if (GUI_MC_X < 0 || GUI_MC_Y < 0 || GUI_MC_X + GUI_MC_W > LCD_W || GUI_MC_Y + GUI_MC_H > LCD_H)
    return -1;
  lcd_textbox_prep(GUI_MC_X, GUI_MC_Y, GUI_MC_W, TITLE_H, LCD_DARKGREY);
  lcd_textbox_move_cursor(TITLE_W, 0, 1);
  lcd_printf(LCD_WHITE, &TITLE_FONT, "Left");
  lcd_textbox_move_cursor(TITLE_W + DATA_W, 10, 0);
  lcd_printf(LCD_WHITE, &TITLE_FONT, "Right");
  lcd_textbox_show();
  lcd_textbox_prep(GUI_MC_X, GUI_MC_Y + TITLE_H, TITLE_W, GUI_MC_H - TITLE_H, LCD_DARKGREY);
  lcd_printf(LCD_WHITE, &TITLE_FONT, "Bus V:\nBus I:\nTorque:\nRPM:\nT_SiC:\nT_Mot:\nCmd:\nMod:\nState:\nFault:\n");
  lcd_textbox_show();
  lcd_fill_rect(GUI_MC_X + TITLE_W, GUI_MC_Y + TITLE_H, DATA_W * 2, GUI_MC_H - TITLE_H, LCD_DARKGREY);
  return 0;
}

void gui_mc_draw_data(void) {
  const char *motor_state_names[] = {"Init", "Fault", "Disable", "Ready", "Soft", "SoftIrq", "FlyStrt", "Torque", "Offset", "Speed"};

  for (mc_id_e i = MC_LEFT; i < N_MCS; i++) {
    mc_telem_t *data = &mc_telem[i];
    lcd_textbox_prep(GUI_MC_X + TITLE_W + DATA_W * i, GUI_MC_Y + TITLE_H, DATA_W, GUI_MC_H - TITLE_H, LCD_DARKGREY);
    lcd_textbox_move_cursor(2, 11, 0);
    lcd_printf(LCD_CYAN, &DATA_FONT, "%.0f\n%.1f\n%.1f\n%.0f\n%.1f\n%.1f\n%.0f%%\n%.0f%%\n%s\n",
               data->bus_v, data->bus_i, data->torque, data->rpm, data->temp_SiC, data->temp_motor, 100 * data->cmd, 100 * data->mod_index,
               motor_state_names[data->state]);
    if (data->faults.all) {
      // TODO: better fault description
      if (data->faults.flags.uvlo)
        lcd_printf(LCD_RED, &DATA_FONT, "U");
      if (data->faults.flags.ovlo)
        lcd_printf(LCD_RED, &DATA_FONT, "V");
      if (data->faults.flags.oclo)
        lcd_printf(LCD_RED, &DATA_FONT, "C");
      if (data->faults.flags.over_temp)
        lcd_printf(LCD_RED, &DATA_FONT, "T");
      if (data->faults.flags.hw_fault)
        lcd_printf(LCD_RED, &DATA_FONT, "H");
      if (data->faults.flags.overspeed)
        lcd_printf(LCD_RED, &DATA_FONT, "S");
    } else {
      lcd_printf(LCD_CYAN, &DATA_FONT, "-");
    }
    lcd_textbox_show();
  }
}
