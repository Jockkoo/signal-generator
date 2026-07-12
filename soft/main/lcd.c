#include "lcd.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_err.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include "util/ints.h"

static struct lcd {
    gpio_num_t enable;
    gpio_num_t reg_select;
    gpio_num_t data_pins[LCD_DATA_PIN_COUNT];
} lcd;

static struct {
    TaskHandle_t lcd_task_handle;
    QueueHandle_t lcd_queue;
} g;

static void
lcd_pulse(void) {
    gpio_set_level(lcd.enable, 1);
    ets_delay_us(5);  // minimum is 500ns
    gpio_set_level(lcd.enable, 0);
    ets_delay_us(50);
}

// function that sends a nibble (4 bits) to the lcd
// since we are using 4 wire mode we need this function
static void
lcd_send_nibble(uint8_t nibble) {
    for(uint8_t i = 0; i < LCD_DATA_PIN_COUNT; i++) {
        gpio_set_level(lcd.data_pins[i], (nibble >> i) & 0x01);
    }
    lcd_pulse();
}

static void
lcd_send_byte(uint8_t byte, uint8_t cmd_type) {
    gpio_set_level(lcd.reg_select, cmd_type);
    ets_delay_us(5);
    // first we send higher bits
    lcd_send_nibble(byte >> 4);
    ets_delay_us(5);
    // then we send lower bits
    lcd_send_nibble(byte & 0x0F);
    // if the command is to go home then we delay more, since it takes the most
    // time
    ets_delay_us(100);
}

esp_err_t
lcd_init(const lcd_config_t *lcd_config) {
    lcd.enable = lcd_config->enable;
    lcd.reg_select = lcd_config->reg_select;

    uint64_t pin_mask = 1ULL << lcd.enable | 1ULL << lcd.reg_select;

    for(uint8_t i = 0; i < LCD_DATA_PIN_COUNT; i++) {
        lcd.data_pins[i] = lcd_config->data_pins[i];
        pin_mask |= 1ULL << lcd_config->data_pins[i];
    }
    // assigning all necessary pins to output

    gpio_config_t pin_config = {
            .pin_bit_mask = pin_mask,
            .mode = GPIO_MODE_OUTPUT,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
    };
    esp_err_t err = gpio_config(&pin_config);
    if(err != ESP_OK) {
        goto err_gpio;
    }

    // We should play around with these to minimize the delays

    vTaskDelay(pdMS_TO_TICKS(30));
    lcd_send_nibble(0x03);
    vTaskDelay(pdMS_TO_TICKS(10));  // wait for LCD to power up
    lcd_send_nibble(0x03);
    ets_delay_us(150);
    lcd_send_nibble(0x03);
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd_send_nibble(0x02);

    ets_delay_us(50);
    lcd_send_byte(0x28, LCD_COMMAND);  // Display Function 2 rows for 4-bit
    // will need to change this for 4 line display
    ets_delay_us(50);
    lcd_send_byte(LCD_DISPLAY_ON_CURSOR_ON_CURSOR_BLINK_OFF, LCD_COMMAND);  // Display on, Cursor and Cursor Blink on
    ets_delay_us(50);
    lcd_send_byte(LCD_CURSOR_INC_SHIFT_OFF, LCD_COMMAND);  // Cursor Increment, Shift off
    ets_delay_us(50);
    lcd_send_byte(LCD_CLEAR_CURSOR_HOME, LCD_COMMAND);  // Clear Display, Cursor to Home
    ets_delay_us(3000);  // Must set FreeRtos tick rate to 1000!

    return ESP_OK;

err_gpio:
    return err;
}

void
lcd_clear(void) {
    lcd_send_byte(LCD_CLEAR_CURSOR_HOME, LCD_COMMAND);
    ets_delay_us(3000);
}

void
lcd_set_cursor(uint8_t row, uint8_t col) {
    u8 row_offsets[] = {0x00, 0x40, 0x10, 0x50};
    lcd_send_byte(0x80 | (col + row_offsets[row]), LCD_COMMAND);
}

void
lcd_printf(uint8_t row, uint8_t col, const char *fmt, ...) {
    char buffer[17];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    lcd_set_cursor(row, col);

    for(uint8_t i = 0; buffer[i] != '\0'; i++) {
        lcd_send_byte(buffer[i], LCD_DATA);
    }
}

