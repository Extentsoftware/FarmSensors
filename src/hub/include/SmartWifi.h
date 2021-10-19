#ifndef _ARDUINO_SMARTWIFI_H
#define _ARDUINO_SMARTWIFI_H

#include <Arduino.h>
#include "FS.h"
#include "esp_system.h"
#include <esp_wifi.h>
#include <string.h>
#include <WiFi.h>
#include <Preferences.h>  // WiFi storage

class SmartWifi {

public:

    SmartWifi();
    void loop();
    int getWifiStatus();
    enum Status {
        INIT,
        IN_SMART_CONFIG,
        CONNECTING,
        CONNECTED,
    };
    enum Status getStatus();

private: 

    bool isSmartConfigComplete();
    bool isSmartConfigRequired();
    bool isValidConfig();
    void saveConfig();
    void initSmartConfig();
    void connect();
    
    String PrefSSID, PrefPassword;  // used by preferences storage
    int LastWFstatus = 0;
    bool doneSmartConfig = false;
    Preferences preferences;  // declare class object
    Status status = Status::INIT;

};

#endif