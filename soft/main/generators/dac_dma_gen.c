#include "dac_dma_gen.h"

#include <inttypes.h>

#include "driver/dac_continuous.h"
#include "util/macros.h"

static inline u32
get_points_count(u32 freq) {
    u32 count = DAC_DMA_GEN_BUFFER_SIZE;

    // we make sure that the total frequency of the dac wont we greater than 1MHz
    while(count * freq > 2E6) {
        count /= 2;
    }

    return count;
}

static u32
dac_dma_gen_start(gen_t *base_gen, gen_params_t *params) {
    dac_dma_gen_t *gen = CONTAINER_OF(base_gen, dac_dma_gen_t, base_gen);

    u32 count = get_points_count(params->freq);

    // generate the points from the impl
    u32 err = gen->generate_points(gen->buffer, count, params);
    if(err) {
        return err;
    }

    dac_continuous_config_t conf = {
            .chan_mask = DAC_CHANNEL_MASK_CH0,
            // these are by default such, may need to change them
            .desc_num = 8,
            .buf_size = 2048,
            .freq_hz = count * params->freq,
            .offset = 0,
            // TODO: try out different clock setting to get around the minimum freq
            .clk_src = DAC_DIGI_CLK_SRC_APLL,
    };

    err = dac_continuous_new_channels(&conf, &gen->dac_handle);
    if(err) {
        return GEN_ERROR_UNKNOWN;
    }

    err = dac_continuous_enable(gen->dac_handle);
    if(err) {
        dac_continuous_del_channels(gen->dac_handle);
        return GEN_ERROR_UNKNOWN;
    }

    err = dac_continuous_write_cyclically(gen->dac_handle, gen->buffer, count, NULL);
    if(err) {
        dac_continuous_disable(gen->dac_handle);
        dac_continuous_del_channels(gen->dac_handle);
        return GEN_ERROR_UNKNOWN;
    }

    return GEN_ERROR_NONE;
}

static esp_err_t
dac_dma_gen_stop(gen_t *base_gen) {
    dac_dma_gen_t *gen = CONTAINER_OF(base_gen, dac_dma_gen_t, base_gen);

    esp_err_t err = dac_continuous_disable(gen->dac_handle);
    if(err) {
        return err;
    }

    return dac_continuous_del_channels(gen->dac_handle);
}

static const gen_interface_t dac_dma_gen_impl = {
        .start = dac_dma_gen_start,
        .stop = dac_dma_gen_stop,
};

void
dac_dma_gen_init(dac_dma_gen_t *gen, dac_dma_generate_points_t generate_points) {
    gen_init(&gen->base_gen, &dac_dma_gen_impl);

    gen->generate_points = generate_points;
}
