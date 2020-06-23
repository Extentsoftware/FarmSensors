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
#define MQTT_MAX_PACKET_SIZE 512


const char * Msg_Quality = "Quality";
const char * Msg_Network = "Network";
const char * Msg_GPRS    = "GPRS";
const char * Msg_MQTT    = "MQTT";

const char * Msg_Failed     = "Failed";
const char * Msg_Connected  = "Connected";
const char * Msg_Connecting = "Connecting";
const char * Msg_FailedToConnect = "Failed Connection";


char gpsjson[32];

#define APP_VERSION 1

#include <ESPAsyncWebServer.h>
#include <vesoil.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>  
#include <LoRa.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <TBeamPower.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <geohash.h>
#include <SoftwareSerial.h>
#include "vesoil_hub.h"

#ifdef HAS_OLED
#include <SSD1306.h>
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif

SoftwareSerial SerialAT(AT_RX, AT_TX, false);

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);
PubSubClient mqtt(client);
GeoHash hasher(8);

SensorReport* store;
int currentStoreWriter=0;
int currentStoreReader=0;
AsyncWebServer server(80);
Preferences preferences;
float snr = 0;
float rssi = 0;
long pfe=0;

int badpacket=0;
bool wifiMode=false;
int buttonState = HIGH;       // the current reading from the input pin
int lastButtonState = HIGH;   // the previous reading from the input pin
// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

struct HubConfig config;

TBeamPower power(PWRSDA,PWRSCL,BUSPWR, BATTERY_PIN);

esp_timer_handle_t oneshot_timer;

void setup() {
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


#ifdef HASPSRAM
  Serial.println("memory check");
  // GPS comms settings  
  MemoryCheck();  
#endif

#ifdef TTGO_TBEAM2
  float vBat = power.get_battery_voltage();
  Serial.printf("Battery voltage %f\n", vBat);
#endif

  doModemStart();
  while (!doNetworkConnect());
  doSetupMQTT();

  Serial.println("start lora");
  startLoRa();


  Serial.printf("End of setup - sensor packet size is %u\n", sizeof(SensorReport));

}

void InitOLED() {
  pinMode(OLED_RST,OUTPUT);
  digitalWrite(OLED_RST, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high、
  delay(50); 

  display.init();
  display.flipScreenVertically();  
}

void SetSimpleMsg(String msg, int line=0, bool clear=true, const uint8_t font[]=ArialMT_Plain_16)
{
#ifdef HAS_OLED
  if (clear) display.clear();
  display.setFont(font);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, line * 16, msg);
  display.display();
#endif
  SerialMon.println(msg);
}

void SetTwoLineMsg(const char * msg0, const char * msg1)
{
    SetSimpleMsg(msg0);
    SetSimpleMsg(msg1,1, false);
}

void SetThreeLineMsg(String msg0, String msg1, String msg2)
{
    SetSimpleMsg(msg0);
    SetSimpleMsg(msg1,1, false);
    SetSimpleMsg(msg2,2, false);
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
  LoRa.setSyncWord(config.syncword);    
  LoRa.setSignalBandwidth(config.bandwidth);
  LoRa.setSpreadingFactor(config.spreadFactor);
  LoRa.setCodingRate4(config.codingRate);
  if (config.enableCRC)
      LoRa.enableCrc();
    else 
      LoRa.disableCrc();

  LoRa.receive();
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
  
  
  if (btndown==0)
    startup_mode = NORMAL;   // sensor mode
  else if (btndown<2000)
    startup_mode = NORMAL;   // NORMAL
  else 
  {
    startup_mode = RESET;   // reset config
  }

  Serial.printf("button held for %d mode is %d\n", btndown, startup_mode );

  return startup_mode;
}

bool detectDebouncedBtnPush() {
  int reading = digitalRead(BTN1);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
    lastButtonState = reading;
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == LOW) {
        return true;
      }
    }
  }
  return false;
}

void loop() {
  if (detectDebouncedBtnPush())
    toggleWifi();

  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);
  delay(100);
}

