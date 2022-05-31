#if 1
#include <Arduino.h>
#include <SPI.h>
#include "main.h"

SPIClass SPI1(HSPI);

void loop() {
}


void setup() {
  SPI1.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa HUB");
}
#else

// https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G
#define APP_VERSION 1

#include <vesoil.h>

#ifdef HAS_LORA
#include <LoRa.h>
#endif

#include <Wire.h>  
#include "mqtt_gsm.h"
#include "main.h"
#include <wd.h>
#include <Preferences.h>
#include <CayenneLPP.h>
#include "messagebuffer.h"
#include "base64.hpp"

#ifdef HAS_BLUETOOTH
#include "BleServer.h"
#endif

static const char * TAG = "Hub1.0";

#ifdef HAS_BLUETOOTH
BleServer bt;
#endif

#ifdef HAS_GSM
MqttGsmClient *mqttGPRS;
MqttGsmStats gprsStatus;
#endif

Preferences preferences;
MessageBuffer messageBuffer(128);

char macStr[18];                      // my mac address
bool gprsConnected=false;
int packetcount=0;
int duppacketcount=0;
struct HubConfig config;

const int connectionTimeout = 3 * 60000; //time in ms to trigger the watchdog
Watchdog connectionWatchdog;

#ifdef HAS_LORA
SPIClass SPI1(HSPI);
#endif

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

static void btCallback(byte* payload, unsigned int len) {
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  // receiving stuff back from the cloud
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

#ifdef HAS_GSM
  mqttGPRS->sendMQTTBinary(report, packetSize);
#endif

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
}

void SystemCheck() {
  Serial.printf("Rev %u Freq %d \n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("PSRAM avail %u free %u\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("FLASH size  %u spd  %u\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.printf("HEAP  size  %u free  %u\n", ESP.getHeapSize(), ESP.getFreeHeap());
}

#ifdef HAS_LORA
void LoraReceive(int packetSize) 
{  
    Serial.printf("lora data %d bytes\n",packetSize);
    float snr = LoRa.packetSnr();
    float rssi = LoRa.packetRssi();
    long pfe = LoRa.packetFrequencyError();
    CayenneLPP lpp(packetSize + 20);
    Serial.printf("Got %d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 

    LoRa.readBytes(lpp._buffer, packetSize);

    // add signal strength etc
    lpp._cursor = packetSize;
    lpp.addGenericSensor(CH_SNR, snr);
    lpp.addGenericSensor(CH_RSSI, rssi);
    lpp.addGenericSensor(CH_PFE, (float)pfe);  

#ifdef HAS_GSM
    SendMQTTBinary(lpp._buffer, lpp._cursor);
#endif

    // convert to base64
    int size = lpp._cursor * 2;
    uint8_t * base64_buffer = (uint8_t *)malloc(size);
    memset(base64_buffer,0,size);
    int base64_length = encode_base64(lpp._buffer, lpp._cursor, base64_buffer);
    Serial.printf("set value %d bytes %s\n", base64_length, base64_buffer); 

    if (messageBuffer.add(base64_buffer))
    {
      ++packetcount;
#ifdef HAS_BLUETOOTH
      bt.sendData(base64_buffer);
#endif
    }
    else
    {
      Serial.printf("Duplicate detected\n"); 
      ++duppacketcount;
      free(base64_buffer);
      return;
    }
}

void startLoRa() {

  Serial.printf("\nStarting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu txpower:%d\n", config.frequency, config.enableCRC, config.codingRate, config.spreadFactor, config.bandwidth, config.txpower);

  SPI1.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(SPI1);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

  int result = LoRa.begin(config.frequency);
  if (!result) 
    Serial.printf("Starting LoRa failed: err %d\n", result);
  else
    Serial.println("Started LoRa OK");

  LoRa.setPreambleLength(config.preamble);
  LoRa.setSpreadingFactor(config.spreadFactor);
  LoRa.setCodingRate4(config.codingRate);

  if (config.enableCRC)
      LoRa.enableCrc();
    else 
      LoRa.disableCrc();

  if (config.syncword>0)
    LoRa.setSyncWord(config.syncword);    

  LoRa.setSignalBandwidth(config.bandwidth);

  LoRa.setTxPower(config.txpower);
  LoRa.receive();
  
  if (!result) {
    Serial.printf("Starting LoRa failed: err %d", result);
  }  
}
#endif


void loop() {
  static int counter = 0;
  counter += 1;
  
#ifdef HAS_BLUETOOTH
  bt.loop();
#endif  

#ifdef HAS_GSM
  gprsConnected = mqttGPRS->ModemCheck();
  if (gprsConnected)
    connectionWatchdog.clrWatchdog();
#endif

#ifdef HAS_LORA
  int packetSize = LoRa.parsePacket();
  if (packetSize>0)
    LoraReceive(packetSize);
#endif

  // Serial.println(touchRead(TOUCH_PIN));  // get value of Touch 0 pin = GPIO 4    
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

#ifdef HAS_GSM
  mqttGPRS = new MqttGsmClient();
  mqttGPRS->init(config.broker, macStr, config.apn, config.gprsUser, config.gprsPass, config.simPin, mqttCallback);
#endif

#ifdef HAS_LORA
  startLoRa();
#endif


#ifdef HAS_BLUETOOTH
  bt.init("LoraHub2", NULL);
#endif

  Serial.printf("End of setup\n");

  connectionWatchdog.setupWatchdog(connectionTimeout, resetModuleFromNoConnection);
}
#endif