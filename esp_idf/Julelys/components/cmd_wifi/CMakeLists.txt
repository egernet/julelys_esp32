idf_component_register(SRCS "cmd_wifi.cpp" 
    INCLUDE_DIRS .
    REQUIRES console esp_wifi settings_controller)
