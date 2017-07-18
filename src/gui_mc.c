#include "gui_mc.h"
#include "font.h"
#include "lcd.h"
#include <stdio.h>
#include <math.h>

int gui_mc_init(gui_mc *handle, int x, int y) {
  if (x < 0 || y < 0 || x + GUI_MC_W > LCD_W || y + GUI_MC_H > LCD_H)
    return -1;
  handle->x = x;
  handle->y = y;
  lcd_textbox_prep(x, y, GUI_MC_W * 0.5, GUI_MC_H, LCD_DARKGREY);
  lcd_printf(LCD_WHITE, &FreeSans9pt7b, "Bus V:\nBus I:\nTorque:\nRPM:\nCmd:\nMod:\nState:\nFault:\nComm:\n");
  lcd_textbox_show();
  lcd_fill_rect(x + GUI_MC_W * 0.5, y, GUI_MC_W * 0.5, GUI_MC_H, LCD_DARKGREY);
  return 0;
}

void gui_mc_update(gui_mc *handle, const mc_gui_data_t *data) {
  const char *motor_state_names[] = {"Init", "Fault", "Disable", "Ready", "Soft", "SoftIrq", "FlyStrt", "Torque", "Offset", "Speed"};

  if (data == NULL)
    return;

  lcd_textbox_prep(handle->x + GUI_MC_W * 0.5, handle->y + 1, GUI_MC_W * 0.5, GUI_MC_H - 1, LCD_DARKGREY);
  lcd_printf(LCD_CYAN, &FreeSansBold9pt7b, "%.0f\n%.1f\n%.1f\n%.0f\n%.0f%%\n%.0f%%\n%s\n",
             data->bus_v, data->bus_i, data->torque, data->rpm, 100 * data->cmd, 100 * data->mod_index,
             motor_state_names[data->state]);
  if (data->faults.all) {
    // TODO: better fault description
    if (data->faults.flags.uvlo)
      lcd_printf(LCD_RED, &FreeSans9pt7b, "U");
    if (data->faults.flags.ovlo)
      lcd_printf(LCD_RED, &FreeSans9pt7b, "V");
    if (data->faults.flags.oclo)
      lcd_printf(LCD_RED, &FreeSans9pt7b, "C");
    if (data->faults.flags.over_temp)
      lcd_printf(LCD_RED, &FreeSans9pt7b, "T");
    if (data->faults.flags.hw_fault)
      lcd_printf(LCD_RED, &FreeSans9pt7b, "H");
    if (data->faults.flags.overspeed)
      lcd_printf(LCD_RED, &FreeSans9pt7b, "S");
  } else {
    lcd_printf(LCD_CYAN, &FreeSans9pt7b, "-");
  }
  if (data->age == 0) {
    lcd_printf(LCD_CYAN, &FreeSansBold9pt7b, "\nOK\n");
  } else {
    lcd_printf(LCD_RED, &FreeSansBold9pt7b, "\n%d:%02d\n", (int)(data->age / 60), (int)(data->age % 60));
  }
  lcd_textbox_show();
}
