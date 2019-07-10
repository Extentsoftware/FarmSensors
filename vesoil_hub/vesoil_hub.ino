// https://github.com/me-no-dev/ESPAsyncWebServer#basic-response-with-http-code
// https://github.com/cyberman54/ESP32-Paxcounter/blob/82fdfb9ca129f71973a1f912a04aa8c7c5232a87/src/main.cpp
// https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/system/log.html

// environment variables
// LOCALAPPDATA = C:\Users\username\AppData\Local

static const char * TAG = "Hub";
#define APP_VERSION 1
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>  
#include <HardwareSerial.h>
#include <LoRa.h>
#include <AsyncTCP.h>
#include <Preferences.h>

#ifndef ESP32
#define ESP32
#endif

#ifndef ARDUINO
#define ARDUINO 100
#endif

#include <ESPAsyncWebServer.h>

struct SensorReport
{
    time_t time;
    char version;
    float volts;
    double lat;
    double lng; 
    double alt;
    char sats;
    char hdop;
    float airtemp;
    float airhum;
    float gndtemp;
    int moist1;
    int moist2;
};

struct SenserConfig
{
  char  ssid[16] = "VESTRONG_H";
  long  frequency = 868E6;      // LoRa transmit frequency
  long  bandwidth = 62.5E3;     // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  int   speadFactor = 12;       // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = 5;         // extra info for CRC
  bool  enableCRC = true;       //
} config;

void SystemCheck();
void getBatteryVoltage();
void setupBatteryVoltage();
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void showBlock(int packetSize);
void readLoraData();
SensorReport *GetFromStore();
void AddToStore(SensorReport *report);
int GetNextRingBufferPos(int pointer);
void MemoryCheck();

#define BLUELED 14   // GPIO14
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6
#define PSRAM   16    // 8M byte - https://www.electrodragon.com/product/2pcs-ipus-ips6404-iot-ram/
                      // https://drive.google.com/file/d/1-5NtY1bz0l9eYN9U0U4dP3uASwnMmYGP/view        

#define BATTERY_PIN 35    // battery level measurement pin, here is the voltage divider connected
#define BTN1        39    // GPIO39 On board button
#define STORESIZE   30000

float vBat; // battery voltage
SensorReport* store;
int currentStoreWriter=0;
int currentStoreReader=0;
AsyncWebServer server(80);
float snr = 0;
float rssi = 0;
long pfe=0;

int badpacket=0;
bool wifiMode=false;
Preferences preferences;

void setup() {

  preferences.begin(TAG, false);
//  if (preferences.getBool("configinit"))
//    preferences.getBytes("config", &config, sizeof(SenserConfig));
//  else
//  {
    preferences.putBytes("config", &config, sizeof(SenserConfig));
    preferences.putBool("configinit", true);
//  }


  setupBatteryVoltage();

  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa Hub");


  esp_log_level_set("*", ESP_LOG_VERBOSE);

  // GPS comms settings  
  SPI.begin(SCK,MISO,MOSI,SS);
  
  MemoryCheck();  
  SystemCheck();

  // turn on (for future: if GPIO is off)
  if (digitalRead(BTN1)==0)
  {
    //wifiMode=true;
    //setupWifi();
  }

  getBatteryVoltage();
  Serial.printf("Battery voltage %f\n", vBat);

  // turn on LoRa  
  Serial.printf("Starting Lora: freq:%u enableCRC:%d coderate:%d spread:%d bandwidth:%u\n", config.frequency, config.enableCRC, config.codingRate, config.speadFactor, config.bandwidth);
  LoRa.setPins(SS,RST,DI0);
  LoRa.setSignalBandwidth(config.bandwidth);
  if (config.enableCRC)
      LoRa.enableCrc();
   else 
      LoRa.disableCrc();
  LoRa.setCodingRate4(config.codingRate);
  LoRa.setSpreadingFactor(config.speadFactor);
    
  int result = LoRa.begin(config.frequency);  
  if (!result) {
    Serial.printf("Starting LoRa failed: err %d", result);
  }  


  LoRa.receive();

  digitalWrite(BLUELED, LOW);   // turn the LED off
}


void loop() {
  readLoraData();
}

