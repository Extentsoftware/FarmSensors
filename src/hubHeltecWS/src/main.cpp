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
char connStatus[64];
char topic[64];

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

void updateDisplay()
{
  char buf[32];
  Heltec.display->clear();
  sprintf( buf, "Incoming Pkts %d",packetcount);
  Heltec.display->drawString(0, 20, buf);
  sprintf( buf, "%s",connStatus);
  Heltec.display->drawString(0, 30, buf);
  Heltec.display->display();
}


void GetMyMacAddress()
{
  uint8_t array[6] { 0,0,0,0,0,0 };
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
}

void connectToWifi()
{
  sprintf(connStatus, "Connecting Wifi");
  updateDisplay();
  Serial.println("Connecting to Wi-Fi...");
  Serial.printf("SSID %s Pwd %s \n", WIFI_SSID, WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  sprintf(connStatus, "Connecting MQTT");
  updateDisplay();
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
//#if USING_CORE_ESP32_CORE_V200_PLUS
    case ARDUINO_EVENT_WIFI_READY:
      sprintf(connStatus, "Wifi Ready");
      updateDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_START:
      sprintf(connStatus, "Wifi Starting");
      updateDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      sprintf(connStatus, "Wifi Connected");
      updateDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      sprintf(connStatus, "Wifi %s", WiFi.localIP());
      updateDisplay();
      connectToMqtt();
      break;

    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      sprintf(connStatus, "WIFI_STA_LOST_IP");
      updateDisplay();
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      sprintf(connStatus, "WIFI_STA_DISCONNECTED");
      updateDisplay();

      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      //xTimerStart(wifiReconnectTimer, 0);
      break;

    default:
      sprintf(connStatus, "Wifi EVENT %d", event);
      updateDisplay();
      break;
  }
}

void onMqttConnect(bool sessionPresent)
{
  sprintf(connStatus, "Connected to MQTT");
  updateDisplay();
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;

  sprintf(connStatus, "Disconnected from MQTT");
  updateDisplay();

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
  else
  {
    xTimerStart(wifiReconnectTimer, 2000);
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

bool sendMQTTBinary(unsigned char *report, int packetSize)
{
  if (mqttClient.connected())
  {
      if (!mqttClient.publish(topic,0,false, (char *)report, packetSize))
      {
        sprintf(connStatus, "MQTT Fail");
      } 
      else
      {
        sprintf(connStatus,"MQTT Sent %d", packetSize);
      }
      return true;
  }
  else
    sprintf(connStatus,"MQTT Not connected");
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
  packetcount++;

  unsigned char buffer[ 64 ];
  memset(buffer,0,sizeof(buffer));

  Wire1.readBytes(buffer, howMany);
  for(int i=0; i<howMany; i++)
    Serial.printf("%02X ", buffer[i]);
  Serial.printf( "\n");

  sendMQTTBinary(buffer, howMany);
  updateDisplay();
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
  sprintf(topic, "bongo/%s/sensor", macStr);

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
