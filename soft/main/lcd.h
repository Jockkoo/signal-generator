#ifndef LCD_H
#define LCD_H

#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <stdint.h>

#include "soc/gpio_num.h"

// Datasheet: https://mil.ufl.edu/4744/docs/LCD_Notes_4-bit.pdf

#define LCD_DATA_PIN_COUNT 4
#define LCD_COMMAND 0
#define LCD_DATA 1
#define ROW_OFFSET 0x40
#define LCD_QUEUE_SIZE 8

// All commands possible on an LCD display
#define LCD_CLEAR_CURSOR_HOME 0x01
#define LCD_CURSOR_HOME 0x02
#define LCD_CURSOR_DEC_SHIFT_OFF 0x04
#define LCD_CURSOR_DEC_SHIFT_ON 0x05
#define LCD_CURSOR_INC_SHIFT_OFF 0x06
#define LCD_CURSOR_INC_SHIFT_ON 0x07
#define LCD_DISPLAY_OFF_CURSOR_OFF_CURSOR_BLINK_OFF 0x08
#define LCD_DISPLAY_ON_CURSOR_OFF_CURSOR_BLINK_OFF 0x0C
#define LCD_DISPLAY_ON_CURSOR_ON_CURSOR_BLINK_OFF 0x0E
#define LCD_DISPLAY_ON_CURSOR_ON_CURSOR_BLINK_ON 0x0F
#define LCD_CURSOR_SHIFT_LEFT 0x10
#define LCD_CURSOR_SHIFT_RIGHT 0x14
#define LCD_DISPLAY_SHIFT_LEFT 0x18
#define LCD_DISPLAY_SHIFT_RIGHT 0x1C
#define LCD_CURSOR_TO_SECOND_ROW 0xC0

typedef struct lcd_conf {
    gpio_num_t enable;
    gpio_num_t reg_select;
    gpio_num_t data_pins[LCD_DATA_PIN_COUNT];
} lcd_config_t;

typedef struct {
    void (*lcd_fn)(uint8_t row, uint8_t col, const char *buffer);
    uint8_t row;
    uint8_t col;
    char buffer[17];
} lcd_row_data_t;

// Functions you can use wherever in code you want
esp_err_t
lcd_init(const lcd_config_t *lcd_config);
void
lcd_clear(void);
void
lcd_set_cursor(uint8_t row, uint8_t col);
void
lcd_printf(uint8_t row, uint8_t col, const char *fmt, ...);
void
lcd_ccprintf(const char *fmt, ...);
void
lcd_clear_row(uint8_t row);

// These functions initialize a thread and are to be used only if lcd_tinit()
// was called All functions are basically the same, prefixed with "t"
esp_err_t
lcd_tinit(const lcd_config_t *lcd_config, uint8_t core);
void
lcd_tclear(void);
void
lcd_tset_cursor(uint8_t row, uint8_t col);
void
lcd_tprintf(uint8_t row, uint8_t col, const char *fmt, ...);
void
lcd_tccprintf(const char *fmt, ...);
void
lcd_tclear_row(uint8_t row);

#endif
