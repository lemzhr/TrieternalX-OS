
#ifndef MOUSE_H
#define MOUSE_H

#include "types.h"

void mouse_init();
int32_t mouse_get_x();
int32_t mouse_get_y();
bool mouse_is_left_clicked();
bool mouse_is_right_clicked();

void draw_mouse_cursor_vbe(int x, int y);
void reset_mouse_cursor_vbe();

#endif
