//https://github.com/Xinyuan-LilyGO/LilyGO-T-SIM7000G

static const char * TAG = "Hub";

#define APP_VERSION 1

#include <vesoil.h>
#include <SPI.h>
#include <Wire.h>  
#include <LoRa.h>
#include <Preferences.h>
#include <FS.h>
#include <CayenneLPP.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#ifdef HAS_WIFI
#include "mqtt_wifi.h"
#endif
#include "mqtt_gsm.h"
#include "main.h"
#include "ringbuffer.h"
#include <wd.h>

bool needReset = false;
RingBuffer ringBuffer(16);

#ifdef HAS_WIFI
const char thingName[] = "Hub";               // -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char wifiInitialApPassword[] = "";      // -- Initial password to connect to the Thing, when it creates an own Access Point.
const char configVersion[] = "1.2";
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, configVersion);
MqttWifiClient mqttWifiClient;
#endif

#ifdef HAS_GSM
MqttGsmClient *mqttGPRS;
MqttGsmStats gprsStatus;
#endif

Preferences preferences;
float snr = 0;                        // signal noise ratio
float rssi = 0;                       // Received Signal Strength Indicator
long  pfe=0;                          // frequency error
char macStr[18];                      // my mac address
char incomingMessage[128];            // incoming mqtt
int  incomingCount=0;
int packetcount=0;
bool wifiConnected=false;
bool gprsConnected=false;
int buttonState = HIGH;               // the current reading from the input pin
unsigned long lastButtonTime = 0;     // the last time the output pin was toggled
unsigned long debounceDelay = 50;     // the debounce time; increase if the output flickers
unsigned long btnLongDelay = 1000;    // the debounce time; increase if the output flickers

String wifiStatus;

int currentPage=0;
uint8_t *report;
bool haveReport=false;

struct HubConfig config;

const int connectionTimeout = 3 * 60000; //time in ms to trigger the watchdog
Watchdog connectionWatchdog;

const int lockupTimeout = 1000; //time in ms to trigger the watchdog
Watchdog lockupWatchdog;

void resetModuleFromLockup() {
  ets_printf("WDT triggered from lockup\n");
  esp_restart();
}

void resetModuleFromNoConnection() {
  ets_printf("WDT triggered from no connection\n");
  esp_restart();
}

void getConfig(STARTUPMODE startup_mode) {

//////////////////// HACK /////////////////////
    memcpy( &config, &default_config, sizeof(HubConfig));
    return;

  preferences.begin(TAG, false);

  // if we have a stored config and we're not resetting, then load the config
  if (preferences.getBool("configinit") && startup_mode != RESET)
  {
    preferences.getBytes("config", &config, sizeof(HubConfig));
  }
  else
  {
    // we're resetting the config or there isn't one
    Serial.println("Reset config");
    preferences.putBytes("config", &default_config, sizeof(HubConfig));
    preferences.putBool("configinit", true);
    memcpy( &config, &default_config, sizeof(HubConfig));
  }
}

STARTUPMODE getStartupMode() {
  STARTUPMODE startup_mode = NORMAL;
  return startup_mode;
}

int HashCode (uint8_t* buffer, int size) {
    int h = 0;
    for (uint8_t* ptr = buffer; size>0; --size, ++ptr)
        h = h * 31 + *ptr;
    return h;
}

void readLoraData(int packetSize) 
{  
  if (packetSize>0) { 
    Serial.printf("lora data %d bytes\n",packetSize);
    snr = LoRa.packetSnr();
    rssi = LoRa.packetRssi();
    pfe = LoRa.packetFrequencyError();

    Serial.printf("Got %d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 

    haveReport = true;
    packetcount++; 
    CayenneLPP lpp(packetSize + 20);

    LoRa.readBytes(lpp._buffer, packetSize);

    int hash = HashCode(lpp._buffer, packetSize);
    
    if (ringBuffer.exists(hash))
    {
      Serial.printf("Duplicate detected hash %d\n", hash); 
      return;
    }
    else
    {
      Serial.printf("Packet hash %d\n", hash); 
    }

    ringBuffer.add( hash );

    lpp._cursor = packetSize;

    lpp.addGenericSensor(CH_SNR, snr);
    lpp.addGenericSensor(CH_RSSI, rssi);
    lpp.addGenericSensor(CH_PFE, (float)pfe);    

    DynamicJsonDocument jsonBuffer(4096);
    JsonArray root = jsonBuffer.to<JsonArray>();
    lpp.decode(lpp._buffer, lpp._cursor, root);
    serializeJsonPretty(root, Serial);
    Serial.println("");
       
    SendMQTTBinary(lpp._buffer, lpp._cursor);
  }  
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  incomingCount++;
  snprintf(incomingMessage, sizeof(incomingMessage), "#%d %s",incomingCount, (char *) payload);
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

#ifdef HAS_WIFI
  if (mqttWifiClient.sendMQTTBinary(report, packetSize))
    return;
#endif

#ifdef HAS_GSM
  if (mqttGPRS->sendMQTTBinary(report, packetSize))
    return;
#endif

  Serial.printf("MQTT Failed, GPRS and WiFi not connected\n");
  return;
}

#ifdef HAS_WIFI
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 02 Status and Reset</title></head><body>";
  s += "Go to <a href='config'>configure page</a> to change settings.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  needReset = true;
}

