#ifndef GUI_MAIN__H
#define GUI_MAIN__H

#include "gui_mc.h"
#include "lcd.h"

#define GUI_MAIN_W 192
#define GUI_MAIN_H 216

#define GUI_MAIN_X ((LCD_W - GUI_MAIN_W) / 2)
#define GUI_MAIN_Y 0

int gui_main_draw_frame(void);
void gui_main_draw_data(void);

#endif // GUI_MAIN__H
