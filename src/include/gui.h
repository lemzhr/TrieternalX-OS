
#ifndef GUI_H
#define GUI_H

#include "types.h"

void gui_start();

void gui_stop();

void gui_handle_key(uint8_t scancode, char c);

void gui_render();

bool is_in_gui_mode();

#endif