void setupWifiConfigurator() {
  // IotWebConfParameterGroup mqttGroup = IotWebConfParameterGroup("mqtt", "MQTT configuration");
  // IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("MQTT server","MQTTserver", config.broker, sizeof(config.broker), DEFAULT_BROKER);
  // IotWebConfTextParameter mqttAPNParam = IotWebConfTextParameter("APN","APN", config.apn, sizeof(config.apn), DEFAULT_APN);
  // IotWebConfTextParameter gprsUserNameParam = IotWebConfTextParameter("GPRS user","GPRSuser", config.gprsUser, sizeof(config.gprsUser), DEFAULT_GPRSUSER);
  // IotWebConfPasswordParameter gprsUserPasswordParam = IotWebConfPasswordParameter("GPRS pwd","GPRSpwd", config.gprsPass, sizeof(config.gprsPass),DEFAULT_GPRSPWD);
  // mqttGroup.addItem(&mqttServerParam);
  // mqttGroup.addItem(&mqttAPNParam);
  // mqttGroup.addItem(&gprsUserNameParam);
  // mqttGroup.addItem(&gprsUserPasswordParam);
  // iotWebConf.addParameterGroup(&mqttGroup);
  // iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.init();
  
  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
}
#endif

void SystemCheck() {
  Serial.printf("Rev %u Freq %d \n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("PSRAM avail %u free %u\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("FLASH size  %u spd  %u\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.printf("HEAP  size  %u free  %u\n", ESP.getHeapSize(), ESP.getFreeHeap());
}

void loop() {
  static int counter = 0;
  counter += 1;
  if ( (counter % 100) == 0)
  {
    //lockupWatchdog.clrWatchdog();
  }  

#ifdef HAS_GSM
    gprsConnected = mqttGPRS->ModemCheck();
#endif

#ifdef HAS_WIFI
  if (needReset)
  {
    Serial.println("Rebooting after 1 second.");
    iotWebConf.delay(1000);
    ESP.restart();
  }  

  iotWebConf.doLoop();
  iotwebconf::NetworkState iotState = iotWebConf.getState();
  if (iotState !=  iotwebconf::NetworkState::OnLine)
  {
    static const char *enum_str[] = { "Boot", "Not configured", "APMode", "Connecting", "Online", "Offline" };

    wifiStatus = enum_str[iotState];
  }
  else
  {
    wifiConnected = mqttWifiClient.isWifiConnected();
  }
#endif
  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);

  if (gprsConnected || wifiConnected)
    connectionWatchdog.clrWatchdog();
}

void setup() {
  
  incomingMessage[0]='\0';

  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa HUB");

  Serial.println("get startup");
  STARTUPMODE startup_mode = getStartupMode();
  
  Serial.println("get config");
  getConfig(startup_mode);

  Serial.println("system check");
  SystemCheck();
  GetMyMacAddress();

#ifdef HAS_WIFI
  setupWifiConfigurator();
  mqttWifiClient.init(config.broker, macStr, mqttCallback);
  mqttWifiClient.doSetupWifiMQTT();
#endif

#ifdef HAS_GSM
  mqttGPRS = new MqttGsmClient();
  mqttGPRS->init(config.broker, macStr, config.apn, config.gprsUser, config.gprsPass, config.simPin, mqttCallback);
#endif

  Serial.printf("End of setup\n");

  connectionWatchdog.setupWatchdog(connectionTimeout, resetModuleFromNoConnection);
  //lockupWatchdog.setupWatchdog(lockupTimeout, resetModuleFromLockup);
}
