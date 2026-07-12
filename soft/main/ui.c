#include "ui.h"

#include <inttypes.h>
#include <math.h>

#include "ctl.h"
#include "lcd.h"
#include "util/constants.h"
#include "util/macros.h"

const u32 adjustement_vals[4][2] = {
    {}, // Because we index this with CLT_SIGNAL_... and CTL_SIGNAL_NONE = 0
    {MIN_SIN_FREQ, MAX_SINE_FREQ},
    {MIN_RECT_FREQ, MAX_RECT_FREQ},
    {MIN_TRI_FREQ, MAX_TRI_FREQ},
};

typedef enum ui_menu {
    UI_MENU_HOME = 0,
    UI_MENU_TYPE,
    UI_MENU_FREQ,
    UI_MENU_OFFSET,
    UI_MENU_SIMMETRY,
    UI_MENU_CHANGED_PARAM,
} ui_menu_t;

typedef enum adjusted_param {
    ADJ_MIN = 0,
    ADJ_MAX,
    ADJ_FREQ,
    ADJ_SYMM,
    ADJ_OFF,
    ADJ_NONE,
} adjusted_param_t;

const char* adj_param_to_string[] = {
    [ADJ_MIN] = "min",
    [ADJ_MAX] = "max",
    [ADJ_FREQ] = "frequency",
    [ADJ_SYMM] = "symmetry",
    [ADJ_OFF] = "offset",
};

static struct g {
    ui_menu_t menu;
    bool enabled;
    adjusted_param_t adj_m;
    adjusted_param_t adj_fso;

    struct {
        i32 sign;
        i32 whole, frac;
        i32 frac_count;
        bool dot;
        // do sada cim bi kucao broj je ostala nula, sada moras da ukucas nulu, izgleda lepse...
        bool zero;
    } pending;

    ctl_signal_type_t type;
    gen_params_t params;
    gen_error_t err;
} g;

static void
go_home(void) {
    g.menu = UI_MENU_HOME;
    g.adj_m = ADJ_NONE;
    g.adj_fso = ADJ_NONE;
    g.pending.whole = 0;
    g.pending.frac = 0;
    g.pending.frac_count = 0;
    g.pending.dot = false;
    g.pending.zero = false;
    g.pending.sign = 1;
}

void
ui_init(void) {
    g.type = CTL_SIGNAL_TYPE_SINE;
    go_home();
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
        } else if(d == 4) {
            g.menu = UI_MENU_SIMMETRY;
        }
    } else if(g.menu == UI_MENU_TYPE && d > CTL_SIGNAL_TYPE_NONE && d < _CTL_SIGNAL_TYPE_COUNT) {
        g.type = d;
        // We can decide if we want all settings to reset uppon signal type change
        g.params.freq = 0;
        g.params.offset = 0;
        g.params.symmetry = 0;
        go_home();
    } else if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET || g.menu == UI_MENU_SIMMETRY) {
        if(g.pending.dot) {
            g.pending.frac_count++;
            g.pending.frac = g.pending.frac * 10 + d;
        } else {
            g.pending.whole = g.pending.whole * 10 + d;
        }
        if (!d) {
            g.pending.zero = true;
        }
    }
}

static void
print_decimal() {
    float value = g.pending.sign * (g.pending.whole + g.pending.frac / pow(10, g.pending.frac_count));
    lcd_tprintf(1, 0, ">%c", (g.pending.sign < 0) ? '-':' ');

    if (value || g.pending.zero) {
        if (g.pending.frac_count == 0) {
            lcd_tprintf(1, 2, "%d%s", (value < 0) ? ((i32)-value) : ((i32)value), (g.pending.dot) ? "." : "");
        } else if (g.pending.frac_count == 1) {
            lcd_tprintf(1, 2, "%.1f", (value < 0) ? (-value) : (value));
        } else {
            lcd_tprintf(1, 2, "%.2f", (value < 0) ? (-value) : (value));
        }
    }
}

// Ove tri funkcije su skoro identicne, mrzelo me je da smislim nesto pametnije

static void
verify_freq() {
    if (g.params.freq < adjustement_vals[g.type][0]) {
        g.params.freq = adjustement_vals[g.type][0];
        g.adj_m = ADJ_MIN;
        g.adj_fso = ADJ_FREQ;
        g.menu = UI_MENU_CHANGED_PARAM;
    } else if (g.params.freq > adjustement_vals[g.type][1]) {
        g.params.freq = adjustement_vals[g.type][1];
        g.adj_m = ADJ_MAX;
        g.adj_fso = ADJ_FREQ;
        g.menu = UI_MENU_CHANGED_PARAM;
    } else {
        g.menu = UI_MENU_HOME;
        go_home();
    }
}