void MemoryCheck() {
  size_t size = STORESIZE * sizeof(SensorReport);
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

void AddToStore(SensorReport *report) {
  memcpy( (void *)(&store[currentStoreWriter]), report, sizeof(SensorReport) );
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

void SystemCheck() {
  ESP_LOGI("TAG", "Rev %u Freq %d", ESP.getChipRevision(), ESP.getCpuFreqMHz());
  ESP_LOGI("TAG", "PSRAM avail %u free %u", ESP.getPsramSize(), ESP.getFreePsram());
  ESP_LOGI("TAG", "FLASH size  %u spd  %u", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  ESP_LOGI("TAG", "HEAP  size  %u free  %u", ESP.getHeapSize(), ESP.getFreeHeap());
}

void readLoraData() {
  int packetSize = LoRa.parsePacket();
  if (packetSize>0) { 
    Serial.printf("%d.",packetSize); 
    snr = LoRa.packetSnr();
    rssi = LoRa.packetRssi();
    pfe = LoRa.packetFrequencyError();

    getBatteryVoltage();
    digitalWrite(BLUELED, HIGH);   // turn the LED on (HIGH is the voltage level)
    showBlock(packetSize);  
    digitalWrite(BLUELED, LOW);   // turn the LED off
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

      Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f rssi=%f snr=%f pfe=%u\n", 
            stime, report.lat,report.lng ,report.alt ,report.sats ,report.hdop 
            ,report.gndtemp,report.airtemp,report.airhum ,report.moist1 ,report.moist2
            , report.volts, rssi, snr, pfe );

        AddToStore(&report);
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

void setupWifi() {
    WiFi.softAP(config.ssid);

    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Hello, from Vestrong");
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        
        String reply = "{ \"app\": \"hub\", \"samples\": [";
        SensorReport *ptr=NULL;
        ptr = GetFromStore();
        do {          
          if (ptr!=NULL)
          {
            char *stime = asctime(gmtime(&(ptr->time)));
            stime[24]='\0';
            char *msg = (char *)malloc(512);
            sprintf(msg, "{ \"date\": \"%s\", \"lat\": %f, \"lng\": %f, \"alt\":%f, \"sats\":%d, \"hdop\":%d, \"gt\":%f, \"at\":%f, \"ah\":%f, \"m1\":%d, \"m2\":%d, \"volts\":%f }\n", 
                  stime, 
                  ptr->lat,ptr->lng ,ptr->alt ,ptr->sats ,ptr->hdop,
                  ptr->gndtemp,ptr->airtemp,ptr->airhum,
                  ptr->moist1 ,ptr->moist2,
                  ptr->volts
                   );
            reply += msg;
            free(msg);
          }

          // get next packet
          ptr = GetFromStore();
          
          if (ptr!=NULL)
            reply += ",";

        } while ( ptr!=NULL );

        reply += " ] }\n";

        request->send(200, "text/plain",  reply);
    });

    server.on("/query", HTTP_GET, [] (AsyncWebServerRequest *request) {
      getBatteryVoltage();
      String reply = "{ \"snr\": " + String(snr, DEC) 
          + ", \"version\": " + String( APP_VERSION, DEC) 
          + ", \"battery\": " + String( vBat, DEC) 
          + ", \"rssi\": " + String( rssi, DEC) 
          + ", \"badpacket\": " + String(badpacket, DEC)
          + ", \"writer\": " + String(currentStoreWriter) 
          + ", \"reader\": " + String(currentStoreReader) 
          + ", \"buffer\": " + String(STORESIZE)           
          + " }\n";
      request->send(200, "text/plain",  reply);
    });

    server.onNotFound(notFound);

    server.begin();
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}


void setupBatteryVoltage()
{
   // set battery measurement pin
  adcAttachPin(BATTERY_PIN);
  adcStart(BATTERY_PIN);
  analogReadResolution(10); // Default of 12 is not very linear. Recommended to use 10 or 11 depending on needed resolution.
 
}


void getBatteryVoltage() {
  // we've set 10-bit ADC resolution 2^10=1024 and voltage divider makes it half of maximum readable value (which is 3.3V)
  vBat = analogRead(BATTERY_PIN) * 2.0 * (3.3 / 1024.0);
}

