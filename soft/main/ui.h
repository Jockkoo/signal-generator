#ifndef UI_H
#define UI_H

#include "keypad.h"

void
ui_init(void);

void
ui_handle_key(key_t key);

void
ui_render(void);

#endif
