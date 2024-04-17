#define APP_VERSION 1

#include <vesoil.h>

#ifdef HAS_LORA
#include <LoRa.h>
#endif

#include <Wire.h>  
#include "main.h"
#include <wd.h>
#include <Preferences.h>
#include <CayenneLPP.h>
#include "base64.hpp"
#include <WiFi.h>
#include "AsyncMqttClient.h"

#ifdef HAS_BLUETOOTH
#include "messagebuffer.h"
#include "BleServer.h"
#endif

static const char * TAG = "Hub1.1";

#ifdef HAS_BLUETOOTH

BleServer bt;
MessageBuffer messageBuffer(128);
#endif

#define MQTT_HOST         DEFAULT_BROKER
#define MQTT_PORT         1883
#define WIFI_SSID         ENV_WIFI_SSID
#define WIFI_PASSWORD     ENV_WIFI_PASSWORD

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

Preferences preferences;

char macStr[18];                      // my mac address
bool gprsConnected=false;
int packetcount=0;
int duppacketcount=0;
struct HubConfig config;
char topic[64];
char connStatus[64];

const int connectionTimeout = 15 * 60000; //time in ms to trigger the watchdog
Watchdog connectionWatchdog;

#ifdef HAS_LORA
SPIClass * hspi = NULL;
#endif

const int lockupTimeout = 1000; //time in ms to trigger the watchdog

void connectToWifi()
{
  sprintf(connStatus, "Connecting Wifi");
  Serial.println("Connecting to Wi-Fi...");
  Serial.printf("SSID %s Pwd %s \n", WIFI_SSID, WIFI_PASSWORD);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt()
{
  sprintf(connStatus, "Connecting MQTT");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  sprintf(connStatus, "Connected to MQTT");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;

  sprintf(connStatus, "Disconnected from MQTT");
  
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
  Serial.println(connStatus);
  return false;
}

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

void callback_mqqt(char* topic, byte* payload, unsigned int len) {
  // receiving stuff back from the cloud
}

void GetMyMacAddress()
{
  uint8_t array[6];
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
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
    Serial.printf("lora received %d bytes\n",packetSize);
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

    sendMQTTBinary(lpp._buffer, lpp._cursor);

#ifdef HAS_BLUETOOTH
    // convert to base64
    int size = lpp._cursor * 2;
    uint8_t * base64_buffer = (uint8_t *)malloc(size);
    memset(base64_buffer,0,size);
    int base64_length = encode_base64(lpp._buffer, lpp._cursor, base64_buffer);
    Serial.printf("set value %d bytes %s\n", base64_length, base64_buffer); 

    if (messageBuffer.add(base64_buffer))
    {
      ++packetcount;
      bt.sendData(base64_buffer);
    }
    else
    {
      Serial.printf("Duplicate detected\n"); 
      ++duppacketcount;
      free(base64_buffer);
      return;
    }
#endif

  Serial.printf("lora receive complete\n");
}

void startLoRa() {

  Serial.printf("\nStarting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu txpower:%d\n", config.frequency, config.enableCRC, config.codingRate, config.spreadFactor, config.bandwidth, config.txpower);

  hspi = new SPIClass(HSPI);
  hspi->begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setSPI(*hspi);
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

  static wl_status_t lastStatus=WL_NO_SHIELD;

  delay(10);

  if ( (counter % 300) == 0)
  { 
    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED && lastStatus!=WL_CONNECTED)
    {
        Serial.printf("WIFI Connected\n");
        connectToMqtt();
    }

    if (status != WL_CONNECTED && lastStatus==WL_CONNECTED)
    {
      Serial.printf("WIFI Disconnected\n");
        connectToWifi();
    }

    Serial.printf("%s\n",connStatus);


    lastStatus = status;
  }
  
#ifdef HAS_BLUETOOTH
  bt.loop();
#endif  

#ifdef HAS_LORA
  int packetSize = LoRa.parsePacket();
  if (packetSize>0)
    LoraReceive(packetSize);
#endif

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
  sprintf(topic, "bongo/%s/sensor", macStr);

#ifdef HAS_LORA
  startLoRa();
#endif


#ifdef HAS_BLUETOOTH
  bt.init("LoraHub2", NULL);
#endif

  Serial.printf("End of setup\n");

  //connectionWatchdog.setupWatchdog(connectionTimeout, resetModuleFromNoConnection);

  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  Serial.printf("topics %s\n", topic);
  
  WiFi.disconnect(true);
  connectToWifi();
}
