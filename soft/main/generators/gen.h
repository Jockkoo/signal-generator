#ifndef GEN_H
#define GEN_H

#include "esp_err.h"
#include "util/ints.h"

// this file containes a generic genartor interface. anything implementing this interface is considered a generator. see
// `dac_dma_gen.h`, `triangle_gen.h` `rect_gen.h` and `sine_gen.h` and appropriate sources for references

typedef struct gen_params {
    i32 freq;
    float offset;
    float symmetry;
} gen_params_t;

struct gen;

typedef enum gen_type {
    GEN_TYPE_SINE,
    GEN_TYPE_RECT,
    GEN_TYPE_TRI
} gen_type_t;

typedef enum gen_error {
    GEN_ERROR_NONE = 0,
    GEN_ERROR_FREQ = (1 << 0),
    GEN_ERROR_OFFSET = (1 << 1),
    GEN_ERROR_SYMMETRY = (1 << 2),
    GEN_ERROR_UNKNOWN = (1 << 3),
} gen_error_t;

// generator interface, i.e. a struct of function pointers. we might want to add hot reloading in the future, that is,
// the abillity to change the parametars on the fly, which should also go here.
typedef struct gen_interface {
    u32 (*start)(struct gen *gen, gen_params_t *params);  // should return a bitmask of all the invalid params
    esp_err_t (*stop)(struct gen *gen);
} gen_interface_t;

typedef struct gen {
    const gen_interface_t *impl;
} gen_t;

void
gen_init(gen_t *gen, const gen_interface_t *impl);

u32
gen_start(gen_t *gen, gen_params_t *params);

esp_err_t
gen_stop(gen_t *gen);

#endif
