// https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G
#define APP_VERSION 1

#include <vesoil.h>
#include <SPI.h>
#include <Wire.h>  
#include "mqtt_gsm.h"
#include "main.h"
#include <wd.h>
#include "bt.h"
#include "BleClient.h"
#include <Preferences.h>

static const char * TAG = "Hub";

BleClient bt;
MqttGsmClient *mqttGPRS;
MqttGsmStats gprsStatus;
Preferences preferences;

char macStr[18];                      // my mac address
bool gprsConnected=false;

struct HubConfig config;

const int connectionTimeout = 3 * 60000; //time in ms to trigger the watchdog
Watchdog connectionWatchdog;

const int lockupTimeout = 1000; //time in ms to trigger the watchdog

void resetModuleFromLockup() {
  ets_printf("WDT triggered from lockup\n");
  esp_restart();
}

void resetModuleFromNoConnection() {
  ets_printf("WDT triggered from no connection\n");
  esp_restart();
}

void resetConfig() {
    // we're resetting the config or there isn't one
    Serial.println("Reset config");
    preferences.putBytes("config", &default_config, sizeof(HubConfig));
    preferences.putBool("configinit", true);
    memcpy( &config, &default_config, sizeof(HubConfig));
}

void getConfig() {
  preferences.begin(TAG, false);

  // if we have a stored config and we're not resetting, then load the config
  if (preferences.getBool("configinit"))
  {
    preferences.getBytes("config", &config, sizeof(HubConfig));
  }
  else
  {
    resetConfig();
  }
}

void btCallback(byte* payload, unsigned int len) {
    SendMQTTBinary(payload, len);
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
}

void GetMyMacAddress()
{
  uint8_t array[6];
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
}

void SendMQTTBinary(uint8_t *report, int packetSize)
{
  char topic[32];
  sprintf(topic, "bongo/%s/sensor", macStr);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  mqttGPRS->sendMQTTBinary(report, packetSize);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
}

void SystemCheck() {
  Serial.printf("Rev %u Freq %d \n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("PSRAM avail %u free %u\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("FLASH size  %u spd  %u\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.printf("HEAP  size  %u free  %u\n", ESP.getHeapSize(), ESP.getFreeHeap());
}

void loop() {
  bt.loop();
  static int counter = 0;
  counter += 1;
  gprsConnected = mqttGPRS->ModemCheck();
  if (gprsConnected)
    connectionWatchdog.clrWatchdog();
  Serial.println(touchRead(TOUCH_PIN));  // get value of Touch 0 pin = GPIO 4    
}

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  Serial.begin(115200);
  while (!Serial);
  esp_log_level_set("*", ESP_LOG_DEBUG);
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa HUB");

  getConfig();
  SystemCheck();
  GetMyMacAddress();

  mqttGPRS = new MqttGsmClient();
  mqttGPRS->init(config.broker, macStr, config.apn, config.gprsUser, config.gprsPass, config.simPin, mqttCallback);

  bt.init(btCallback);
  Serial.printf("End of setup\n");

  connectionWatchdog.setupWatchdog(connectionTimeout, resetModuleFromNoConnection);
}
