#include <SPI.h>
#include <Wire.h>  
#include <HardwareSerial.h>
#include <LoRa.h>
#include <Preferences.h>

struct SensorReport
{
    long time;
    char version;
    float volts;
    float lat;
    float lng; 
    float alt;
    char sats;
    char hdop;
    float airtemp;
    float airhum;
    float gndtemp;
    int moist1;
    int moist2;
};

#define ERR_LOWPOWER  0x15  // 00010101
#define INFO_WIFI     0x33  // 00110011
#define INFO_SENSOR   0xAA  // 10101010
#define FREQUENCY    868E6
#define BAND        12.5E3   

struct HubConfig
{
  char  ssid[16] = "VESTRONG_H";
  long  frequency = FREQUENCY;  // LoRa transmit frequency
  long  bandwidth = BAND;       // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  long  preamble = 8;
  int   syncword = 0xa5a5;
  int   speadFactor = 6;        // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = 5;         // extra info for CRC
  bool  enableCRC = true;       //
} config;

void onReceive(int packetSize);
void showBlock(int packetSize);
void readLoraData(int packetSize);
void startLoRa();

#define BLUELED 14   // GPIO14
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define PSRAM   16    // 8M byte - https://www.electrodragon.com/product/2pcs-ipus-ips6404-iot-ram/
                      // https://drive.google.com/file/d/1-5NtY1bz0l9eYN9U0U4dP3uASwnMmYGP/view        

#define BATTERY_PIN 35    // battery level measurement pin, here is the voltage divider connected
#define BTN1        39    // GPIO39 On board button
#define STORESIZE   30000


SensorReport* store;
Preferences preferences;
float snr = 0;
float rssi = 0;
long pfe=0;
int badpacket=0;

void setup() {
  pinMode(BLUELED, OUTPUT);   // onboard Blue LED
  pinMode(BTN1,INPUT);        // Button 1

  digitalWrite(BLUELED, HIGH);   // turn the LED off - we're doing stuff

  Serial.begin(115200);
  while (!Serial);
  Serial.println("VESTRONG LaPoulton LoRa Hub");

  startLoRa();

  digitalWrite(BLUELED, LOW);   // turn the LED off
}

void startLoRa() {

  Serial.printf("Starting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu\n", config.frequency, config.enableCRC, config.codingRate, config.speadFactor, config.bandwidth);

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);

  //LoRa.setSpreadingFactor(config.speadFactor);
  //LoRa.setCodingRate4(config.codingRate);

  int result = LoRa.begin(FREQUENCY);  
  if (!result) 
    Serial.printf("Starting LoRa failed: err %d\n", result);
  else
    Serial.println("Started LoRa OK");

#if true
    LoRa.setPreambleLength(config.preamble);
    LoRa.setSyncWord(config.syncword);    
    LoRa.setSignalBandwidth(config.bandwidth);
    if (config.enableCRC)
      LoRa.enableCrc();
    else 
      LoRa.disableCrc();
#else
  
  
#endif      

  LoRa.receive();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  readLoraData(packetSize);
}

void readLoraData(int packetSize) {  
  if (packetSize>0) { 
    snr = LoRa.packetSnr();
    rssi = LoRa.packetRssi();
    pfe = LoRa.packetFrequencyError();
    Serial.printf("%d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 
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

      Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f rssi=%f snr=%f pfe=%lu\n", 
            stime, report.lat,report.lng ,report.alt ,report.sats ,report.hdop 
            ,report.gndtemp,report.airtemp,report.airhum ,report.moist1 ,report.moist2
            , report.volts, rssi, snr, pfe );

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
