// Pin map http://tinymicros.com/wiki/TTGO_T-Beam
// https://github.com/Xinyuan-LilyGO/TTGO-T-Beam
//
// to upload new html files use this command:
// pio device list
// pio run --target upload
// pio run --target uploadfs
// pio device monitor -p COM14 -b 115200

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
#include <TBeamPower.h>
#include "vesoil_hub.h"

SensorReport* store;
int currentStoreWriter=0;
int currentStoreReader=0;
AsyncWebServer server(80);
Preferences preferences;
TBeamPower power( BATTERY_PIN,PWRSDA,PWRSCL, BUSPWR);

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

void setup() {
  pinMode(BTN1,INPUT);        // Button 1

  power.begin();

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

  Serial.printf("Battery voltage %f\n", power.get_battery_voltage());

  Serial.println("start lora");
  startLoRa();

  Serial.printf("End of setup - sensor packet size is %u\n", sizeof(SensorReport));
}

void startLoRa() {
  power.power_LoRa(true);
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

void toggleWifi() {
  wifiMode = !wifiMode;
  if (wifiMode)
    setupWifi();
  else
    exitWifi();
}

void exitWifi() {
  Serial.printf("Exit WiFi mode\n");
  WiFi.softAPdisconnect( true );
  server.reset();
}

void setupWifi() {
    WiFi.softAP(config.ssid);
    Serial.printf("Entering WiFi mode\n");
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
      
      float vBat = power.get_battery_voltage();

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
