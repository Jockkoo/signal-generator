#include "ui.h"

#include <inttypes.h>
#include <math.h>

#include "ctl.h"
#include "lcd.h"

typedef enum ui_menu {
    UI_MENU_HOME = 0,
    UI_MENU_TYPE,
    UI_MENU_FREQ,
    UI_MENU_OFFSET,
} ui_menu_t;

static struct g {
    ui_menu_t menu;
    bool enabled;

    struct {
        i32 sign;
        i32 whole, frac;
        i32 frac_count;
        bool dot;
    } pending;

    ctl_signal_type_t type;
    gen_params_t params;
    gen_error_t err;
} g;

void
ui_init(void) {
    g.menu = UI_MENU_HOME;
    g.type = CTL_SIGNAL_TYPE_SINE;
}

static void
go_home(void) {
    g.menu = UI_MENU_HOME;
    g.pending.whole = 0;
    g.pending.frac = 0;
    g.pending.frac_count = 0;
    g.pending.dot = false;
    g.pending.sign = 1;
}

static void
handle_digit(i32 d) {
    if(g.menu == UI_MENU_HOME) {
        if(d == 1) {
            g.menu = UI_MENU_TYPE;
        } else if(d == 2) {
            g.menu = UI_MENU_FREQ;
        } else if(d == 3) {
            g.menu = UI_MENU_OFFSET;
        }
    } else if(g.menu == UI_MENU_TYPE && d > CTL_SIGNAL_TYPE_NONE && d < _CTL_SIGNAL_TYPE_COUNT) {
        g.type = d;
        go_home();
    } else if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET) {
        if(g.pending.dot) {
            g.pending.frac_count++;
            g.pending.frac = g.pending.frac * 10 + d;
        } else {
            g.pending.whole = g.pending.whole * 10 + d;
        }
    }
}

void
ui_render(void) {
    lcd_tclear();

    switch(g.menu) {
        case UI_MENU_HOME: {
            lcd_tprintf(0, 0, "1.type:%s", ctl_signal_type_to_string[g.type]);
            lcd_tprintf(1, 0, "2.freq:%" PRIi32, g.params.freq);
            lcd_tprintf(2, 0, "3.offs:%.2f", g.params.offset);

            break;
        }
        case UI_MENU_TYPE: {
            lcd_tprintf(0, 0, "select type:");

            for(size_t i = 1; i < _CTL_SIGNAL_TYPE_COUNT; i++) {
                lcd_tprintf(i, 0, "%d. %s", i, ctl_signal_type_to_string[i]);
            }

            break;
        }
        case UI_MENU_FREQ: {
            lcd_tprintf(0, 0, "type freq:");
            lcd_tprintf(1, 0, "> %d", g.pending.whole);

            break;
        }

        case UI_MENU_OFFSET: {
            lcd_tprintf(0, 0, "type offset:");

            if(g.pending.dot) {
                float value = g.pending.sign * (g.pending.whole + g.pending.frac / pow(10, g.pending.frac_count));
                lcd_tprintf(1, 0, "> %.2f", value);
            } else {
                i32 value = g.pending.sign * g.pending.whole;
                lcd_tprintf(1, 0, "> %" PRIi32, value);
            }

            break;
        }
    }
}

void
ui_handle_key(key_t key) {
    printf("key: %d\n", key);

    switch(key) {
        case KEY_NONE: {
            break;
        }
        case KEY_ENABLE: {
            if(g.enabled) {
                g.enabled = false;
                ctl_disable();
            } else {
                g.err = ctl_enable(g.type, &g.params);
                if(g.err) {
                    // set the asterixes here
                }

                g.enabled = true;
                go_home();
            }

            break;
        }
        case KEY_OK: {
            if(g.menu == UI_MENU_FREQ) {
                g.params.freq = g.pending.whole;
                go_home();
            } else if(g.menu == UI_MENU_OFFSET) {
                // double check this
                g.params.offset = g.pending.sign * (g.pending.whole + g.pending.frac / pow(10, g.pending.frac_count));
                go_home();
            }

            break;
        }
        case KEY_CANCEL: {
            go_home();

            break;
        }
        case KEY_BACKSLASH: {
            if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET) {
                if(g.pending.dot) {
                    if(g.pending.frac_count > 0) {
                        g.pending.frac /= 10;
                        g.pending.frac_count--;
                    } else {
                        g.pending.dot = false;
                    }
                } else {
                    g.pending.whole /= 10;
                }
            }

            break;
        }
        case KEY_SIGN: {
            if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET) {
                g.pending.sign *= -1;
            }

            break;
        }
        case KEY_DOT: {
            if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET) {
                g.pending.dot = true;
            }

            break;
        }

        default: {
            // only digits are left
            handle_digit(key);
            break;
        }
    }

    ui_render();
}
