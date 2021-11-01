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


#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   1024
#define TINY_GSM_DEBUG SerialMon
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#define GSM_PIN ""

const char * Msg_Quality = "Db ";
const char * Msg_Network = "Radio";
const char * Msg_GPRS    = "GPRS";
const char * Msg_MQTT    = "MQTT";

const char * Msg_Failed     = "Failed";
const char * Msg_Connected  = "Connected";
const char * Msg_Connecting = "Connecting";
const char * Msg_FailedToConnect = "Failed Connection";
char gpsjson[32];


enum MODEM_STATE
{
    MODEM_INIT,
    MODEM_NOT_CONNECTED,
    MODEM_CONNECTED,
    GPRS_CONNECTED,
    MQ_CONNECTED,
};

enum MODEM_STATE modem_state=MODEM_INIT;

#define APP_VERSION 1

//#include <ESPAsyncWebServer.h>
#include <vesoil.h>
#include <SmartWifi.h>
#include <SPI.h>
#include <Wire.h>  
#include <LoRa.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include <CayenneLPP.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <geohash.h>
#include <SoftwareSerial.h>
#include <IotWebConf.h>
#include "vesoil_hub.h"

#ifdef HAS_OLED
#include <SSD1306.h>
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif



const char thingName[] = "";                     // -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char wifiInitialApPassword[] = "LoraHub";  // -- Initial password to connect to the Thing, when it creates an own Access Point.
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);


WiFiClient espClient;
int lastWifiStatus=-1;
int lastMqttConnected=-1;
PubSubClient mqttWifi(espClient);

SoftwareSerial SerialAT(AT_RX, AT_TX, false);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqttGPRS(client);

GeoHash hasher(8);

//AsyncWebServer server(80);
Preferences preferences;
float snr = 0;                        // signal noise ratio
float rssi = 0;                       // Received Signal Strength Indicator
long  pfe=0;                          // frequency error
char macStr[18];                      // my mac address
char incomingMessage[128];            // incoming mqtt
int  incomingCount=0;
int packetcount=0;
bool wifiConnected=false;
int buttonState = HIGH;               // the current reading from the input pin
unsigned long lastButtonTime = 0;     // the last time the output pin was toggled
unsigned long debounceDelay = 50;     // the debounce time; increase if the output flickers
unsigned long btnLongDelay = 1000;    // the debounce time; increase if the output flickers
String address;                       // current ipaddress

String gsmStage;
String gsmStatus;
String wifiStatus;

int currentPage=0;
uint8_t *report;
bool haveReport=false;

struct HubConfig config;

esp_timer_handle_t oneshot_timer;

void ShowNextPage()
{
    currentPage++;

    if (currentPage==6)
      currentPage=0;

    DisplayPage(currentPage);
}

#define L1 0
#define L2 12
#define L3 24
#define L4 36
#define L5 48
#define C1 0
#define C2 32
#define C3 64

#ifdef HAS_DISPLAY

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
          display.drawString(C2, L1, gsmStage);
          display.drawString(C3, L1, gsmStatus);

          display.drawString(C1, L2, "WiFi");
          display.drawString(C2, L2, wifiStatus);

          display.drawString(C1, L3, "LORA");
          display.drawString(C2, L3, "Pkts");
          display.drawString(C3, L3, String(packetcount,DEC));
          break;
        case 1:
          display.drawString(0, 0, "Wifi Status");
          display.drawString(C2, L2, address);
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
          display.drawString(C1, L1, "Gsm Status");
          display.drawString(C1, L2, "dB");
          //display.drawString(C2, L2, String(modem.getSignalQuality(), DEC));
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
  //display.flipScreenVertically();  
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
  DisplayPage(currentPage);
}

#endif

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  //snprintf(incomingMessage, sizeof(incomingMessage), "%s",(char *) payload);
  incomingCount++;
  snprintf(incomingMessage, sizeof(incomingMessage), "#%d %s",incomingCount, (char *) payload);
  DisplayPage(currentPage);
}

void GetMyMacAddress()
{
  uint8_t array[6];
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
}

#ifdef HAS_GSM
void doSetupGprsMQTT()
{
  mqttGPRS.setServer(config.broker, 1883);
  mqttGPRS.setCallback(mqttCallback);
}


void doModemStartGPRS()
{  
  Serial.println("Modem Start");
  gsmStage = "Modem";
  gsmStatus = "Starting";
  DisplayPage(currentPage);
  //delay(1000);
  Serial.println("modem init");

  TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,9600);
  Serial.println("modem restart");
  modem.restart();  
  String modemInfo = modem.getModemInfo();
  //delay(1000);  
  gsmStage = "Radio";
  modem_state=MODEM_NOT_CONNECTED;
}

void doModemStart()
{  
    if (config.gprsEnabled)
      doModemStartGPRS();
}

void waitForNetwork() 
{
  SimStatus simStatus = modem.getSimStatus();
  RegStatus regStatus = modem.getRegistrationStatus();
  String thisgsmStatus = String("Sim: ") + String(simStatus, DEC) + String(" Reg: ") + String(regStatus, DEC);

  if (gsmStatus != thisgsmStatus)
  {
    gsmStatus = thisgsmStatus;
    DisplayPage(currentPage);    
  }

  if (modem.isNetworkConnected()) 
  { 
    gsmStage = Msg_Network;
    gsmStatus = Msg_Connected;
    DisplayPage(currentPage);
    modem_state=MODEM_CONNECTED;
  }
}

