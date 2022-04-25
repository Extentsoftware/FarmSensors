#ifndef __MQTT_WIFI__
#define __MQTT_WIFI__

#define SerialMon Serial

#include <PubSubClient.h>
#include <SoftwareSerial.h>
#include <WifiClient.h>

class MqttWifiClient 
{
  public:
    MqttWifiClient();
    void init(const char *broker, const char *macStr, std::function<void(char*, byte*, unsigned int)> callback, std::function<void()> displayUpdate);
    bool sendMQTTBinary(uint8_t *report, int packetSize);
    void doSetupWifiMQTT();
    void mqttWiFiConnect();
    bool isWifiConnected();

  private:
    std::function<void(char*, byte*, unsigned int)> _callback;
    std::function<void()> _displayUpdate;
    bool _wifiConnected=false;
    String _broker;
    String _address;
    String _wifiStatus;
    const char *_macStr;
    WiFiClient _wifiClient; 
    bool _lastMqttConnected=false;
    PubSubClient _mqttWifi;
};

#endif