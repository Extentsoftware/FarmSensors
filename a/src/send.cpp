#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>  
#include <HardwareSerial.h>
#include <LoRa.h>         // https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md

#define FREQUENCY 868E6
#define BAND    12.5E3   
#define TXPOWER     14   // max power
#define TXVOLTS    2.7
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define ONE_WIRE_BUS 2
#define BATTERY_PIN 35   // battery level measurement pin, here is the voltage divider connected
#define MOIST1      13   // Analogue soil sensor 1
#define MOIST2      25   // Analogue soil sensor 2
#define DHTPIN      22   // DHT11
#define BLUELED     14   // GPIO14
#define BTN1        39   // GPIO39 On board button
#define BUSPWR       0   // GPIO00
#define SCK          5   // GPIO5  -- SX1278's SCK
#define MISO        19   // GPIO19 -- SX1278's MISnO
#define MOSI        27   // GPIO27 -- SX1278's MOSI
#define SS          18   // GPIO18 -- SX1278's CS
#define RST         14   // GPIO14 -- SX1278's RESET
#define DI0         26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define ERR_LOWPOWER  0x15  // 00010101
#define INFO_WIFI     0x33  // 00110011
#define INFO_SENSOR   0xAA  // 10101010

struct SensorConfig
{
  char  ssid[16] = "VESTRONG_S";
  char  password[16] = "";
  int   gps_timeout = 60;       // wait n seconds to get GPS fix
  int   failedGPSsleep = 60;    // sleep this long if failed to get GPS
  int   reportEvery = 1 * 60;   // get sample every n seconds
  int   fromHour = 6;           // between these hours
  int   toHour = 22;            // between these hours
  long  frequency = FREQUENCY;  // LoRa transmit frequency
  int   txpower = TXPOWER;      // LoRa transmit power
  long  preamble = 8;
  int   syncword = 0xa5a5;
  float txvolts = TXVOLTS;      // power supply must be delivering this voltage in order to xmit.
  int   lowvoltsleep = 600;     // sleep this long if low on volts
  long  bandwidth = BAND;       // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  int   spreadFactor = 6;       // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = 5;         // extra info for CRC
  bool  enableCRC = true;       //
} config;

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

void getSampleAndSend();
void setupSerial();
void setupLoRa() ;
void sendSampleLora(SensorReport *report);
void getSample(SensorReport *report);

void setupLoRa() {
  
  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);

  //LoRa.setSpreadingFactor(config.spreadFactor);
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

  LoRa.setTxPower(TXPOWER);
  LoRa.idle();
  
  if (!result) {
    Serial.printf("Starting LoRa failed: err %d", result);
  }  
}


void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton LoRa Sensor");
}

void getSample(SensorReport *report) {
  report->volts=10;
  report->time = 0;
  report->lat = 1;
  report->lng = 2;
  report->alt = 3;
  report->sats = 4;
  report->hdop = 5;
  report->gndtemp = 6;
  report->airtemp = 7;
  report->airhum = 8;  
  report->moist1 = 9;
  report->moist2 = 10;
}

void getSampleAndSend() 
{
  // get GPS and then gather/send a sample if within the time window
  // best not to send at night as we drain the battery
  SensorReport report;

  digitalWrite(BUSPWR, HIGH);   // turn on power to the sensor bus
  getSample(&report);
  digitalWrite(BUSPWR, LOW);   // turn off power to the sensor bus

  char *stime = asctime(gmtime(&report.time));
  stime[24]='\0';

  Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f\n",
  stime, report.lat, report.lng ,report.alt , +report.sats , +report.hdop ,report.gndtemp,report.airtemp,report.airhum ,report.moist1 ,report.moist2, report.volts );
  // send packet
  digitalWrite(BLUELED, HIGH);   // turn the LED on (HIGH is the voltage level)
  LoRa.beginPacket();
  Serial.println("LoRa begin");
  LoRa.write( (const uint8_t *)&report, sizeof(SensorReport));
  Serial.println("LoRa Write");
  LoRa.endPacket();
  Serial.println("LoRa End");
  digitalWrite(BLUELED, LOW);   // turn the LED off
}

void setup() {

  pinMode(BLUELED, OUTPUT);   // onboard Blue LED
  pinMode(BTN1,INPUT);        // Button 1
  pinMode(BUSPWR,OUTPUT);     // power enable for the sensors
  
  digitalWrite(BLUELED, HIGH);   // turn the LED off - we're doing stuff

  setupSerial();  

  setupLoRa();
}

void loop() {
      getSampleAndSend();
      delay(2000);
}
