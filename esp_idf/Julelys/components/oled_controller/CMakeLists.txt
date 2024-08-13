idf_component_register(
    SRCS "oled_controller.cpp"
    INCLUDE_DIRS .
    REQUIRES lvgl esp_lcd_sh1107 esp_lvgl_port driver)