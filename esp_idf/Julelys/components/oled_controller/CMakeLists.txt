idf_component_register(
    SRCS "lv_font_montserrat_12.c" "lv_font_montserrat_10.c" "oled_controller.cpp"
    INCLUDE_DIRS .
    REQUIRES lvgl esp_lcd_sh1107 esp_lvgl_port driver)