void SystemCheck() {
  ESP_LOGI("TAG", "Rev %u Freq %d", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  ESP_LOGI("TAG", "PSRAM avail %u free %u", ESP.getPsramSize(), ESP.getFreePsram());
  ESP_LOGI("TAG", "FLASH size  %u spd  %u", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  ESP_LOGI("TAG", "HEAP  size  %u free  %u", ESP.getHeapSize(), ESP.getFreeHeap());
}

void readLoraData(int packetSize) {  
  if (packetSize>0) { 
    snr = LoRa.packetSnr();
    rssi = LoRa.packetRssi();
    pfe = LoRa.packetFrequencyError();
    Serial.printf("%d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 
    processLoraBlock(packetSize);  
    delay(100);
  }
}

void processLoraBlock(int packetSize) {
  SensorReport report;

  if (packetSize == sizeof(SensorReport))
  {
      unsigned char *ptr = (unsigned char *)&report;
      for (int i = 0; i < packetSize; i++, ptr++)
         *ptr = (unsigned char)LoRa.read(); 

      char *stime = asctime(gmtime(&report.gps.time));
      stime[24]='\0';

      Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f rssi=%f snr=%f pfe=%ld\n", 
            stime, report.gps.lat,report.gps.lng,
            report.gps.alt ,report.gps.sats ,report.gps.hdop,
            report.gndTemp.value,
            report.airTempHumidity.airtemp.value,
            report.airTempHumidity.airhum.value,
            report.moist1.value,
            report.moist2.value,
            report.volts.value, rssi, snr, pfe );
        
        SendMQTT(&report);
#ifdef HASPSRAM        
        AddToStore(report);
#endif
  }
  else
  {
      badpacket++;
      char * tmp= (char *)calloc(1,101);
      char *ptr = tmp;
      
      for (int i = 0; i < packetSize; i++, ptr++)
      {
         unsigned char ch = (unsigned char)LoRa.read(); 
          if (i<100)
            *ptr = ch;
      }

      Serial.printf("Invalid packet size, got %d instead of %d\n",packetSize, sizeof(SensorReport));
      Serial.printf("%s\n", tmp);
      free(tmp);
  }
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

void setupWifi() {
    WiFi.softAP(config.ssid);
    String address = WiFi.softAPIP().toString();

    SetThreeLineMsg("WiFi mode", "IP Address", address);
    
    wifiMode = true;

    setupWifiTimer();

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, from Vestrong");
    });
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
    server.on("/query", HTTP_GET, [] (AsyncWebServerRequest *request) {

    float vBat = power.get_battery_voltage();

    String reply = "{ \"snr\": " + String(snr, DEC) 
        + ", \"version\": " + String( APP_VERSION, DEC) 
        + ", \"battery\": " + String( vBat, DEC) 
        + ", \"rssi\": " + String( rssi, DEC) 
        + ", \"badpacket\": " + String(badpacket, DEC)
        + ", \"writer\": " + String(currentStoreWriter) 
        + ", \"reader\": " + String(currentStoreReader) 
//          + ", \"buffer\": " + String(STORESIZE, DEC)           
        + " }\n";
      request->send(200, "text/plain",  reply);
    });

    server.onNotFound(notFound);

    server.begin();
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
  power.led_onoff(true);
  power.led_onoff(false);
}

void doModemStart()
{
  SetSimpleMsg("Starting Modem");
  delay(1000);

  TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,9600);
  // delay(2000);  
  modem.restart();  
  String modemInfo = modem.getModemInfo();
  delay(1000);  
  SetSimpleMsg("Started Modem");
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
      RegStatus s = modem.getRegistrationStatus();    
      int quality = modem.getSignalQuality();
      char buf1[32];
      char buf2[32];
      sprintf(buf1, "%s %d",Msg_Quality, quality);
      sprintf(buf2, "%s %d",Msg_Network, s);
      SetThreeLineMsg("Wait for network", buf1, buf2);
    }
    return false;
}

bool doNetworkConnect()
{

  boolean status = waitForNetwork();

  SetTwoLineMsg(Msg_Network,(char*)(status ? Msg_Connected : Msg_Failed));
  delay(2000);  

  if (status)
  {
    SetTwoLineMsg(Msg_GPRS, Msg_Connecting);
    status = modem.gprsConnect(config.apn, config.gprsUser, config.gprsPass);
    SetTwoLineMsg(Msg_GPRS,(char*)(status ? Msg_Connected : Msg_Failed));
    delay(1000);  
  }
  else
  {
    // modem.restart();
  }
  return status;
}

void doSetupMQTT()
{
  // MQTT Broker setup
  mqtt.setServer(config.broker, 1883);
  mqtt.setCallback(mqttCallback);
}