void
lcd_clear_row(uint8_t row) {
    lcd_printf(row, 0, "                ");
}
// same function but prints from Current Cursor position
void
lcd_ccprintf(const char *fmt, ...) {
    char buffer[17];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    for(uint8_t i = 0; buffer[i] != '\0'; i++) {
        lcd_send_byte(buffer[i], LCD_DATA);
    }
}

// Below are functions which relly on a task that must be created with
// lcd_tinit();

static void
lcd_task(void *arg) {
    lcd_row_data_t row_data;
    while(1) {
        if(xQueueReceive(g.lcd_queue, &row_data, portMAX_DELAY)) {
            row_data.lcd_fn(row_data.row, row_data.col, row_data.buffer);
        }
    }
}

esp_err_t
lcd_tinit(const lcd_config_t *lcd_config, uint8_t core) {
    esp_err_t err = lcd_init(lcd_config);
    if(err != ESP_OK)
        return err;

    g.lcd_queue = xQueueCreate(LCD_QUEUE_SIZE, sizeof(lcd_row_data_t));
    if(g.lcd_queue == NULL)
        goto err_queue;

    if(xTaskCreatePinnedToCore(lcd_task, "LCD task", 2048, NULL, 5, &g.lcd_task_handle, core) != pdPASS)
        goto err_task;

    return ESP_OK;

err_task:
    vTaskDelete(g.lcd_task_handle);
    return ESP_FAIL;
err_queue:
    vQueueDelete(g.lcd_queue);
    return ESP_ERR_NO_MEM;
}

// Clear
static void
_lcd_clear(uint8_t row, uint8_t col, const char *buffer) {
    lcd_clear();
}

void
lcd_tclear(void) {
    lcd_row_data_t row_data = {
            .lcd_fn = _lcd_clear,
    };
    if(xQueueSend(g.lcd_queue, &row_data, 0) != pdTRUE) {
        printf("LCD queue is full! Message dropped!\n");
    };
}

// Set cursor
static void
_lcd_set_cursor(uint8_t row, uint8_t col, const char *buffer) {
    lcd_set_cursor(row, col);
}

void
lcd_tset_cursor(uint8_t row, uint8_t col) {
    lcd_row_data_t row_data = {
            .lcd_fn = _lcd_set_cursor,
            .row = row,
            .col = col,
    };
    if(xQueueSend(g.lcd_queue, &row_data, 0) != pdTRUE) {
        printf("LCD queue is full! Message dropped!\n");
    };
}

// Print at a specified place
static void
_lcd_printf(uint8_t row, uint8_t col, const char *buffer) {
    lcd_printf(row, col, "%s", buffer);
}

void
lcd_tprintf(uint8_t row, uint8_t col, const char *fmt, ...) {
    lcd_row_data_t row_data = {
            .lcd_fn = _lcd_printf,
            .row = row,
            .col = col,
    };
    va_list args;
    va_start(args, fmt);
    vsnprintf(row_data.buffer, sizeof(row_data.buffer), fmt, args);
    va_end(args);
    if(xQueueSend(g.lcd_queue, &row_data, 0) != pdTRUE) {
        printf("LCD queue is full! Message dropped!\n");
    };
}

// Print from current cursor position
static void
_lcd_ccprintf(uint8_t row, uint8_t col, const char *buffer) {
    lcd_ccprintf("%s", buffer);
}

void
lcd_tccprintf(const char *fmt, ...) {
    lcd_row_data_t row_data = {
            .lcd_fn = _lcd_ccprintf,
    };
    va_list args;
    va_start(args, fmt);
    vsnprintf(row_data.buffer, sizeof(row_data.buffer), fmt, args);
    va_end(args);
    if(xQueueSend(g.lcd_queue, &row_data, 0) != pdTRUE) {
        printf("LCD queue is full! Message dropped!\n");
    };
}

// Clear Row
static void
_lcd_clear_row(uint8_t row, uint8_t col, const char *buffer) {
    lcd_clear_row(row);
}

void
lcd_tclear_row(uint8_t row) {
    lcd_row_data_t row_data = {
            .lcd_fn = _lcd_clear_row,
            .row = row,
    };
    if(xQueueSend(g.lcd_queue, &row_data, 0) != pdTRUE) {
        printf("LCD queue is full! Message dropped!\n");
    };
}
