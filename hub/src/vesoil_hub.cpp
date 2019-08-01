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
#include "vesoil_hub.h"

float vBat; // battery voltage
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
struct HubConfig config;

void setup() {
  pinMode(BLUELED, OUTPUT);   // onboard Blue LED
  pinMode(BTN1,INPUT);        // Button 1

  digitalWrite(BLUELED, HIGH);   // turn the LED off - we're doing stuff

  setupBatteryVoltage();

  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa HUB");

  Serial.println("get startup");
  STARTUPMODE startup_mode = getStartupMode();
  
  Serial.println("get gonfig");
  getConfig(startup_mode);

  Serial.println("memory check");
  // GPS comms settings  
  MemoryCheck();  

  Serial.println("system check");
  SystemCheck();

  getBatteryVoltage();
  Serial.printf("Battery voltage %f\n", vBat);

  Serial.println("start lora");
  startLoRa();

  digitalWrite(BLUELED, LOW);   // turn the LED off

  if (startup_mode==WIFI)
  {
    flashlight(INFO_WIFI);
    Serial.printf("Entering WiFi mode\n");
    wifiMode=true;
    setupWifi();
  }
  else
    flashlight(INFO_SENSOR);
  
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
  while (digitalRead(BTN1)==0)
  {
    delay(interval);
    btndown += interval;
  }
  
  if (btndown==0)
    startup_mode = NORMAL;   // sensor mode
  else if (btndown<2000)
    startup_mode = WIFI;   // wifi
  else 
  {
    startup_mode = RESET;   // reset config
    flashlight(ERR_LOWPOWER);
  }

  Serial.printf("button held for %d mode is %d\n", btndown, startup_mode );

  return startup_mode;
}

void flashlight(char code) {
  digitalWrite(BLUELED, LOW);   // turn the LED off - we're doing stuff
  for (char i=0;i<8;i++)
  {
    digitalWrite(BLUELED, ((code & 1)==1));   // turn the LED off - we're doing stuff
    delay(150);
    code = code>>1;
    digitalWrite(BLUELED, LOW);   // turn the LED off - we're doing stuff
  }  
}

void loop() {
  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);
  delay(100);
}

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

void readLoraData(int packetSize) {  
  if (packetSize>0) { 
    snr = LoRa.packetSnr();
    rssi = LoRa.packetRssi();
    pfe = LoRa.packetFrequencyError();
    Serial.printf("%d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 
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

      Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f rssi=%f snr=%f pfe=%ld\n", 
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