#include "gen.h"

#include "driver/gpio.h"
#include "main.h"

void
gen_init(gen_t *gen, const gen_interface_t *impl) {
    gen->impl = impl;
}

u32
gen_start(gen_t *gen, gen_params_t *params) {
    u32 err = gpio_reset_pin(O_SIGNAL);
    if(err) {
        return err;
    }

    err = gen->impl->start(gen, params);
    if(err) {
        return err;
    }

    return GEN_ERROR_NONE;
}

esp_err_t
gen_stop(gen_t *gen) {
    esp_err_t err = gen->impl->stop(gen);
    if(err) {
        return err;
    }

    // ground the pin so there is no residue output
    err = gpio_reset_pin(O_SIGNAL);
    if(err) {
        return err;
    }

    err = gpio_set_direction(O_SIGNAL, GPIO_MODE_OUTPUT);
    if(err) {
        return err;
    }

    return gpio_set_level(O_SIGNAL, 0);
}
