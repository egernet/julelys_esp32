#include "settings_controller.h"
#include "nvs_flash.h"
#include "esp_log.h"

#define CONFIG_NAMESPACE "storage"
#define CONFIG_WIFI_SSID_KEY "wifi_ssid_key"
#define CONFIG_WIFI_PASS_KEY "wifi_pass_key"

static const char *TAG = "Jylelys.Settings";

SettingsController::SettingsController() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
} 

int SettingsController::deleteKeyValue(const char *key) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_erase_key(nvs_handle, key);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    nvs_close(nvs_handle);

    return ESP_OK;
}

void SettingsController::saveCharConfig(char *key, char *value) {
    nvs_handle_t nvs_handle;
    nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    nvs_set_str(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

char* SettingsController::loadCharConfig(char *key) {
    nvs_handle_t nvs_handle;
    char *value;
    nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &nvs_handle);

    size_t required_size;
    esp_err_t err = nvs_get_str(nvs_handle, key, NULL, &required_size);
    if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
        return NULL;
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
        return NULL;
    } else {
        value = (char*)malloc(required_size);
        err = nvs_get_str(nvs_handle, key, value, &required_size);
        if (err != ESP_OK) {
            return NULL;
        }
    }

    nvs_close(nvs_handle);
    return value;
}

void SettingsController::saveIntConfig(char *key, int value) {
    nvs_handle_t nvs_handle;
    nvs_open(CONFIG_NAMESPACE, NVS_READWRITE, &nvs_handle);
    nvs_set_i32(nvs_handle, key, value);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

int SettingsController::loadIntConfig(char *key) {
    nvs_handle_t nvs_handle;
    int32_t value = 0;
    nvs_open(CONFIG_NAMESPACE, NVS_READONLY, &nvs_handle);
    nvs_get_i32(nvs_handle, key, &value);
    nvs_close(nvs_handle);
    return value;
}

void SettingsController::resetAll() {
    deleteKeyValue(CONFIG_WIFI_SSID_KEY);
    deleteKeyValue(CONFIG_WIFI_PASS_KEY);

     ESP_LOGW(TAG, "All data is delete!");
}

char* SettingsController::getWIFISSID() {
    return loadCharConfig(CONFIG_WIFI_SSID_KEY);
}

char* SettingsController::getWIFIPassword() {
    return loadCharConfig(CONFIG_WIFI_PASS_KEY);
}

void SettingsController::setWIFISSID(char* ssid) {
    saveCharConfig(CONFIG_WIFI_SSID_KEY, ssid);
}
    
void SettingsController::setWIFIPassword(char* password) { 
    saveCharConfig(CONFIG_WIFI_PASS_KEY, password);
}