void doGPRSConnect()
{
  Serial.println("GPRS Connect");

  gsmStage = Msg_GPRS;
  gsmStatus = Msg_Connecting;
  DisplayPage(currentPage);
    
  bool connected = modem.gprsConnect(config.apn, config.gprsUser, config.gprsPass);    

  gsmStatus = (char*)(connected ? Msg_Connected : Msg_Failed);
  DisplayPage(currentPage);

  if (connected) 
  { 
    modem_state=GPRS_CONNECTED;
  }
}

void mqttGPRSConnect()
{
  char topic[32];

  if (!mqttGPRS.connected())
  {
    gsmStage = Msg_MQTT;
    gsmStatus = Msg_Connecting;
    DisplayPage(currentPage);
    boolean mqttConnected = mqttGPRS.connect(macStr);
    if (!mqttConnected)
    {
      gsmStatus = Msg_FailedToConnect;
    }
    else
    {
      sprintf(topic, "bongo/%s/hub", macStr);
      mqttGPRS.subscribe(topic);  
      mqttGPRS.publish(topic, "{\"state\":\"connected\"}", true);
      modem_state=MQ_CONNECTED;
    }
  }
}  


void mqttGPRSPoll()
{
  bool gprsConnected = modem.isGprsConnected();
  if (!gprsConnected)
    modem_state=MODEM_NOT_CONNECTED;
  mqttGPRS.loop();
}

void ModemCheck()
{
  switch(modem_state)
  {
    case MODEM_INIT:
      doModemStart();
      break;
    case MODEM_NOT_CONNECTED:
      waitForNetwork();
      break;
    case MODEM_CONNECTED:
      doGPRSConnect();
      break;
    case GPRS_CONNECTED:
      mqttGPRSConnect();
      break;      
    case MQ_CONNECTED:
      gsmStatus = Msg_Connected;
      mqttGPRSPoll();
      break;      
  }
}


#endif

#ifdef HAS_WIFI

void doSetupWifiMQTT()
{
  mqttWifi.setCallback(mqttCallback);
  mqttWifi.setServer(config.broker,1883);

}

void mqttWiFiConnect()
{
  char topic[32];

  boolean mqttConnected = mqttWifi.connect(macStr);
  if (mqttConnected)
  {
    Serial.printf("MQTT Connected over WiFi\n");
    sprintf(topic, "bongo/%s/hub", macStr);
    mqttWifi.subscribe(topic);  
    mqttWifi.publish(topic, "{\"state\":\"connected\"}", true);
  }
}  

bool isWifiConnected() {

  iotWebConf.doLoop();
  int WiFiStatus = WiFi.status();

  if (WiFiStatus == WL_CONNECTED)
  {
    int mqttConnected = mqttWifi.connected() ? 1:0;
    if (lastMqttConnected != mqttConnected)
    {
        wifiStatus = mqttConnected==1?"MQTT Connected":"MQTT Disconnected";
        DisplayPage(currentPage);
    }

    if (mqttConnected==0)
    {
        mqttWiFiConnect();
    }

    mqttWifi.loop();

    lastMqttConnected = mqttConnected;
  }
  else
  {
    if (lastWifiStatus != WiFiStatus)
    {
        switch (WiFiStatus) {
          case WL_CONNECTED:
            wifiStatus = "Connected";
            break;
          case WL_IDLE_STATUS:
            wifiStatus = "Idle";
            break;
          case WL_NO_SSID_AVAIL:
            wifiStatus = "No SSID";
            break;
          case WL_SCAN_COMPLETED:
            wifiStatus = "Scan done";
            break;
          case WL_CONNECT_FAILED:
            wifiStatus = "Connect failed";
            break;
          case WL_CONNECTION_LOST:
            wifiStatus = "Connect lost";
            break;
          case WL_DISCONNECTED:
            wifiStatus = "Disconnected";
            break;
      }
      DisplayPage(currentPage);
    }
  }

  lastWifiStatus = WiFiStatus;
  return mqttWifi.connected();
}


#endif


void SendMQTTBinary(uint8_t *report, int packetSize)
{
  char topic[32];
  sprintf(topic, "bongo/%s/sensor", macStr);

  if (mqttWifi.connected())
  {
      if (!mqttWifi.publish(topic, report, packetSize))
      {
        Serial.printf("MQTT Wifi Fail\n");
      } 
      else
      {
        Serial.printf("MQTT Sent Wifi %d bytes to %s\n", packetSize, topic);
      }
      return;
  }
  
  if (modem_state == MQ_CONNECTED)
  {
    if (!mqttGPRS.publish(topic, report, packetSize))
    {
      Serial.printf("MQTT GPRS Fail\n");
    }
      else
      {
        Serial.printf("MQTT Sent GPRS %d bytes to %s\n", packetSize, topic);
      }
    return;    
  }

  Serial.printf("MQTT Failed, GPRS and WiFi not connected\n");
  return;
}

void loop() {

  ModemCheck();
  
  wifiConnected = isWifiConnected();

  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);

  int btnState = detectDebouncedBtnPush();
  if (btnState==1)
  {
    ShowNextPage();
  }
}

void SystemCheck() {
  Serial.printf("Rev %u Freq %d \n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("PSRAM avail %u free %u\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("FLASH size  %u spd  %u\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.printf("HEAP  size  %u free  %u\n", ESP.getHeapSize(), ESP.getFreeHeap());
}

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

void setupWifi() {
  iotWebConf.init();

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });
}

void setup() {
  
  incomingMessage[0]='\0';

  pinMode(BTN1, INPUT);        // Button 1
  Serial.begin(115200);
  while (!Serial);
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

  setupWifi();

  GetMyMacAddress();

  startLoRa();

  doSetupWifiMQTT();
  doSetupGprsMQTT();
  
  gsmStage = "Booting";
  gsmStatus = "";

  DisplayPage(currentPage);

  Serial.printf("End of setup\n");

}
