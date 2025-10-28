#define APP_VERSION 1
// Add this to board manager URLs https://resource.heltec.cn/download/package_heltec_esp32_index.json
// And install:Heltec ESP32 Series Dev-boards
// Then select the board, and it works.
// https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html#via-arduino-board-manager

#include <vesoil.h>

#include <Wire.h>  
#include "main.h"
#include <CayenneLPP.h>
#include "base64.hpp"
#include <WiFi.h>
#include <MQTT.h>
#include "SSD1306Wire.h"

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

#define DEFAULT_BROKER  "iot.vestrong.eu"

// extern "C"
// {
// #include "freertos/FreeRTOS.h"
// #include "freertos/timers.h"
// }
// #include <AsyncMqttClient.h>

#define MQTT_HOST         DEFAULT_BROKER
#define MQTT_PORT         1883
#define WIFI_SSID         "poulton" 
// ENV_WIFI_SSID
#define WIFI_PASSWORD     ENV_WIFI_PASSWORD

WiFiClient net;
MQTTClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

#ifdef HASDISPLAY
SSD1306Wire display(0x3c, SDA_OLED, SCL_OLED);

void updateDisplay()
{
  char buf[32];
  display.clear();
  sprintf( buf, "Incoming Pkts %d",packetcount);
  display.drawString(0, 20, buf);
  sprintf( buf, "%s",connStatus);
  display.drawString(0, 30, buf);
  display.display();
}
#else
void updateDisplay()
{
  Serial.println(connStatus);
}
#endif


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
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  mqttClient.begin(MQTT_HOST, MQTT_PORT, net);
}

void connectToMqtt()
{
    if (!mqttClient.connected()) {
    
      mqttClient.connect("hub");
    }
  //sprintf(connStatus, "Connecting MQTT");
  //updateDisplay();
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

bool sendMQTTBinary(unsigned char *report, int packetSize)
{
  if (!mqttClient.connected())
  {
    mqttClient.connect("hub");
  }

  if (mqttClient.connected())
  {
      if (!mqttClient.publish(topic, (char *)report,packetSize, false, 0 ))
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
  {
    sprintf(connStatus,"MQTT Not connected");    
  }
  return false;
}


void loop() {
  static long i=0;
  i++;
  if ( (i % 300000) == 0)
  {    
    sprintf(connStatus, " 1W %d WIFI %d MQTT %d",Wire1.available(), WiFi.status(),mqttClient.connected());
    updateDisplay();

    if (!mqttClient.connected())
    {
      mqttClient.connect("hub");
    }
  }  

  mqttClient.loop();
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

void wireBegin()
{
  for (;;)
  {
    if (Wire1.begin(CHANNEL, 1 ,2 ,0 ))
    {
      Serial.printf("I2C started slave %d clock %d\n",CHANNEL, Wire1.getClock());
      Wire1.onReceive(receiveEvent); 
      break;
    }
    Serial.printf("I2C failed to start\n");
    delay(1000);
  }
}

void setup()
{
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  Serial.printf("Begin i2c\n");
  pinMode(1, INPUT);
  pinMode(2, INPUT);

  #ifdef HASDISPLAY
  display.init();
  display.flipscreenvertically();  
  display.setfont(arialmt_plain_10);
  display.setbrightness(5);
  display.clear();
  display.drawstring(0, 0, "ready");
  display.display();
  #endif

  wireBegin();

  GetMyMacAddress();
  sprintf(topic, "bongo/%s/sensor", macStr);



  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  connectToWifi();

  Serial.printf("End of setup\n");
}
