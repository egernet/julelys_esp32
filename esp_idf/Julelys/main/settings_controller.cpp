#include "settings_controller.h"
#include "nvs_flash.h"

#define CONFIG_NAMESPACE "storage"
#define CONFIG_KEY "example_key"

SettingsController::SettingsController() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
}     

void SettingsController::saveConfig(int value) {
    nvs_handle_t nvs_handle;
    nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    nvs_set_i32(nvs_handle, CONFIG_KEY, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

int SettingsController::loadConfig() {
    nvs_handle_t nvs_handle;
    int32_t value = 0;
    nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &nvs_handle);
    nvs_get_i32(nvs_handle, CONFIG_KEY, &value);
    nvs_close(nvs_handle);
    return value;
}

void SettingsController::reset() {
    saveConfig(0);
}