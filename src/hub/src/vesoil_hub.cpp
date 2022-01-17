// https://github.com/me-no-dev/ESPAsyncWebServer#basic-response-with-http-code
// https://github.com/cyberman54/ESP32-Paxcounter/blob/82fdfb9ca129f71973a1f912a04aa8c7c5232a87/src/main.cpp
// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/log.html
// https://github.com/LilyGO/TTGO-T-Beam
// Pin map http://tinymicros.com/wiki/TTGO_T-Beam
// https://github.com/Xinyuan-LilyGO/TTGO-T-Beam
// https://github.com/LilyGO/TTGO-LORA32

// environment variables
// LOCALAPPDATA = C:\Users\username\AppData\Local

static const char * TAG = "Hub";

#define SerialMon Serial
#define APP_VERSION 1

#ifdef HAS_GSM
#define TINY_GSM_DEBUG SerialMon
#endif

#include <vesoil.h>
#include <SPI.h>
#include <Wire.h>  
#include <LoRa.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include <CayenneLPP.h>
#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.
#include "mqtt_wifi.h"
#include "mqtt_gsm.h"
#include "vesoil_hub.h"
#include <wd.h>

#ifdef HAS_OLED
#include <SSD1306.h>
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif

bool needReset = false;

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
#define AT_RX        13
#define AT_TX        12
#define TINY_GSM_DEBUG SerialMon
MqttGsmClient mqttGPRS(AT_RX, AT_TX);
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

void displayUpdate()
{
  DisplayPage(currentPage);
}

void ShowNextPage()
{
    currentPage++;

    if (currentPage==6)
      currentPage=0;

    displayUpdate();
}

#ifdef HAS_DISPLAY

#define L1 0
#define L2 12
#define L3 24
#define L4 36
#define L5 48
#define C1 0
#define C2 32
#define C3 64
#define C4 96

void DisplayPage(int page)
{
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    switch(currentPage)
    {
        // status
        case 0:
          display.drawString(C1, L1, "GSM");
          display.drawString(C2, L1, mqttGPRS.getGsmStage());
          display.drawString(C3, L1, mqttGPRS.getGsmStatus());

          display.drawString(C1, L2, "WiFi");
          display.drawString(C2, L2, wifiStatus);

          display.drawString(C1, L3, "LORA");
          display.drawString(C2, L3, "Pkts");
          display.drawString(C3, L3, String(packetcount,DEC));
          break;
        case 1:
          display.drawString(0, 0, "Wifi Status");
          if (wifiConnected)
          {
            display.drawString(C1, L2, "On");
          }
          else
          {
            display.drawString(C1, L2, "Off");
          }
          break;
        case 2:
#ifdef HAS_GSM        
          display.drawString(C1, L1, "Gsm Status");
          display.drawString(C1, L2, "Volts");
          display.drawString(C2, L2, String(gprsStatus.gsmVolts, DEC));
          display.drawString(C3, L2, "dB");
          display.drawString(C4, L2, String(gprsStatus.signalQuality, DEC));
          display.drawString(C1, L3, "net");
          display.drawString(C2, L3, gprsStatus.netConnected?"1":"0");
          display.drawString(C3, L3, "gprs");
          display.drawString(C4, L3, gprsStatus.gprsConnected?"1":"0");
          display.drawString(C1, L4, "mqtt");
          display.drawString(C2, L4, gprsStatus.mqttConnected?"1":"0");
#else
          display.drawString(C1, L1, "Gsm Status");
          display.drawString(C1, L2, "** DISABLED **");
#endif          
          break;
        case 3:
          display.drawString(C1, L1, "LoRa Status");
          display.drawString(C1, L2, "rssi");
          display.drawString(C2, L2, String(rssi, 2));
          display.drawString(C1, L2, "snr");
          display.drawString(C2, L2, String(snr, 2));
          display.drawString(C1, L3, "pfe");
          display.drawString(C2, L3, String(pfe, DEC));
          display.drawString(C1, L4, "pkts");
          display.drawString(C2, L4, String(packetcount,DEC));
          break;
        case 4:
          display.drawString(C1, L1, "LoRa Settings");
          display.drawString(C1, L2, "Freq");
          display.drawString(C2, L2, String(config.frequency, DEC));
          display.drawString(C1, L2, "Code");
          display.drawString(C2, L2, String(config.codingRate, DEC));
          display.drawString(C1, L3, "Spread");
          display.drawString(C2, L3, String(config.spreadFactor, DEC));
          display.drawString(C1, L4, "Bwidth");
          display.drawString(C2, L4, String(config.bandwidth,DEC));
          display.drawString(C1, L4, "CRC");
          display.drawString(C2, L4, String(config.enableCRC,DEC));
          break;
        case 5:
          // screen saver
          break;
        default:
          break;
    }
    display.display();
}

void InitOLED() {
  pinMode(OLED_RST,OUTPUT);
  digitalWrite(OLED_RST, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high、
  delay(50); 
  display.init();
  display.displayOn();
  display.clear();
  display.drawString(0, 0, "Starting..");
}

#endif

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
  const int interval = 100;
  STARTUPMODE startup_mode = NORMAL;

  // get startup by detecting how many seconds the button is held
  int btndown = 0;
  int pin=0;
  do {
    pin = digitalRead(BTN1);
    Serial.printf("pin=%d\n",pin);
    delay(interval);
    btndown += interval;
  } while (pin==0);
  
  
  if (btndown<2000)
    startup_mode = NORMAL;   // NORMAL
  else 
    startup_mode = RESET;   // reset config

  Serial.printf("button held for %d mode is %d\n", btndown, startup_mode );

  buttonState = digitalRead(BTN1);

  return startup_mode;
}

