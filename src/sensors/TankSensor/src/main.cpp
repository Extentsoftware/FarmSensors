// pio run --target upload
// pio device monitor -p COM12 -b 115200
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <TBeamPower.h>
#include <TinyGPS++.h>
#include <CayenneLPP.h>
#include <main.h>
#include "vesoil.h"

TBeamPower power(PWRSDA, PWRSCL, BATTERY_PIN, BUSPWR, LED_BUILTIN);

TinyGPSPlus gps;                            
struct SensorConfig config;

void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton Ultrasound Sensor");
}

GPSLOCK GpsStatus() {

    // check whethe we have  gps sig
    if (gps.location.lat()!=0 && gps.location.isValid() )
    {
        power.flashlight(1);    
        return LOCK_OK;          
    }
    else
    {
        Serial.printf("waiting for GPS try: Age:%u  valid: %d   %d\n", gps.location.age(), gps.location.isValid(), gps.time.isValid());
        return LOCK_WINDOW;
    }
}

float GetDistanceMm() {
    // Define inputs and outputs
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  digitalWrite(TRIGPIN, LOW);   // Clear the TRIGPIN by setting it LOW:
  delayMicroseconds(2); 
  digitalWrite(TRIGPIN, HIGH);  // Trigger the sensor by setting the TRIGPIN high for 10 microseconds:
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  
  // Read the ECHOPIN. pulseIn() returns the duration (length of the pulse) in microseconds:
  long duration = pulseIn(ECHOPIN, HIGH);
  
  return duration / 5.882 ;
}

void SendPacket(float distanceMm) 
{
    uint8_t chipid[]={0,0,0,0,0,0,0,0};
    esp_efuse_mac_get_default(&chipid[0]);
    uint32_t chipid_l = chipid[0] + (chipid[1]<<8) + (chipid[2]<<16) + (chipid[3]<<24);
    uint32_t chipid_h = chipid[4] + (chipid[5]<<8) + (chipid[6]<<16) + (chipid[7]<<24);

    float volts = power.get_supply_voltage();
    

    CayenneLPP lpp(64);
    lpp.reset();
    lpp.addPresence(CH_ID_LO, chipid_l);
    lpp.addPresence(CH_ID_HI, chipid_h); 
    lpp.addDistance(CH_Distance, distanceMm);
    lpp.addVoltage(CH_VoltsS, volts);

    Serial.printf( " Id " );
    Serial.print( (chipid_h << 16) + chipid_l);
    Serial.printf( " Vs " );
    Serial.print( volts);
    Serial.printf( " distanceMm ");
    Serial.print( distanceMm );

    if (GpsStatus() == LOCK_OK)
    {
      Serial.printf( " lat ");
      Serial.print( (float)gps.location.lat() );
      Serial.printf( " lng ");
      Serial.print( (float)gps.location.lng() );
      Serial.printf( " alt ");
      Serial.print( gps.altitude.feet() );
      lpp.addGPS(CH_GPS, (float)gps.location.lat(), (float)gps.location.lng(), gps.altitude.feet());
    }

    Serial.printf( " Tx Size=%d .. ", lpp.getSize());

    power.led_onoff(true);
    LoRa.beginPacket();
    LoRa.write( (const uint8_t *)lpp.getBuffer(), lpp.getSize());
    LoRa.endPacket();
    power.led_onoff(false);
    
    Serial.printf( " done\n");
}

void setup() {
  // Begin Serial communication at a baudrate of 9600:
  setupSerial();

  power.begin();
  power.power_sensors(false);
  power.power_peripherals(true);

  startLoRa();
  startGPS();
}

bool waitForGps()
{
  for (int i = 0; i < config.gps_timeout; i++)
  {

    Serial.printf("waiting for GPS try: %d  Age:%u  valid: %d   %d\n", i, gps.location.age(), gps.location.isValid(), gps.time.isValid());

    // check whether we have a gps signal
    if (gps.location.lat() != 0 && gps.location.isValid())
    {
      power.led_onoff(true);
      return true;
    }

    power.flashlight(1);

    smartDelay(1000);
  }
  power.led_onoff(false);
  return false;
}

void loop() {
  waitForGps();
  float distanceMm =GetDistanceMm();
  Serial.printf( " %f \n", distanceMm);
  SendPacket(distanceMm);

  int delay = 60*4;
  Serial.printf( "deep sleeping for %d seconds\n", delay);
  power.deep_sleep(delay);
}

void stopLoRa()
{
  LoRa.sleep();
  LoRa.end();
  power.power_LoRa(false);
}

void startLoRa() {
  Serial.begin(115200);
  while (!Serial);

  power.power_LoRa(true);
  Serial.printf("\nStarting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu txpower:%d\n", config.frequency, config.enableCRC, config.codingRate, config.spreadFactor, config.bandwidth, config.txpower);

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

  if (config.syncword>0)
    LoRa.setSyncWord(config.syncword);    

  LoRa.setSignalBandwidth(config.bandwidth);

  LoRa.setTxPower(config.txpower);
  LoRa.idle();
  
  if (!result) {
    Serial.printf("Starting LoRa failed: err %d", result);
  }  
}

void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (Serial1.available())
    {
      char c = Serial1.read();
      //Serial.print(c);
      gps.encode(c);
    }
  } while (millis() - start < ms);
}


void startGPS() {
  power.power_GPS(true);
  Serial1.begin(GPSBAUD, SERIAL_8N1, GPSRX, GPSTX);
  Serial.println("Wake GPS");
  int data = -1;
  do {
    for(int i = 0; i < 20; i++){ //send random to trigger respose
        Serial1.write(0xFF);
      }
    data = Serial1.read();
  } while(data == -1);
  Serial.println("GPS is awake");
}

void stopGPS() {
  const byte CFG_RST[12] = {0xb5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x01,0x00, 0x0F, 0x66};//Controlled Software reset
  const byte RXM_PMREQ[16] = {0xb5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4d, 0x3b};   //power off until wakeup call
  Serial1.write(CFG_RST, sizeof(CFG_RST));
  delay(600); //give some time to restart //TODO wait for ack
  Serial1.write(RXM_PMREQ, sizeof(RXM_PMREQ));
  power.power_GPS(false);
}
