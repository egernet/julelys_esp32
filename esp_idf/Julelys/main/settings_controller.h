#include <stdio.h>

#ifndef SETTINGS_CONTROLLER_H
#define SETTINGS_CONTROLLER_H

class SettingsController {
private:

public:
    SettingsController();

    void saveConfig(int value);
    int loadConfig();
    void reset();
};

#endif /* SETTINGS_CONTROLLER_H */