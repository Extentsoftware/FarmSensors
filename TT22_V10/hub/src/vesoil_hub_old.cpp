#if false
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
#include <TBeamPower.h>
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


//SensorReport* store;

#ifdef HASPSRAM
int currentStoreWriter=0;
int currentStoreReader=0;
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

TBeamPower power(PWRSDA,PWRSCL,BUSPWR, BATTERY_PIN);

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

#ifdef HASPSRAM
  Serial.println("memory check");
  // GPS comms settings  
  MemoryCheck();  
#endif

#ifdef TTGO_TBEAM2
  float vBat = power.get_battery_voltage();
  Serial.printf("Battery voltage %f\n", vBat);
#endif

#ifdef HAS_GSM
  doModemStart();
  while (!doNetworkConnect());
  doSetupMQTT();
#endif

  setupWifi();
  
  startLoRa();

  networkStage = "Lora";
  networkStatus = "Ready";

  DisplayPage(currentPage);

  //Serial.printf("End of setup - sensor packet size is %u\n", sizeof(SensorReport));
  Serial.printf("End of setup\n");

}

void ShowNextPage()
{
    currentPage++;

    if (!haveReport && currentPage>2)
      currentPage=0;

    if (currentPage==9)
      currentPage=0;

    DisplayPage(currentPage);
}

void DisplayPage(int page)
{
    char sensor[32];
    float d;
    int percentFull;

    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    switch(currentPage)
    {
        // status
        case 0:
          display.drawString(0, 0, "System Status");
          display.drawString(0, 16, networkStage);
          display.drawString(32, 16, networkStatus);
          display.drawString(0, 32, incomingMessage);
          
#ifdef HAS_GSM
          display.drawString(0, 48, String("Gsm dB: ") + String(modem.getSignalQuality(), DEC));
#endif
          break;
        case 1:
          display.drawString(0, 0, "Wifi Status");
          if (wifiMode)
          {
            display.drawString(0, 16, "On");
            display.drawString(0, 32, address);
          }
          else
          {
            display.drawString(0, 16, "Off");
          }
          break;
        case 2:
          display.drawString(0, 0, "Signal");
          display.drawString(0, 16, String("rssi ") + String(rssi, 2));
          display.drawString(64, 16, String("snr  ") + String(snr, 2));

          display.drawString(0, 32, String("pfe  ") + String(pfe, DEC));

          display.drawString(64, 48, String("Ok:  ") + String(packetcount,DEC));
          break;
        case 3:
          display.drawString(0, 0, "Sensor Id");
          //sprintf(sensor, "%02x%02x%02x%02x%02x%02x/sensor", report.id.id[0], report.id.id[1], report.id.id[2], report.id.id[3], report.id.id[4], report.id.id[5]);          
          display.drawString(0, 16, sensor );
          //display.drawString(0, 32, String("Batt ") + String(report.volts.value, 2) );
          break;
        case 4:
          display.drawString(0, 0, "GPS #1");
          //display.drawString(0, 16, asctime(gmtime(&report.gps.time)));
          //display.drawString(0, 32, String("sats ") + String((int)report.gps.sats, DEC));
          //display.drawString(0, 48, String("hdop ") + String((int)report.gps.hdop,DEC));
          break;
        case 5:
          display.drawString(0, 0, "GPS #2");
          //display.drawString(0, 16, String("Lat ") + String(report.gps.lat, 6));
          //display.drawString(0, 32, String("Lng ") + String(report.gps.lng, 6));
          //display.drawString(0, 48, String("alt ") + String(report.gps.alt));
          break;
        case 6:
          display.drawString(0, 0, "Distance");
          //d = (report.distance.value < config.FullHeight)? config.FullHeight : report.distance.value;
          //d = (d > config.EmptyHeight)? config.EmptyHeight : d;
          //d = d - config.FullHeight;
          //percentFull = (int)(100 * (config.EmptyHeight - d) / (config.EmptyHeight - config.FullHeight));
          //percentFull = percentFull>100?100:percentFull;
          //percentFull = percentFull<0?0:percentFull;
          //display.drawString(0, 16, String(report.distance.value, 1) + String(" cm") );
          //display.drawProgressBar(0,32,120, 12, percentFull);
          break;
        case 7:
          display.drawString(0, 0, "Temp");
          //display.drawString(0, 16, String("Air Tmp. ") + String(report.airTempHumidity.airtemp.value, 1) );
          //display.drawString(0, 32, String("Air Hum. ") + String(report.airTempHumidity.airhum.value, 1) );
          //display.drawString(64, 16, String("Gnd ") + String(report.gndTemp.value, 1) );
          break;
        case 8:
          display.drawString(0, 0, "Moisture");
          //display.drawString(0, 16, String("M1. ") + String(report.moist1.value, DEC) );
          //display.drawString(0, 32, String("M2. ") + String(report.moist2.value, DEC) );
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
  mqtt.loop();
#endif

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

  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);
}

