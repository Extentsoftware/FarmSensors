#ifndef __VESOIL_SENSOR__
#define __VESOIL_SENSOR__

#define FREQUENCY 868E6
#define BAND      125E3   
#define SPREAD       12   
#define CODERATE      6
#define SYNCWORD 0xa5a5
#define PREAMBLE      8
#define TXPOWER      20   // max power
#define TXVOLTS     2.7
#define ONE_WIRE_BUS  2   // used for 
#define BATTERY_PIN  35   // battery level measurement pin, here is the voltage divider connected

#define SDA       21      // I2C
#define SCL       22      // I2C
#define ADC_ADDR  0x48    // Base address of ADS1115

#define DHTPIN       25   // DHT11/DHT22
#define BLUELED      14   // GPIO14
#define BTN1         39   // GPIO39 On board button
#define BUSPWR        4   // GPIO04
#define SCK           5   // GPIO5  -- SX1278's SCK
#define MISO         19   // GPIO19 -- SX1278's MISnO
#define MOSI         27   // GPIO27 -- SX1278's MOSI
#define SS           18   // GPIO18 -- SX1278's CS
#define RST          14   // GPIO14 -- SX1278's RESET
#define DI0          26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define ERR_LOWPOWER  0x15  // 00010101
#define INFO_WIFI     0x33  // 00110011
#define INFO_SENSOR   0xAA  // 10101010
#define INFO_NOGPS    0x49  // 01001001

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */

struct SensorConfig
{
  char  ssid[16] = "VESTRONG_S";
  char  password[16] = "";
  int   gps_timeout = 4 * 60;    // wait n seconds to get GPS fix
  int   failedGPSsleep = 5 * 60; // sleep this long if failed to get GPS
  int   reportEvery = 60 * 60;   // get sample every n seconds
  int   fromHour = 6;            // between these hours
  int   toHour = 22;             // between these hours
  long  frequency = FREQUENCY;   // LoRa transmit frequency
  int   txpower = TXPOWER;       // LoRa transmit power
  long  preamble = PREAMBLE;     // bits to send before transmition
  int   syncword = SYNCWORD;     // unique packet identifier
  float txvolts = TXVOLTS;       // power supply must be delivering this voltage in order to xmit.
  int   lowvoltsleep = 60*60*8;  // sleep this long (seconds) if low on volts (8hrs)
  long  bandwidth = BAND;        // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  int   spreadFactor = SPREAD;   // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = CODERATE;   // extra info for CRC
  bool  enableCRC = true;        // cyclic redundancy check mode
} default_config;

enum STARTUPMODE
{
    NORMAL=0,
    WIFI=1,
    RESET=2
};

enum GPSLOCK
{
    LOCK_OK,
    LOCK_FAIL,
    LOCK_WINDOW
};

void loopSensorMode();
void loopWifiMode();
void setConfigParam(const String& var, const char *value);
GPSLOCK getGpsLock();
void getSampleAndSend();
void print_wakeup_reason();
void setupSerial();
void setupLoRa() ;
void smartDelay(unsigned long ms);
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