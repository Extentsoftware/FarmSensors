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

#ifdef HAS_GSM
#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   1024
#define TINY_GSM_DEBUG SerialMon
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#define GSM_PIN ""

const char * Msg_Quality = "Db ";
const char * Msg_Network = "Network";
const char * Msg_GPRS    = "GPRS";
const char * Msg_MQTT    = "MQTT";

const char * Msg_Failed     = "Failed";
const char * Msg_Connected  = "Connected";
const char * Msg_Connecting = "Connecting";
const char * Msg_FailedToConnect = "Failed Connection";
char gpsjson[32];
#endif


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

#include <ESPAsyncWebServer.h>
#include <vesoil.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>  
#include <LoRa.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include <CayenneLPP.h>


#ifdef HAS_GSM
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <geohash.h>
#include <SoftwareSerial.h>
#endif

#include "vesoil_hub.h"

#ifdef HAS_OLED
#include <SSD1306.h>
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif

#ifdef HAS_GSM
SoftwareSerial SerialAT(AT_RX, AT_TX, false);
TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
GeoHash hasher(8);
#endif

AsyncWebServer server(80);
Preferences preferences;
float snr = 0;                        // signal noise ratio
float rssi = 0;                       // Received Signal Strength Indicator
long  pfe=0;                          // frequency error
char macStr[18];                      // my mac address
char incomingMessage[128];            // incoming mqtt
int  incomingCount=0;

int packetcount=0;

bool wifiMode=false;
int buttonState = HIGH;               // the current reading from the input pin
unsigned long lastButtonTime = 0;     // the last time the output pin was toggled
unsigned long debounceDelay = 50;     // the debounce time; increase if the output flickers
unsigned long btnLongDelay = 1000;    // the debounce time; increase if the output flickers
String address;                       // current ipaddress
String networkStage;
String networkStatus;
int currentPage=0;
//SensorReport report;                  // Last report from a sensor
uint8_t *report;
bool haveReport=false;

struct HubConfig config;

esp_timer_handle_t oneshot_timer;

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
  
  Serial.println("get gonfig");
  getConfig(startup_mode);

  Serial.println("system check");
  SystemCheck();

  GetMyMacAddress();

  startLoRa();

  doSetupMQTT();

  setupWifi();
  
  networkStage = "Lora";
  networkStatus = "Ready";

  DisplayPage(currentPage);

  Serial.printf("End of setup\n");

}

void ShowNextPage()
{
    currentPage++;

    if (currentPage==2)
      currentPage=0;

    DisplayPage(currentPage);
}

#define L1 0
#define L2 12
#define L3 24
#define L4 36
#define L5 48
#define C1 0
#define C2 64

