// https://github.com/me-no-dev/ESPAsyncWebServer#basic-response-with-http-code
// https://github.com/cyberman54/ESP32-Paxcounter/blob/82fdfb9ca129f71973a1f912a04aa8c7c5232a87/src/main.cpp
// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/log.html
// https://github.com/LilyGO/TTGO-T-Beam
// Pin map http://tinymicros.com/wiki/TTGO_T-Beam
// https://github.com/Xinyuan-LilyGO/TTGO-T-Beam


//UART	RX IO	TX IO	CTS	RTS
//UART0	GPIO3	GPIO1	N/A	N/A
//UART1	GPIO9	GPIO10	GPIO6	GPIO11
//UART2	GPIO16	GPIO17	GPIO8	GPIO7

// environment variables
// LOCALAPPDATA = C:\Users\username\AppData\Local

static const char * TAG = "Hub";

#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER   1024
#define SerialMon Serial
#define SerialAT Serial2
#define TINY_GSM_DEBUG SerialMon
#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 115200
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#define GSM_PIN ""
#define TINY_GSM_YIELD() { delay(2); }
#define MQTT_MAX_PACKET_SIZE 512

const char * Msg_WaitingForNetwork = "Waiting For Network";
const char * Msg_NetConnectFailed = "Failed to connect to Network";
const char * Msg_NetConnectOk = "Connected to Network";
const char * Msg_GPRSConnectFailed = "Failed to connect to GPRS";
const char * Msg_GPRSConnectOk = "Connected to GPRS";
const char * Msg_MQTTConnectFailed = "Failed to connect to MQTT";
const char * Msg_MQTTConnectOk = "Connect to MQTT";

// MQTT details
const char* broker = "86.21.199.245";
const char apn[]      = "wap.vodafone.co.uk"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
const char gprsUser[] = "wap"; // GPRS User
const char gprsPass[] = "wap"; // GPRS Password


#define APP_VERSION 1
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>  
#include <HardwareSerial.h>
#include <LoRa.h>
#include <AsyncTCP.h>
#include <Preferences.h>
#include <TBeamPower.h>
#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <geohash.h>

#include "vesoil_hub.h"

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
  doNetworkConnect();
  doSetupMQTT();

  Serial.println("start lora");
  startLoRa();


  Serial.printf("End of setup - sensor packet size is %u\n", sizeof(SensorReport));

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
    showBlock(packetSize);  
    delay(100);
  }
}

void showBlock(int packetSize) {
  SensorReport report;

  if (packetSize == sizeof(SensorReport))
  {
      unsigned char *ptr = (unsigned char *)&report;
      for (int i = 0; i < packetSize; i++, ptr++)
         *ptr = (unsigned char)LoRa.read(); 

      char *stime = asctime(gmtime(&report.time));
      stime[24]='\0';

      Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f rssi=%f snr=%f pfe=%ld\n", 
            stime, report.lat,report.lng ,report.alt ,report.sats ,report.hdop 
            ,report.gndtemp,report.airtemp,report.airhum ,report.moist1 ,report.moist2
            , report.volts, rssi, snr, pfe );
        
        SendMQTT(report);
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
    Serial.printf("Entering WiFi mode\n");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
    
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

void SetSimpleMsg(const char * msg)
{
#ifdef HAS_OLED
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, msg);
  display.display();
#endif
  SerialMon.println(msg);
}


// incoming message
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
  SetSimpleMsg("Modem Starting");
  delay(1000);
  TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,GSM_AUTOBAUD_MAX);
  delay(3000);  
  modem.restart();  
  String modemInfo = modem.getModemInfo();
  delay(1000);  
  SetSimpleMsg("Modem Started");
}

bool doNetworkConnect()
{
  SetSimpleMsg(Msg_WaitingForNetwork);
  boolean status =  modem.waitForNetwork();
  SetSimpleMsg((char*)(status ? Msg_NetConnectOk : Msg_NetConnectFailed));

  if (status)
  {
    status = modem.gprsConnect(apn, gprsUser, gprsPass);
    SetSimpleMsg((char*)(status ? Msg_GPRSConnectOk : Msg_GPRSConnectFailed));
  }
  return status;
}

void doSetupMQTT()
{
  // MQTT Broker setup
  mqtt.setServer(broker, 1883);
  mqtt.setCallback(mqttCallback);
}

void SendMQTT(SensorReport report) {
  uint8_t array[6];
  esp_efuse_mac_get_default(array);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x", array[0], array[1], array[2], array[3], array[4], array[5]);
  bool gprsConnected = modem.isGprsConnected();

  if (!gprsConnected)
    gprsConnected = doNetworkConnect();

  if (gprsConnected)
  {
    boolean mqttConnected = mqtt.connect(macStr);
    SetSimpleMsg((char*)(mqttConnected ? Msg_MQTTConnectOk : Msg_MQTTConnectFailed));

    if (mqttConnected)
    {
      char topic[32];
      sprintf(topic, "bongo/%02x%02x%02x%02x%02x%02x/sensor", report.id[0], report.id[1], report.id[2], report.id[3], report.id[4], report.id[5]);
      SerialMon.println( topic );

      char payload[MQTT_MAX_PACKET_SIZE];

      GetSys1JsonReport(report, payload);
      SerialMon.println( payload );      
      mqtt.publish(topic, payload);

      GetSys2JsonReport(report, payload);
      SerialMon.println( payload );      
      mqtt.publish(topic, payload);

      GetDistanceJsonReport(report, payload);
      SerialMon.println( payload );      
      mqtt.publish(topic, payload);

      GetMoistJsonReport(report, payload);
      SerialMon.println( payload );      
      mqtt.publish(topic, payload);

      delay(3000);

      SerialMon.println( "Sent" );
      mqtt.disconnect();
    }
  }
}

void GetDistanceJsonReport(SensorReport ptr, char * buf)
{
    const char *geohash = hasher.encode(ptr.lat, ptr.lng);
    sprintf(buf, "{\"geohash\":\"%s\",\"dist\":%f}\n", geohash, ptr.distance );
}

void GetMoistJsonReport(SensorReport ptr, char * buf)
{
    const char *geohash = hasher.encode(ptr.lat, ptr.lng);
    sprintf(buf, "{\"geohash\":\"%s\",\"gt\":%.2g,\"at\":%f,\"ah\":%f,\"m1\":%d,\"m2\":%d}\n", 
          geohash, ptr.gndtemp,ptr.airtemp,ptr.airhum, ptr.moist1 ,ptr.moist2 );
}

void GetSys1JsonReport(SensorReport ptr, char * buf)
{
    sprintf(buf, "{\"lat\":%f,\"lng\":%f,\"alt\":%f,\"sats\":%d,\"hdop\":%d}\n", 
          ptr.lat, ptr.lng ,ptr.alt ,ptr.sats ,ptr.hdop );
}

void GetSys2JsonReport(SensorReport ptr, char * buf)
{
    const char *geohash = hasher.encode(ptr.lat, ptr.lng);
    sprintf(buf, "{\"geohash\":\"%s\",\"volts\":%f}\n", 
          geohash, ptr.volts );
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
