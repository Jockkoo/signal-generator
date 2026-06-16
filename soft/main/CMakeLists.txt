set(SRCS
    main.c
    isr_mgr.c
    lcd.c
    keypad.c
    ctl.c
    ui.c
    generators/gen.c
    generators/sine_gen.c
    generators/rect_gen.c
    generators/dac_dma_gen.c
    generators/triangle_gen.c
)

idf_component_register(SRCS ${SRCS}
                       INCLUDE_DIRS ".")
