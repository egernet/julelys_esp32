#include <stdio.h>

#ifndef OLED_CONTROLLER_H
#define OLED_CONTROLLER_H

class OLEDController {
private:
    const char* ip_address;
    const char* host_ip_address;
public:
    OLEDController();
    
    void clean();

    void update();   

    void setLocalIPAddress(const char* ip_address);
    void setHostIPAddress(const char* ip_address);
};

#endif /* OLED_CONTROLLER_H */