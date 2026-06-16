#ifndef DAC_DMA_GEN_H
#define DAC_DMA_GEN_H

#include "driver/dac_continuous.h"
#include "esp_err.h"
#include "gen.h"
#include "util/ints.h"

#define DAC_DMA_GEN_BUFFER_SIZE 512

// this is a generic generator implementation that uses dac in dma continuous mode to generate a signal from the
// `dac_dma_gen_t.buffer`. users should set the `dac_dma_gen_t->generate_points` function thats going to fill the buffer
// with the desired data points.
//
// see the reference usage for the triangle signal generation

// should return bitmask of errors if the params are not good
typedef u32 (*dac_dma_generate_points_t)(u8 *buffer, u32 count, gen_params_t *params);

typedef struct dac_dma_gen {
    gen_t base_gen;

    dac_dma_generate_points_t generate_points;
    u8 buffer[DAC_DMA_GEN_BUFFER_SIZE];

    dac_continuous_handle_t dac_handle;
} dac_dma_gen_t;

void
dac_dma_gen_init(dac_dma_gen_t *gen, dac_dma_generate_points_t generate_points);

#endif
