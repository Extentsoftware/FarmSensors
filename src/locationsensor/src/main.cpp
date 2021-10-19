// pio run --target upload
// pio device monitor -p COM12 -b 115200
#include <Arduino.h>
#include <SPI.h>
#include "LoRa.h"
#include <TBeamPower.h>
#include <TinyGPS++.h>
#include <CayenneLPP.h>
#include <main.h>
#include "vesoil.h"

TBeamPower power( PWRSDA, PWRSCL, BATTERY_PIN, BUSPWR);

TinyGPSPlus gps;                            
struct SensorConfig config;

void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton Location Sensor");
}

bool waitForGps() {
  for (int i=0; i<config.gps_timeout; i++)
  {
    
    Serial.printf("waiting for GPS try: %d  Age:%u  valid: %d   %d\n", i, gps.location.age(), gps.location.isValid(), gps.time.isValid());

    // check whethe we have  gps sig
    if (gps.location.lat()!=0 && gps.location.isValid() )
    {
        return true;
    }

    power.flashlight(1);    

    smartDelay(1000);
  }
  return false;
}


void SendPacket() 
{
    if (!waitForGps())
      return;

    uint8_t chipid[]={0,0,0,0,0,0,0,0};
    esp_efuse_mac_get_default(&chipid[0]);
    uint32_t chipid_l = chipid[0] + (chipid[1]<<8) + (chipid[2]<<16) + (chipid[3]<<24);
    uint32_t chipid_h = chipid[4] + (chipid[5]<<8) + (chipid[6]<<16) + (chipid[7]<<24);

    float volts = power.get_battery_voltage();

    CayenneLPP lpp(64);
    lpp.reset();
    lpp.addPresence(CH_ID_LO, chipid_l);
    lpp.addPresence(CH_ID_HI, chipid_h); 
    lpp.addGPS(CH_GPS, (float)gps.location.lat(), (float)gps.location.lng(), gps.altitude.feet());
    lpp.addVoltage(CH_VoltsS, volts);

    Serial.printf( " Id " );
    Serial.print( (chipid_h << 16) + chipid_l);
    Serial.printf( " Vs " );
    Serial.print( volts);
    Serial.printf( " lat ");
    Serial.print( (float)gps.location.lat() );
    Serial.printf( " lng ");
    Serial.print( (float)gps.location.lng() );
    Serial.printf( " alt ");
    Serial.print( gps.altitude.feet() );
    Serial.printf( " Tx Size=%d .. \n", lpp.getSize());

    power.led_onoff(true);
    LoRa.beginPacket();
    LoRa.write( (const uint8_t *)lpp.getBuffer(), lpp.getSize());
    LoRa.endPacket();
    power.led_onoff(false);
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

void loop() {  

  SendPacket();

  delay(10000);
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

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do
  {
    while (Serial1.available())
    {
      char c = Serial1.read();
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

GPSLOCK getGpsLock() {
  for (int i=0; i<config.gps_timeout; i++)
  {
    
    Serial.printf("waiting for GPS try: %d  Age:%u  valid: %d   %d\n", i, gps.location.age(), gps.location.isValid(), gps.time.isValid());

    // check whethe we have  gps sig
    if (gps.location.lat()!=0 && gps.location.isValid() )
    {
      // in the report window?
      if (gps.time.hour() >=config.fromHour && gps.time.hour() < config.toHour)
        return LOCK_OK;          
      else
        return LOCK_WINDOW;
    }

    power.flashlight(1);    

    smartDelay(1000);
  }
  return LOCK_FAIL;
}
