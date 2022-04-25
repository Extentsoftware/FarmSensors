#include "mqtt_wifi.h"

#define SerialMon Serial

MqttWifiClient::MqttWifiClient() 
  : _wifiClient()
  , _mqttWifi(_wifiClient)
{
}

void MqttWifiClient::init(const char *broker, const char *macStr, std::function<void(char*, byte*, unsigned int)> callback, std::function<void()> displayUpdate)
{
  _broker = broker;
  _macStr = macStr;
  _callback = callback;
  _displayUpdate = displayUpdate;
}

bool MqttWifiClient::sendMQTTBinary(uint8_t *report, int packetSize)
{
  char topic[32];
  sprintf(topic, "bongo/%s/sensor", _macStr);

  if (_mqttWifi.connected())
  {
      if (!_mqttWifi.publish(topic, report, packetSize))
      {
        Serial.printf("MQTT Wifi Fail\n");
      } 
      else
      {
        Serial.printf("MQTT Sent Wifi %d bytes to %s\n", packetSize, topic);
      }
      return true;
  }
  return false;
}

void MqttWifiClient::doSetupWifiMQTT()
{
  _mqttWifi.setCallback(_callback);
  _mqttWifi.setServer(_broker.c_str(),1883);
};

void MqttWifiClient::mqttWiFiConnect()
{
  char topic[32];

  boolean mqttConnected = _mqttWifi.connect(_macStr);
  if (mqttConnected)
  {
    Serial.printf("MQTT Connected over WiFi\n");
    sprintf(topic, "bongo/%s/hub", _macStr);
    _mqttWifi.subscribe(topic);  
    _mqttWifi.publish(topic, "{\"state\":\"connected\"}", true);
  }
}  

bool MqttWifiClient::isWifiConnected() 
{  
  _mqttWifi.loop();

  bool mqttConnected = _mqttWifi.connected();
  if (_lastMqttConnected != mqttConnected)
  {
      _wifiStatus = mqttConnected ? "MQTT Connected" : "MQTT Disconnected";
      _displayUpdate();
  }

  if (!mqttConnected)
  {
      mqttWiFiConnect();
  }

  _lastMqttConnected = mqttConnected;
  return _mqttWifi.connected();
}