static void
verify_offset() {
    if (g.params.offset < -MAX_OFFSET) {
        g.params.offset = -MAX_OFFSET;
        g.adj_m = ADJ_MIN;
        g.adj_fso = ADJ_OFF;
        g.menu = UI_MENU_CHANGED_PARAM;
    } else if (g.params.offset > MAX_OFFSET) {
        g.params.offset = MAX_OFFSET;
        g.adj_m = ADJ_MAX;
        g.adj_fso = ADJ_OFF;
        g.menu = UI_MENU_CHANGED_PARAM;
    } else {
        g.menu = UI_MENU_HOME;
        go_home();
    }
}

static void
verify_symmetry() {
    if (g.params.symmetry < 0) {
        g.params.symmetry = 0;
        g.adj_m = ADJ_MIN;
        g.adj_fso = ADJ_SYMM;
        g.menu = UI_MENU_CHANGED_PARAM;
    } else if (g.params.symmetry > 1) {
        g.params.symmetry = 1;
        g.adj_m = ADJ_MAX;
        g.adj_fso = ADJ_SYMM;
        g.menu = UI_MENU_CHANGED_PARAM;
    } else {
        g.menu = UI_MENU_HOME;
        go_home();
    }
}


static void // Sorry ako je nepregledno, al poenta je da samo jedna funkcija moze svaki "error" da ispise
ui_adjusted_param_screen() {
    lcd_tprintf(0, 0, "%s %s", adj_param_to_string[g.adj_m], adj_param_to_string[g.adj_fso]);
    lcd_tprintf(1, 0, "for %s is", ctl_signal_type_to_string[g.type]);
    if (g.adj_fso == ADJ_FREQ) {
        lcd_tprintf(2, 0, "%d", adjustement_vals[g.type][g.adj_m]);
    } else if (g.adj_fso == ADJ_OFF) {
        lcd_tprintf(2, 0, "%d", (g.adj_m == ADJ_MIN) ? -12 : 12);
    } else if (g.adj_fso == ADJ_SYMM) {
        lcd_tprintf(2, 0, "%d", (g.adj_m == ADJ_MIN) ? 0 : 1);
    }
    lcd_tprintf(3, 0, "Click OK to adj");
}


void
ui_render(void) {
    lcd_tclear();

    switch(g.menu) {
        case UI_MENU_HOME: {
            lcd_tprintf(0, 0, "1.type:%s", ctl_signal_type_to_string[g.type]);
            lcd_tprintf(1, 0, "2.freq:%" PRIi32, g.params.freq);
            lcd_tprintf(2, 0, "3.offs:%.2f", g.params.offset);
            if (g.type != CTL_SIGNAL_TYPE_SINE) {
                lcd_tprintf(3, 0, "4.sim:%.2f", g.params.symmetry);
            }
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
            lcd_tprintf(0, 0, "type frequency");
            lcd_tprintf(1, 0, "> ", g.pending.whole);
            if (g.pending.whole) {
                lcd_tprintf(1, 2, "%d", g.pending.whole);
            }

            break;
        }

        case UI_MENU_OFFSET: {
            lcd_tprintf(0, 0, "type offset");

            print_decimal();

            break;
        }
        case UI_MENU_SIMMETRY: {
            lcd_tprintf(0, 0, "type symmetry");

            print_decimal();

            break;

        }
        case UI_MENU_CHANGED_PARAM: {
            ui_adjusted_param_screen();
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
                verify_freq(); // Prvo ovde verifikujemo vrednosti i clampujemo ih ako treba
            } else if(g.menu == UI_MENU_OFFSET) {
                // double check this
                g.params.offset = g.pending.sign * (g.pending.whole + g.pending.frac / pow(10, g.pending.frac_count));
                verify_offset();
            } else if(g.menu == UI_MENU_SIMMETRY) {
                g.params.symmetry = g.pending.sign * (g.pending.whole + g.pending.frac / pow(10, g.pending.frac_count));
                verify_symmetry();
            } else if(g.menu == UI_MENU_CHANGED_PARAM) {
                go_home();
            }

            break;
        }
        case KEY_CANCEL: {
            go_home();

            break;
        }
        case KEY_BACKSLASH: {
            if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET || g.menu == UI_MENU_SIMMETRY) {
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
            if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET || g.menu == UI_MENU_SIMMETRY) {
                g.pending.sign *= -1;
            }

            break;
        }
        case KEY_DOT: {
            if(g.menu == UI_MENU_FREQ || g.menu == UI_MENU_OFFSET || g.menu == UI_MENU_SIMMETRY) {
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
