#ifndef __VESOIL_SENSOR__
#define __VESOIL_SENSOR__



#define FREQUENCY 868E6
#define TXPOWER     20   // max power
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
#define BAND     868E6P

struct ReceiverConfig
{
  char  ssid[16] = "VESTRONG_S";
  int   gps_timout = 60;        // wait n seconds to get GPS fix
  int   failedGPSsleep = 60;    // sleep this long if failed to get GPS
  int   reportEvery = 60 * 60;  // get sample every n seconds
  int   fromHour = 6;           // between these hours
  int   toHour = 20;            // between these hours
  long  frequency = FREQUENCY;  // LoRa transmit frequency
  int   txpower = TXPOWER;      // LoRa transmit power
  float txvolts = TXVOLTS;      // power supply must be delivering this voltage in order to xmit.
  long  bandwidth = 62.5E3;     // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  int   speadFactor = 12;       // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = 5;         // extra info for CRC
  bool  enableCRC = true;       //
} config;


void print_wakeup_reason();
void  turnOffRTC();
void turnOffWifi();
void setupSerial();
void setupLoRa() ;
void smartDelay(unsigned long ms);
void turnOffWifi();
void turnOffBluetooth();
void deepSleep(uint64_t timetosleep);
void setupTempSensors();
float readGroundTemp();
float readAirTemp();
float readAirHum();
void GPSwakeup();
void GPSReset();
void setupGPS();
float getBatteryVoltage();
void sendSampleLora(SensorReport *report);
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void getSample(SensorReport *report);

#endif