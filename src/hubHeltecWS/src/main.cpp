#define APP_VERSION 1

#include "heltec.h"

#include <vesoil.h>
#include <Wire.h>  
#include "main.h"
#include <CayenneLPP.h>
#include "base64.hpp"
#include <WiFi.h>

static const char * TAG = "Hub1.1";

char macStr[18];                      // my mac address
bool gprsConnected=false;
int packetcount=0;

const int connectionTimeout = 15 * 60000; //time in ms to trigger the watchdog

#define BUFFER_SIZE                                 30 // Define the payload size here
#define SYNCWORD                                    0
#define CHANNEL                                     5

#define DEFAULT_BROKER            "bongomqtt.uksouth.cloudapp.azure.com"

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

#define MQTT_HOST         DEFAULT_BROKER
#define MQTT_PORT         1883
#define WIFI_SSID         "Poulton"
#define WIFI_PASSWORD     "12345678"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void GetMyMacAddress()
{
  uint8_t array[6] { 0,0,0,0,0,0 };
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  Serial.printf("SSID %s Pwd %s \n", WIFI_SSID, WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
//#if USING_CORE_ESP32_CORE_V200_PLUS

    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("WiFi ready");
      break;

    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi STA starting");
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      Serial.println("WiFi STA connected");
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      Serial.println("WiFi lost IP");
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      //Serial.println("WiFi lost connection");
      //xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      //xTimerStart(wifiReconnectTimer, 0);
      break;

    default:
      break;
  }
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT broker: ");
  //char topic[32];
  //sprintf(topic, "bongo/%s", macStr);
  //uint16_t packetIdSub = mqttClient.subscribe(topic, 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;

  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
  else
  {
    xTimerStart(wifiReconnectTimer, 0);
  }
}

void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(const uint16_t& packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total)
{
  (void) payload;

  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(const uint16_t& packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

bool sendMQTTBinary(unsigned char *report, int packetSize, const char *subtopic)
{
  Serial.printf("sendMQTTBinary::entered\n");
  char topic[32];
  sprintf(topic, "bongo/%s/%s", macStr,subtopic);

  if (mqttClient.connected())
  {
      if (!mqttClient.publish(topic,0,false, (char *)report, packetSize))
      {
        Serial.printf("MQTT GPRS Fail\n");
      } 
      else
      {
        Serial.printf("MQTT Sent GPRS %d bytes to %s\n", packetSize, topic);
      }
      return true;
  }
  else
    Serial.printf("MQTT GPRS Not connected!\n");
  return false;
}


void loop() {
  static long i=0;
  i++;
  if ( (i % 300000) == 0)
  {
    Serial.printf(" %d ",Wire1.available());
  }  
}

void receiveEvent(int howMany)
{
  static int pkts =0;
  pkts++;
  char buf[32];
  sprintf( buf, "I2C %d Pkt %d",howMany, pkts);
  Heltec.display->clear();
  Heltec.display->drawString(0, 20, buf);
  Heltec.display->display();

  unsigned char buffer[ 64 ];
  memset(buffer,0,sizeof(buffer));

  Wire1.readBytes(buffer, howMany);
  for(int i=0; i<howMany; i++)
    Serial.printf("%02X ", buffer[i]);
  Serial.printf( "\n");

  sendMQTTBinary(buffer, howMany, "sensor");
}

void setup()
{
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  Serial.printf("Begin i2c\n");
  pinMode(1, INPUT);
  pinMode(2, INPUT);
  for (;;)
  {
    if (Wire1.begin(CHANNEL, 1 ,2 ,0 ))
    {
      Serial.printf("I2C started slave %d clock %d\n",CHANNEL, Wire1.getClock());
      break;
    }
    Serial.printf("I2C failed to start\n");
    delay(1000);
  }
  Wire1.onReceive(receiveEvent); 


  Heltec.begin(
      true /*DisplayEnable Enable*/, 
      false /*Heltec.Heltec.Heltec.LoRa Disable*/, 
      true /*Serial Enable*/, 
      false /*PABOOST Enable*/, 
      0 );
 
  GetMyMacAddress();

  Heltec.display->init();
  Heltec.display->flipScreenVertically();  
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setBrightness(5);
  Heltec.display->clear();
  
  Heltec.display->drawString(0, 0, "Ready");
  Heltec.display->display();


  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  //mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();


  Serial.printf("End of setup\n");
}