void DisplayPacket(SensorReport *report) {
  char buf[64];
  sprintf(buf, "%02x%02x%02x%02x%02x%02x", report->id.id[0], report->id.id[1], report->id.id[2], report->id.id[3], report->id.id[4], report->id.id[5]);
  char *stime = asctime(gmtime(&report->gps.time));
  SetSimpleMsg(stime, 0, true, ArialMT_Plain_10);
  SetSimpleMsg(buf, 1, false, ArialMT_Plain_10);
  sprintf(buf,"Dist: %d V: %f", (int)(report->distance.value), report->volts.value);
  SetSimpleMsg(buf, 2, false, ArialMT_Plain_10);
  sprintf(buf,"M1: %d M2: %d", report->moist1.value, report->moist2.value);
  SetSimpleMsg(buf, 3, false, ArialMT_Plain_10);
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

void SendDistanceJsonReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  if (ptr->capability & Distance)
  {
    sprintf(payload, "{%s\"dist\":%f}\n", GetGeohash(ptr), ptr->distance.value );
    mqtt.publish(topic, payload);
  }
}

void SendMoist1JsonReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  if (ptr->capability & Moist1)
  {
    sprintf(payload, "{%s\"m1\":%d}\n", GetGeohash(ptr), ptr->moist1.value );
    mqtt.publish(topic, payload);
  }
}

void SendMoist2JsonReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  if (ptr->capability & Moist2)
  {
    sprintf(payload, "{%s\"m2\":%d}\n", GetGeohash(ptr), ptr->moist2.value );
    mqtt.publish(topic, payload);
  }
}

void SendAirHumJsonReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  if (ptr->capability & AirTempHum)
  {
    sprintf(payload, "{%s\"at\":%f,\"ah\":%f}\n", GetGeohash(ptr), ptr->airTempHumidity.airtemp.value, ptr->airTempHumidity.airhum.value );
    mqtt.publish(topic, payload);
  }
}

void SendGndTmpJsonReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  if (ptr->capability & GndTemp)
  {
    sprintf(payload, "{%s\"gt\":%f}\n", GetGeohash(ptr), ptr->gndTemp.value );
    mqtt.publish(topic, payload);
  }
}

void SendSysJsonReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  sprintf(payload, "{%s\"volts\":%f,\"version\":%d}\n", GetGeohash(ptr), ptr->volts.value, ptr->version );
  mqtt.publish(topic, payload);
}

void SendGpsReport(SensorReport *ptr, char * topic)
{
  char payload[MQTT_MAX_PACKET_SIZE];
  if (ptr->capability & GPS)
  {
    sprintf(payload, "{%s\"lat\":%f,\"lng\":%f,\"alt\":%f,\"sats\":%d,\"hdop\":%d}\n", GetGeohash(ptr), 
          ptr->gps.lat, ptr->gps.lng ,ptr->gps.alt ,ptr->gps.sats ,ptr->gps.hdop );
    mqtt.publish(topic, payload);
  }
}

void SendMQTT(SensorReport *report) {
  uint8_t array[6];
  esp_efuse_mac_get_default(array);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
  bool gprsConnected = modem.isGprsConnected();

  char *stime = asctime(gmtime(&report->gps.time));

  if (!gprsConnected)
    if (!doNetworkConnect())
      return;

  SetThreeLineMsg(Msg_MQTT, Msg_Connecting, stime);

  boolean mqttConnected = mqtt.connect(macStr);
  if (!mqttConnected)
  {
    SetThreeLineMsg(Msg_MQTT, Msg_FailedToConnect, stime);
    return;
  }

  DisplayPacket(report);

  char topic[32];
  sprintf(topic, "bongo/%02x%02x%02x%02x%02x%02x/sensor", report->id.id[0], report->id.id[1], report->id.id[2], report->id.id[3], report->id.id[4], report->id.id[5]);

  SendSysJsonReport(report, topic);
  SendDistanceJsonReport(report, topic);
  SendMoist1JsonReport(report, topic);
  SendMoist2JsonReport(report, topic);
  SendAirHumJsonReport(report, topic);
  SendGndTmpJsonReport(report, topic);
  SendGpsReport(report, topic);

  delay(1000);

  mqtt.disconnect();
}

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