void DisplayPage(int page)
{
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    switch(currentPage)
    {
        // status
        case 0:
          display.drawString(C1, L1, networkStage);
          display.drawString(C2, L1, networkStatus);
                    if (wifiMode)
          {
            display.drawString(C1, L2, "On");
            display.drawString(C2, L2, address);
          }
          else
          {
            display.drawString(C1, L2, "Off");
          }
          display.drawString(C1, L3, String("rssi ") + String(rssi, 2));
          display.drawString(C2, L3, String("snr  ") + String(snr, 2));
          display.drawString(C1, L4, String("pfe  ") + String(pfe, DEC));
          display.drawString(C2, L4, String("pkts  ") + String(packetcount,DEC));

          
#ifdef HAS_GSM
          display.drawString(C1, L5, String("Gsm dB: ") + String(modem.getSignalQuality(), DEC));
#endif
          break;
        case 1:
          display.drawString(0, 0, "Wifi Status");
          break;
        case 2:
          display.drawString(0, 0, "Signal");
          display.drawString(0, 16, String("rssi ") + String(rssi, 2));
          display.drawString(64, 16, String("snr  ") + String(snr, 2));
          display.drawString(0, 32, String("pfe  ") + String(pfe, DEC));
          display.drawString(64, 48, String("pkts  ") + String(packetcount,DEC));
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

void getConfig(STARTUPMODE startup_mode) {
  preferences.begin(TAG, false);

  // if we have a stored config and we're not resetting, then load the config
  if (preferences.getBool("configinit") && startup_mode != RESET)
  {
    preferences.getBytes("config", &config, sizeof(HubConfig));
  }
  else
  {
    // we're resetting the config or there isn't one
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

void loop() {
  
#ifdef HAS_GSM
  ModemCheck();
#endif

  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);

  int btnState = detectDebouncedBtnPush();
  if (btnState==1)
  {
    ShowNextPage();
  }

  if (btnState==2) 
  {
    switch(currentPage)
    {
      case 1:
        toggleWifi();
        DisplayPage(currentPage);
        break;
    }
  }
}

void SystemCheck() {
  Serial.printf("Rev %u Freq %d \n", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  Serial.printf("PSRAM avail %u free %u\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("FLASH size  %u spd  %u\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.printf("HEAP  size  %u free  %u\n", ESP.getHeapSize(), ESP.getFreeHeap());
}

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

        
    #ifdef HAS_GSM
        SendMQTTBinary(lpp._buffer, lpp._cursor);
    #endif
  }
  DisplayPage(currentPage);
}

void toggleWifi() {
  wifiMode = !wifiMode;
  if (wifiMode)
    setupWifi();
  else
    exitWifi();
}

void exitWifi() {
  if (wifiMode) {
    Serial.printf("Exit WiFi mode\n");
    WiFi.softAPdisconnect( true );
    server.reset();
    wifiMode = false;
    ESP_ERROR_CHECK(esp_timer_delete(oneshot_timer));
  }
}

void oneshot_timer_callback(void* arg)
{
  exitWifi();
}

void setupWifiTimer() {

    esp_timer_create_args_t oneshot_timer_args;

    oneshot_timer_args.callback = &oneshot_timer_callback;
    oneshot_timer_args.arg = (void*) oneshot_timer;
    oneshot_timer_args.name = "one-shot";

    esp_timer_create( &oneshot_timer_args, &oneshot_timer);
    esp_timer_start_once( oneshot_timer, 5 * 60 * 1000 ); // 5-minute timer
}

String processor(const String& var)
{
  if(var == "SSID")
    return config.ssid;
  if(var == "password")
    return config.password;
  if(var == "apn")
    return config.apn;
  if(var == "gprspass")
    return config.gprsPass;
  if(var == "gprsuser")
    return config.gprsUser;
  if(var == "frequency")
    return String(config.frequency);
  if(var == "spreadFactor")
    return String(config.spreadFactor);
  if(var == "enableCRC")
    return String(config.enableCRC?"checked":"");
  if(var == "codingRate")
    return String(config.codingRate);
  if(var == "bandwidth")
    return String(config.bandwidth);
    
  return String();
}

void setupWifi() {

    if (!SPIFFS.begin())
    {
      Serial.println("Failed to initialise SPIFFS");
    }

    if (strlen(config.password)==0)
      WiFi.softAP(config.ssid);
    else
      WiFi.softAP(config.ssid, config.password);

    address = WiFi.softAPIP().toString();

    wifiMode = true;

    // setupWifiTimer();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.printf("Web request %s\n", request->url().c_str());
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    server.onNotFound(notFound);

    server.begin();
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void GetMyMacAddress()
{
  uint8_t array[6];
  esp_efuse_mac_get_default(array);
  snprintf(macStr, sizeof(macStr), "%02x%02x%02x%02x%02x%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
}


#ifdef HAS_GSM


void doModemStart()
{  
  Serial.println("Modem Start");
  networkStage = "Modem";
  networkStatus = "Starting";
  DisplayPage(currentPage);
  delay(1000);
  Serial.println("modem init");

  TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,9600);
  Serial.println("modem restart");
  modem.restart();  
  String modemInfo = modem.getModemInfo();
  delay(1000);  
  networkStatus = "Started";
  modem_state=MODEM_NOT_CONNECTED;
}

void waitForNetwork() 
{
  networkStage = "Network";
  SimStatus simStatus = modem.getSimStatus();
  RegStatus regStatus = modem.getRegistrationStatus();
  networkStatus = String("Sim: ") + String(simStatus, DEC) + String(" Reg: ") + String(regStatus, DEC);
  DisplayPage(currentPage);
  if (modem.isNetworkConnected()) 
  { 
    networkStage = Msg_Network;
    networkStatus = Msg_Connected;
    DisplayPage(currentPage);
    modem_state=MODEM_CONNECTED;
  }
}

void doGPRSConnect()
{
  Serial.println("GPRS Connect");

  networkStage = Msg_GPRS;
  networkStatus = Msg_Connecting;
  DisplayPage(currentPage);
    
  bool connected = modem.gprsConnect(config.apn, config.gprsUser, config.gprsPass);    

  networkStatus = (char*)(connected ? Msg_Connected : Msg_Failed);
  DisplayPage(currentPage);

  if (connected) 
  { 
    modem_state=GPRS_CONNECTED;
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  //snprintf(incomingMessage, sizeof(incomingMessage), "%s",(char *) payload);
  incomingCount++;
  snprintf(incomingMessage, sizeof(incomingMessage), "#%d %s",incomingCount, (char *) payload);
  DisplayPage(currentPage);
}

void doSetupMQTT()
{
  // MQTT Broker setup
  mqtt.setServer(config.broker, 1883);
  mqtt.setCallback(mqttCallback);
}

void mqttConnect()
{
  char topic[32];

  if (!mqtt.connected())
  {
    networkStage = Msg_MQTT;
    networkStatus = Msg_Connecting;
    DisplayPage(currentPage);
    boolean mqttConnected = mqtt.connect(macStr);
    if (!mqttConnected)
    {
      networkStatus = Msg_FailedToConnect;
    }
    else
    {
      networkStatus = Msg_Connected;
      sprintf(topic, "bongo/%s/hub", macStr);
      mqtt.subscribe(topic);  
      mqtt.publish(topic, "{\"state\":\"connected\"}", true);
      modem_state=MQ_CONNECTED;
    }
  }
}  

void mqttPoll()
{
  bool gprsConnected = modem.isGprsConnected();
  if (!gprsConnected)
    modem_state=MODEM_CONNECTED;
  bool networkConnected = modem.isGprsConnected();
  if (!networkConnected)
    modem_state=MODEM_CONNECTED;
  mqtt.loop();
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
      mqttConnect();
      break;      
    case MQ_CONNECTED:
      mqttPoll();
      break;      
  }
}

void SendMQTTBinary(uint8_t *report, int packetSize)
{
  char topic[32];
  sprintf(topic, "bongo/%s/sensor", macStr);
  
  if (modem_state != MQ_CONNECTED)
    return;

  if (!mqtt.publish(topic, report, packetSize))
  {
    Serial.printf("MQTT Fail\n");
  }

  return;
}

#endif