int detectDebouncedBtnPush() {
  int currentState = digitalRead(BTN1);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // ignore if nothing has changed
  if (currentState == buttonState)
    return 0;

  // ignore if time delay too quick
  unsigned long delay = (millis() - lastButtonTime);
  if (delay < debounceDelay)
    return 0;

  // whatever the reading is at, it's been there for longer than the debounce
  // delay, so take it as the actual current state:
  // if the button state has changed:
  buttonState = currentState;
  lastButtonTime = millis();

  // only toggle the LED if the new button state is HIGH
  if (buttonState == HIGH) {
    return (delay>btnLongDelay)?2:1;
  }
  return 0;
}

#ifdef HAS_LORA

void startLoRa() {

  Serial.printf("Starting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu\n", config.frequency, config.enableCRC, config.codingRate, config.spreadFactor, config.bandwidth);

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);

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

  // setting the sync work breaks the transmission.
  if (config.syncword>0)
    LoRa.setSyncWord(config.syncword);  

  LoRa.setSignalBandwidth(config.bandwidth);

  LoRa.receive();
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
    displayUpdate();
    CayenneLPP lpp(packetSize + 20);
    DynamicJsonDocument jsonBuffer(4096);
    JsonArray root = jsonBuffer.to<JsonArray>();

    LoRa.readBytes(lpp._buffer, packetSize);

    lpp._cursor = packetSize;

    lpp.addGenericSensor(CH_SNR, snr);
    lpp.addGenericSensor(CH_RSSI, rssi);
    lpp.addGenericSensor(CH_PFE, (float)pfe);    

    lpp.decode(lpp._buffer, lpp._cursor, root);
    serializeJsonPretty(root, Serial);
    Serial.println("");
       
    SendMQTTBinary(lpp._buffer, lpp._cursor);
  }  
}

#endif

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  incomingCount++;
  snprintf(incomingMessage, sizeof(incomingMessage), "#%d %s",incomingCount, (char *) payload);
  displayUpdate();
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
  if (mqttGPRS.sendMQTTBinary(report, packetSize))
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
  IotWebConfParameterGroup mqttGroup = IotWebConfParameterGroup("mqtt", "MQTT configuration");
  IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("MQTT server","MQTTserver", config.broker, sizeof(config.broker), DEFAULT_BROKER);
  IotWebConfTextParameter mqttAPNParam = IotWebConfTextParameter("APN","APN", config.apn, sizeof(config.apn), DEFAULT_APN);
  IotWebConfTextParameter gprsUserNameParam = IotWebConfTextParameter("GPRS user","GPRSuser", config.gprsUser, sizeof(config.gprsUser), DEFAULT_GPRSUSER);
  IotWebConfPasswordParameter gprsUserPasswordParam = IotWebConfPasswordParameter("GPRS pwd","GPRSpwd", config.gprsPass, sizeof(config.gprsPass),DEFAULT_GPRSPWD);
  mqttGroup.addItem(&mqttServerParam);
  mqttGroup.addItem(&mqttAPNParam);
  mqttGroup.addItem(&gprsUserNameParam);
  mqttGroup.addItem(&gprsUserPasswordParam);
  iotWebConf.addParameterGroup(&mqttGroup);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setStatusPin(LED_BUILTIN);
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
    displayUpdate();
    Serial.printf("x");
  }  

#ifdef HAS_GSM
    mqttGPRS.ModemCheck();
    mqttGPRS.getStats(gprsStatus);
    gprsConnected = gprsStatus.gprsConnected;
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
    displayUpdate();
  }
  else
  {
    wifiConnected = mqttWifiClient.isWifiConnected();
  }
#endif

  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);

  int btnState = detectDebouncedBtnPush();
  if (btnState==1)
  {
    ShowNextPage();
  }
  
  if (gprsConnected || wifiConnected)
    connectionWatchdog.clrWatchdog();
}

void setup() {
  
  incomingMessage[0]='\0';

  pinMode(BTN1, INPUT);        // Button 1
  Serial.begin(115200);
  while (!Serial);
  delay(1000); 
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa HUB");

#ifdef HAS_OLED
  InitOLED();
#endif

  Serial.println("get startup");
  STARTUPMODE startup_mode = getStartupMode();
  
  Serial.println("get config");
  getConfig(startup_mode);

  Serial.println("system check");
  SystemCheck();
  GetMyMacAddress();

#ifdef HAS_WIFI
  setupWifiConfigurator();
  mqttWifiClient.init(config.broker, macStr, mqttCallback, displayUpdate);
  mqttWifiClient.doSetupWifiMQTT();
#endif

  startLoRa();
  
#ifdef HAS_GSM
  mqttGPRS.init(config.broker, macStr, config.apn, config.gprsUser, config.gprsPass, mqttCallback, displayUpdate);
#endif

  displayUpdate();

  Serial.printf("End of setup\n");

  connectionWatchdog.setupWatchdog(connectionTimeout, resetModuleFromNoConnection);
  //lockupWatchdog.setupWatchdog(lockupTimeout, resetModuleFromLockup);
}
