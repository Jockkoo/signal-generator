#include "main.h"

#include "ctl.h"
#include "freertos/idf_additions.h"
#include "keypad.h"
#include "lcd.h"
#include "soc/soc.h"
#include "ui.h"

static void
init_display(void) {
    lcd_config_t lcd_conf = {
            .enable = O_LCD_ENABLE,
            .reg_select = O_LCD_REG_SELECT,
            .data_pins =
                    {
                            O_LCD_DATA_PIN_0,
                            O_LCD_DATA_PIN_1,
                            O_LCD_DATA_PIN_2,
                            O_LCD_DATA_PIN_3,
                    },
    };

    ESP_ERROR_CHECK(lcd_tinit(&lcd_conf, PRO_CPU_NUM));

    vTaskDelay(pdMS_TO_TICKS(100));
    ui_render();
}

void
app_main(void) {
    printf("initilizing the generator...\n");

    printf("initilizing the controler...\n");
    ctl_init();

    printf("initilizing the ui...\n");
    ui_init();

    printf("initilizing the display...\n");
    init_display();

    printf("initilizing the keypad...\n");
    keypad_init();

    printf("initilizition successful!\n");

    // nothing, just wait
    while(1) {
        vTaskDelay(1000);
    }
}
