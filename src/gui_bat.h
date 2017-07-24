#ifndef GUI_BAT__H
#define GUI_BAT__H

#include "lcd.h"

#define GUI_BAT_W 144
#define GUI_BAT_H 216

#define GUI_BAT_X (LCD_W - GUI_BAT_W)
#define GUI_BAT_Y 0

int gui_bat_draw_frame(void);
void gui_bat_draw_data(void);


#endif // GUI_BAT__H
