#include "ctl.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "generators/rect_gen.h"
#include "generators/sine_gen.h"
#include "generators/triangle_gen.h"
#include "main.h"

static const char *tag = "CTL";

const char *ctl_signal_type_to_string[] = {
        [CTL_SIGNAL_TYPE_NONE] = "none",
        [CTL_SIGNAL_TYPE_SINE] = "sine",
        [CTL_SIGNAL_TYPE_RECT] = "rect",
        [CTL_SIGNAL_TYPE_TRIANGLE] = "tri",
};

static struct g {
    bool enabled;

    ctl_signal_type_t type;

    // map of signal types to appropriate generators bases, see `ctl_init()`
    gen_t *signal_type_to_gen[_CTL_SIGNAL_TYPE_COUNT];

    sine_gen_t sine_gen;
    rect_gen_t rect_gen;
    dac_dma_gen_t triangle_gen;
} g;

void
ctl_init(void) {
    sine_gen_init(&g.sine_gen);
    rect_gen_init(&g.rect_gen);
    triangle_gen_init(&g.triangle_gen);

    g.signal_type_to_gen[CTL_SIGNAL_TYPE_SINE] = &g.sine_gen.base_gen;
    g.signal_type_to_gen[CTL_SIGNAL_TYPE_RECT] = &g.rect_gen.base_gen;
    g.signal_type_to_gen[CTL_SIGNAL_TYPE_TRIANGLE] = &g.triangle_gen.base_gen;
}

u32
ctl_enable(ctl_signal_type_t type, gen_params_t *params) {
    assert(!g.enabled);

    if(type == CTL_SIGNAL_TYPE_NONE) {
        return GEN_ERROR_UNKNOWN;
    }

    // reset the pin just to be sure
    gpio_reset_pin(O_SIGNAL);
    gen_t *gen = g.signal_type_to_gen[type];

    ESP_LOGI(tag, "starting %s generator with parametars - freq: %" PRIi32 ", symmetry: %.2f, offset: %0.2f",
            ctl_signal_type_to_string[type], params->freq, params->symmetry, params->offset);
    params->symmetry = 0.5;
    u32 err = gen_start(gen, params);
    if(err) {
        ESP_LOGI(tag, "generator error: %" PRIu32, err);
        return err;
    }

    ESP_LOGI(tag, "started generator successfully");
    g.enabled = true;
    g.type = type;
    return GEN_ERROR_NONE;
}

void
ctl_disable(void) {
    gen_t *current = g.signal_type_to_gen[g.type];

    ESP_LOGI(tag, "generator stopped");
    esp_err_t err = gen_stop(current);
    if(err) {
        // TODO: better error handling here, currently we get into a situation where nothing is handled as it should be
        return;
    }

    g.enabled = false;
}
