#include "SmartWifi.h"

SmartWifi::SmartWifi()
{
    WiFi.mode(WIFI_AP_STA);  // Init WiFi, start SmartConfig
}

void SmartWifi::loop() {

    switch(status)
    {
    case Status::INIT:
        // Attempt connection if config is available
        if (isSmartConfigRequired())
        {
            // enter smart config
            initSmartConfig();
        }
        else
        {
            connect();
        }
        break;
    case Status::IN_SMART_CONFIG:
        if (isSmartConfigComplete())
        {
            connect();
        }
        break;
    case Status::CONNECTING:
        if (WiFi.status() == WL_CONNECTED)
        {
            status = Status::CONNECTED;
        }
        break;
    case Status::CONNECTED:
        if (getWifiStatus() != WL_CONNECTED)
        {
            // we became disconnected
            status = Status::INIT;
        }

        // got connected, do we need to save the creds?
        if (doneSmartConfig)
        {
            // save the creds & reboot.
            saveConfig();
            delay(3000);
            ESP.restart();  // reboot with wifi configured            
        }
        break;
    }   
}

void SmartWifi::connect()
{
    // attempt to connect
    WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());
    status = Status::CONNECTING;
    Serial.printf("Connecting to %s\n", PrefSSID.c_str());   
}

bool SmartWifi::isSmartConfigRequired()
{
    // Open Preferences with "wifi" namespace. Namespace is limited to 15 chars
    preferences.begin("wifi", false);
    PrefSSID = preferences.getString("ssid", "none");  // NVS key ssid
    PrefPassword = preferences.getString("password", "none");  // NVS key password
    preferences.end();

    // wifi_config_t conf;
    // esp_wifi_get_config(WIFI_IF_STA, &conf);
    // String NVssid = String(reinterpret_cast<const char*>(conf.sta.ssid));
    // String NVpass = String(reinterpret_cast<const char*>(conf.sta.password));

    // Serial.printf("NV config ssid=%s pwd=%s\n", NVssid.c_str(), NVpass.c_str());
    Serial.printf("PF config ssid=%s pwd=%s\n", PrefSSID.c_str(), PrefPassword.c_str());

    //bool valid = (PrefSSID != "none" && (NVssid.equals(PrefSSID) && NVpass.equals(PrefPassword)));
    bool valid = (PrefSSID != "none");
    Serial.printf("SmartConfig is %srequired\n", valid?"not ":"");
    return !valid;
}

void SmartWifi::saveConfig()
{
    // wifi_config_t conf;
    // esp_wifi_get_config(WIFI_IF_STA, &conf);
    // String NVssid = String(reinterpret_cast<const char*>(conf.sta.ssid));
    // String NVpass = String(reinterpret_cast<const char*>(conf.sta.password));

    // Serial.printf("saving config ssid=%s pwd=%s\n", NVssid.c_str(), NVpass.c_str());

    Serial.printf("saving config ssid=%s pwd=%s\n", PrefSSID.c_str(), PrefPassword.c_str());
    preferences.begin("wifi", false);
    preferences.putString("ssid", PrefSSID);
    preferences.putString("password", PrefPassword);
    preferences.end();
}

void SmartWifi::initSmartConfig() {
    Serial.printf("Entering SmartConfig\n");
    WiFi.beginSmartConfig();
    doneSmartConfig = true;
    status= Status::IN_SMART_CONFIG;
}

bool SmartWifi::isSmartConfigComplete() {
    bool complete = WiFi.smartConfigDone();
    if (complete)
    {
        Serial.printf("SmartConfig complete\n");
        PrefSSID = WiFi.SSID();
        PrefPassword = WiFi.psk();
    }
    return complete;
}

int SmartWifi::getWifiStatus() {
  int WiFiStatus = WiFi.status();
  if (LastWFstatus != WiFiStatus)
  {
    switch (WiFiStatus) {
        case WL_IDLE_STATUS:  // WL_IDLE_STATUS     = 0,
        Serial.printf("WiFi IDLE \n");
        break;
        case WL_NO_SSID_AVAIL:  // WL_NO_SSID_AVAIL   = 1,
        Serial.printf("NO SSID AVAIL \n");
        break;
        case WL_SCAN_COMPLETED:  // WL_SCAN_COMPLETED  = 2,
        Serial.printf("WiFi SCAN_COMPLETED \n");
        break;
        case WL_CONNECTED:  // WL_CONNECTED       = 3,
        Serial.printf("WiFi CONNECTED \n");
        break;
        case WL_CONNECT_FAILED:  // WL_CONNECT_FAILED  = 4,
        Serial.printf("WiFi WL_CONNECT FAILED\n");
        break;
        case WL_CONNECTION_LOST:  // WL_CONNECTION_LOST = 5,
        Serial.printf("WiFi CONNECTION LOST\n");
        WiFi.persistent(false);  // don't write FLASH
        break;
        case WL_DISCONNECTED:  // WL_DISCONNECTED    = 6
        Serial.printf("WiFi DISCONNECTED\n");
        WiFi.persistent(false);  // don't write FLASH when reconnecting
        break;
    }
  }
  LastWFstatus = WiFiStatus;
  return WiFiStatus;
}