void SystemCheck() {
  ESP_LOGI("TAG", "Rev %u Freq %d", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  ESP_LOGI("TAG", "PSRAM avail %u free %u", ESP.getPsramSize(), ESP.getFreePsram());
  ESP_LOGI("TAG", "FLASH size  %u spd  %u", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  ESP_LOGI("TAG", "HEAP  size  %u free  %u", ESP.getHeapSize(), ESP.getFreeHeap());
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

void readLoraData(int packetSize) {  
  if (packetSize>0) { 
    snr = LoRa.packetSnr();
    rssi = LoRa.packetRssi();
    pfe = LoRa.packetFrequencyError();

    Serial.printf("Got %d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 

    haveReport = true;
    packetcount++; 
    CayenneLPP lpp(64);
    DynamicJsonDocument jsonBuffer(4096);
    JsonArray root = jsonBuffer.to<JsonArray>();
    uint8_t * buffer = (uint8_t *)malloc(packetSize);
    LoRa.readBytes(buffer, packetSize);
    lpp.decode(buffer, packetSize, root);
    serializeJsonPretty(root, Serial);

    #ifdef HAS_GSM
        SendMQTT(&report);
    #endif

    #ifdef HASPSRAM        
        AddToStore(report);
    #endif

      //String s = LoRa.readString();
      //strcpy( incomingMessage, s.c_str() );
      //Serial.print(s); 
      //Serial.printf("  %d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 
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
  if(var == "vbatt")
    return String(power.get_battery_voltage());
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

#ifdef HASPSRAM
    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        
        String reply = "{ \"app\": \"hub\", \"samples\": [";
        SensorReport *ptr=NULL;
        ptr = GetFromStore();
        do {      
          char payload[132];    
          if (ptr!=NULL)
          {
            GetJsonReport(*ptr, payload);
            reply += payload;
          }

          // get next packet
          ptr = GetFromStore();
          
          if (ptr!=NULL)
            reply += ",";

        } while ( ptr!=NULL );

        reply += " ] }\n";

        request->send(200, "text/plain",  reply);
    });
#endif

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
  networkStage = "Modem";
  networkStatus = "Starting";
  DisplayPage(currentPage);
  delay(1000);

  TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,9600);
  // delay(2000);  
  modem.restart();  
  String modemInfo = modem.getModemInfo();
  delay(1000);  
  networkStatus = "Started";
}

bool waitForNetwork(uint32_t timeout_ms = 120000L) 
{
    for (uint32_t start = millis(); millis() - start < timeout_ms;) 
    {
      if (modem.isNetworkConnected()) 
      { 
        return true; 
      }

      delay(1000);
      networkStage = "Network";
      SimStatus simStatus = modem.getSimStatus();
      RegStatus regStatus = modem.getRegistrationStatus();
      networkStatus = String("Sim: ") + String(simStatus, DEC) + String(" Reg: ") + String(regStatus, DEC);
      DisplayPage(currentPage);
    }
    return false;
}

bool doNetworkConnect()
{

  boolean status = waitForNetwork();

  networkStage = Msg_Network;
  networkStatus = (char*)(status ? Msg_Connected : Msg_Failed);
  DisplayPage(currentPage);

  delay(2000);  

  if (status)
  {
    networkStage = Msg_GPRS;
    networkStatus = Msg_Connecting;
    DisplayPage(currentPage);
    
    status = modem.gprsConnect(config.apn, config.gprsUser, config.gprsPass);    

    networkStatus = (char*)(status ? Msg_Connected : Msg_Failed);
    DisplayPage(currentPage);

    delay(1000);  
  }
  else
  {
    // modem.restart();
  }
  return status;
}

char *GetGeohash(SensorReport *ptr)
{
  gpsjson[0]='\0';
  if (ptr->capability & GPS)
  {
    const char *geohash = hasher.encode(ptr->gps.lat, ptr->gps.lng);
    sprintf(gpsjson, "\"geohash\":\"%s\",", geohash );
  }
  return gpsjson;
}

void SendCompleteReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];

  sprintf(payload, "{%s\"lat\":%f,\"lng\":%f,\"alt\":%f,\"sats\":%d,\"hdop\":%d,\"volts\":%f,\"version\":%d,\"gt\":%f,\"at\":%f,\"ah\":%f,\"m1\":%d,\"m2\":%d,\"dist\":%f,\"snr\":%f,\"rssi\":%f,\"pfe\":%ld}"
    , ptr->capability & GPS ? GetGeohash(ptr): ""
    , ptr->gps.lat, ptr->gps.lng ,ptr->gps.alt ,ptr->gps.sats ,ptr->gps.hdop
    , ptr->volts.value
    , ptr->version 
    , ptr->gndTemp.value
    , ptr->airTempHumidity.airtemp.value
    , ptr->airTempHumidity.airhum.value
    , ptr->moist1.value
    , ptr->moist2.value
    , ptr->distance.value
    , snr
    , rssi
    , pfe
    );
  if (!mqtt.publish(topic, payload, true))
  {
    Serial.printf("MQTT Fail %d\n", strlen(payload));
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

bool SendMQTTBinary(uint32 *report) {
  char topic[32];
  bool gprsConnected = modem.isGprsConnected();

  if (!gprsConnected)
    if (!doNetworkConnect())
      return false;

  if (!mqtt.connected())
  {
    networkStage = Msg_MQTT;
    networkStatus = Msg_Connecting;
    DisplayPage(currentPage);
    boolean mqttConnected = mqtt.connect(macStr);
    if (!mqttConnected)
    {
      networkStatus = Msg_FailedToConnect;
      return false;
    }
    else
    {
      networkStatus = Msg_Connected;
      sprintf(topic, "bongo/%s/hub", macStr);
      mqtt.subscribe(topic);  
      mqtt.publish(topic, "{\"state\":\"connected\"}", true);
    }
  }

  DisplayPage(currentPage);

  if (!mqtt.publish(topic, payload, true))
  {
    Serial.printf("MQTT Fail %d\n", strlen(payload));
  }


  sprintf(topic, "bongo/%02x%02x%02x%02x%02x%02x/sensor", report->id.id[0], report->id.id[1], report->id.id[2], report->id.id[3], report->id.id[4], report->id.id[5]);

  SendCompleteReport(report, topic);

  return true;
}


bool SendMQTT(SensorReport *report) {
  char topic[32];
  bool gprsConnected = modem.isGprsConnected();

  if (!gprsConnected)
    if (!doNetworkConnect())
      return false;

  if (!mqtt.connected())
  {
    networkStage = Msg_MQTT;
    networkStatus = Msg_Connecting;
    DisplayPage(currentPage);
    boolean mqttConnected = mqtt.connect(macStr);
    if (!mqttConnected)
    {
      networkStatus = Msg_FailedToConnect;
      return false;
    }
    else
    {
      networkStatus = Msg_Connected;
      sprintf(topic, "bongo/%s/hub", macStr);
      mqtt.subscribe(topic);  
      mqtt.publish(topic, "{\"state\":\"connected\"}", true);
    }
  }

  DisplayPage(currentPage);

  sprintf(topic, "bongo/%02x%02x%02x%02x%02x%02x/sensor", report->id.id[0], report->id.id[1], report->id.id[2], report->id.id[3], report->id.id[4], report->id.id[5]);

  SendCompleteReport(report, topic);

  return true;
}

#endif

#ifdef HASPSRAM

void MemoryCheck() {
  
  store = (SensorReport *) ps_calloc(STORESIZE,  sizeof(SensorReport));
  if (store != NULL)
  {
    ESP_LOGI("TAG","Memory allocated %u bytes for %d records @ %u",size, STORESIZE, store);
  }
  else
    ESP_LOGI("TAG","Could not allocate memory for %u bytes",size);
}

int GetNextRingBufferPos(int pointer) {
  pointer++;
  if (pointer>=STORESIZE)
    pointer=0;
  return pointer;
}

void AddToStore(SensorReport report) {
  memcpy( (void *)(&store[currentStoreWriter]), &report, sizeof(SensorReport) );
  currentStoreWriter = GetNextRingBufferPos(currentStoreWriter);
  // hit the reader?
  if (currentStoreWriter == currentStoreReader)
    currentStoreReader = GetNextRingBufferPos(currentStoreReader);
}

SensorReport *GetFromStore() {
  if (currentStoreReader==currentStoreWriter)
    return NULL;  // no data
  SensorReport *ptr = &store[currentStoreReader];
  currentStoreReader = GetNextRingBufferPos(currentStoreReader);
  return ptr;
}
#endif

#endif
