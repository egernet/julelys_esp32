#include <stdio.h>

#ifndef SETTINGS_CONTROLLER_H
#define SETTINGS_CONTROLLER_H

class SettingsController {
private:
    void saveCharConfig(char *key, char *value);
    void saveIntConfig(char *key, int value);
    char* loadCharConfig(char *key);
    int loadIntConfig(char *key);
    int deleteKeyValue(const char *key);
public:
    SettingsController();

    void resetAll();

    char* getWIFISSID();
    char* getWIFIPassword();

    void setWIFISSID(char* ssid);
    void setWIFIPassword(char* password);
};

#endif /* SETTINGS_CONTROLLER_H */