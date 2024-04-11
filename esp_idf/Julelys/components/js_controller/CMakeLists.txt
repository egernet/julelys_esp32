idf_component_register(
    SRCS "elk.c" "js_controller.cpp"
    INCLUDE_DIRS .
    REQUIRES led_controller esp_timer)