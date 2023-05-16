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

#define RF_FREQUENCY        868E6           // Hz
#define RF_BAND             125E3         
#define TX_OUTPUT_POWER     20              // dBm
#define SPREAD              12   
#define CODERATE            5
#define SYNCWORD            0 

#define LORA_PREAMBLE_LENGTH                        8               // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0               // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  1
#define LORA_FIX_LENGTH_PAYLOAD_OFF                 0
#define LORA_IQ_INVERSION_ON                        false
#define LORA_CRC                                    true
#define LORA_FREQ_HOP                               false
#define LORA_HOP_PERIOD                             0
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here
#define SYNCWORD                                    0

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
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;

    default:
      break;
  }
}

void onMqttConnect(bool sessionPresent)
{
  Serial.println("Connected to MQTT broker: ");
  char topic[32];
  sprintf(topic, "bongo/%s", macStr);

  uint16_t packetIdSub = mqttClient.subscribe(topic, 2);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;

  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
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

void GetMyMacAddress()
{
  uint8_t array[6] { 0,0,0,0,0,0 };
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
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
  return false;
}

void onReceive(int packetSize) 
{  
  Serial.printf("lora received %d bytes\n",packetSize);
  float snr = LoRa.packetSnr();
  float rssi = LoRa.packetRssi();
  long pfe = 0; //LoRa.packetFrequencyError();
  CayenneLPP lpp(packetSize + 20);
  Serial.printf("Got %d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 

  LoRa.readBytes(lpp._buffer, packetSize);

  // add signal strength etc
  lpp._cursor = packetSize;
  lpp.addGenericSensor(CH_SNR, snr);
  lpp.addGenericSensor(CH_RSSI, rssi);
  lpp.addGenericSensor(CH_PFE, (float)pfe);  

  sendMQTTBinary(lpp._buffer, lpp._cursor, "sensor");

  Serial.printf("lora receive complete\n");
}

void startLoRa() {
  Serial.printf("\nStarting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%ld txpower:%d\n"
  , (unsigned long)RF_FREQUENCY, LORA_CRC, CODERATE, SPREAD, (long)RF_BAND, TX_OUTPUT_POWER);

  LoRa.setPreambleLength(LORA_PREAMBLE_LENGTH);
  LoRa.setSpreadingFactor(SPREAD);
  LoRa.setCodingRate4(CODERATE);

  if (LORA_CRC)
      LoRa.enableCrc();
    else 
      LoRa.disableCrc();

  LoRa.setSyncWord(SYNCWORD);
  LoRa.setSignalBandwidth((long)RF_BAND);
 
  LoRa.onReceive(onReceive);
  LoRa.receive();
}

void loop() {
  // int packetSize = LoRa.parsePacket();
  // if (packetSize>0)
  //   onReceive(packetSize);
}

void setup()
{
  Serial.begin(115200);

  while (!Serial && millis() < 5000);

  Heltec.begin(
      true /*DisplayEnable Enable*/, 
      true /*Heltec.Heltec.Heltec.LoRa Disable*/, 
      true /*Serial Enable*/, 
      true /*PABOOST Enable*/, 
      (unsigned long)RF_FREQUENCY /*long BAND*/);
 
  startLoRa();

  // Heltec.display->init();
//  Heltec.display->flipScreenVertically();  
  // Heltec.display->setFont(ArialMT_Plain_10);
  // Heltec.display->clear();
  
  // Heltec.display->drawString(0, 0, "VESTRONG");
  // Heltec.display->drawString(0, 10, "WIFI Listen");
  // Heltec.display->drawString(0, 20, "LORA Listen");
  // Heltec.display->display();

#ifdef WIFI
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  connectToWifi();
#endif  
  Serial.printf("End of setup\n");
}
