#include "keypad.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "main.h"
#include "ui.h"
#include "util/ints.h"

// TODO: wrap this state into g struct, i am lazy to do it now
static const gpio_num_t rows[4] = {
        O_KEYPAD_ROW_0,
        O_KEYPAD_ROW_1,
        O_KEYPAD_ROW_2,
        O_KEYPAD_ROW_3,
};

static const gpio_num_t cols[4] = {
        I_KEYPAD_COL_0,
        I_KEYPAD_COL_1,
        I_KEYPAD_COL_2,
        I_KEYPAD_COL_3,
};

static volatile i32 current_pulse_row_idx = 0;

// export this keymap
const key_t keymap[4][4] = {
        {KEY_7, KEY_8, KEY_9, KEY_ENABLE},
        {KEY_4, KEY_5, KEY_6, KEY_BACKSLASH},
        {KEY_1, KEY_2, KEY_3, KEY_OK},
        {KEY_SIGN, KEY_0, KEY_DOT, KEY_CANCEL},
};

static void
task(void *data) {
    while(1) {
        // go through all the pins, pull them low for a brief second, and check for column pins.
        for(size_t i = 0; i < 4; i++) {
            current_pulse_row_idx = i;
            gpio_set_level(rows[i], 0);
            vTaskDelay(pdMS_TO_TICKS(5));

            for(size_t j = 0; j < 4; j++) {
                if(gpio_get_level(cols[j]) == 0) {
                    // this one was shortened
                    while(gpio_get_level(cols[j]) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }

                    ui_handle_key(keymap[i][j]);
                    break;
                }
            }

            gpio_set_level(rows[i], 1);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void
keypad_init(void) {
    // we set all the rows high initially and then periodicaly set the low and wait for interupts on the column pins
    for(size_t i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(gpio_reset_pin(rows[i]));
        ESP_ERROR_CHECK(gpio_set_direction(rows[i], GPIO_MODE_OUTPUT));
        ESP_ERROR_CHECK(gpio_set_level(rows[i], 1));
    }

    for(size_t i = 0; i < 4; i++) {
        ESP_ERROR_CHECK(gpio_reset_pin(cols[i]));
        ESP_ERROR_CHECK(gpio_set_direction(cols[i], GPIO_MODE_INPUT));
        ESP_ERROR_CHECK(gpio_set_pull_mode(cols[i], GPIO_PULLUP_ONLY));
    }

    assert(xTaskCreate(task, "keypad-and-ui", 8192, NULL, 3, NULL));
}
