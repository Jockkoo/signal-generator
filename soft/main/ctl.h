#ifndef CTL_H
#define CTL_H

#include "generators/gen.h"

typedef enum ctl_signal_type {
    CTL_SIGNAL_TYPE_NONE = 0,
    CTL_SIGNAL_TYPE_SINE,
    CTL_SIGNAL_TYPE_RECT,
    CTL_SIGNAL_TYPE_TRIANGLE,
    _CTL_SIGNAL_TYPE_COUNT,
} ctl_signal_type_t;

extern const char *ctl_signal_type_to_string[_CTL_SIGNAL_TYPE_COUNT];

void
ctl_init(void);

// returns errors if any
u32
ctl_enable(ctl_signal_type_t type, gen_params_t *params);

void
ctl_disable(void);

#endif
