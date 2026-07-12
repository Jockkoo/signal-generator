#include "rect_gen.h"

#include <inttypes.h>

#include "driver/ledc.h"
#include "gen.h"
#include "main.h"

static u32
rect_gen_start(gen_t *base_gen, gen_params_t *params) {
    u32 err = GEN_ERROR_NONE;

    // Sklonio sam verifikaciju parametara jer se sada uvek clampuju na granicne vrednosti

    ledc_timer_config_t timer_conf = {
                .speed_mode = LEDC_HIGH_SPEED_MODE,
                .timer_num = LEDC_TIMER_0,
                .duty_resolution = LEDC_TIMER_8_BIT,
                .freq_hz = params->freq,
                .clk_cfg = LEDC_AUTO_CLK,
    };

    err = ledc_timer_config(&timer_conf);
    if(err) {
        return GEN_ERROR_UNKNOWN;
    }

    ledc_channel_config_t channel_conf = {
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .channel = LEDC_CHANNEL_0,
            .timer_sel = LEDC_TIMER_0,
            .gpio_num = O_SIGNAL,
            .duty = params->symmetry * 255,
            .hpoint = 0,
    };

    err = ledc_channel_config(&channel_conf);
    if(err) {
        return GEN_ERROR_UNKNOWN;
    }

    return GEN_ERROR_NONE;
}

static esp_err_t
rect_gen_stop(gen_t *base_gen) {
    return ledc_stop(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, 0);
}

static const gen_interface_t rect_gen_impl = {
        .start = rect_gen_start,
        .stop = rect_gen_stop,
};

void
rect_gen_init(rect_gen_t *gen) {
    gen_init(&gen->base_gen, &rect_gen_impl);
}
