#ifndef __VESOIL_SENSOR__
#define __VESOIL_SENSOR__

#define FREQUENCY 868E6
#define BAND      125E3   
#define SPREAD       12   
#define CODERATE      6
#define SYNCWORD 0xa5a5
#define PREAMBLE      8
#define TXPOWER      20   // max transmit power
#define MINBATVOLTS 2.7   // minimum voltage on battery - if lower, device goes to deep sleep to recharge
#define ONE_WIRE_BUS  2   // used for Dallas temp sensor

#define ADC_ADDR     0x48    // Base address of ADS1115
#define DHTPIN       25   // DHT11/DHT22
#define SCK           5   // GPIO5  -- SX1278's SCK
#define MISO         19   // GPIO19 -- SX1278's MISnO
#define MOSI         27   // GPIO27 -- SX1278's MOSI
#define SS           18   // GPIO18 -- SX1278's CS
#define RST          14   // GPIO14 -- SX1278's RESET
#define DI0          26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define GPSBAUD      9600


#define TRIGPIN      0 // 13 // ultrasound
#define ECHOPIN      2  // ultrasound


#ifdef TTGO_LORA32_V1

#define OLED_SDA      4
#define OLED_SCL      15
#define OLED_RST      16
#define PWRSDA        21
#define PWRSCL        22
#define BATTERY_PIN   35      // battery level measurement pin, here is the voltage divider connected
#define BUSPWR        4       // GPIO04 -- sensor bus power control
#undef  HASPSRAM
#define BTN1          0 

#endif

#ifdef TTGO_TBEAM_V07

#define GPSRX         12
#define GPSTX         15   
#define PWRSDA        21    
#define PWRSCL        22
#define BATTERY_PIN   35      // battery level measurement pin, here is the voltage divider connected
#define BUSPWR        4       // GPIO04 -- sensor bus power control
#define HASPSRAM
#define PSRAMSIZE     64000000
#define STORESIZE     PSRAMSIZE / sizeof(SensorReport)
#define BTN1          39      // V07 board is GPIO39

#endif

#ifdef TTGO_TBEAM_V10

#define GPSRX         34   
#define GPSTX         12   
#define PWRSDA        21    
#define PWRSCL        22
#define BATTERY_PIN   -1      // battery level measurement pin, here is the voltage divider connected
#define BUSPWR        4       // GPIO04 -- sensor bus power control

#define HASPSRAM
#define PSRAMSIZE     64000000
#define STORESIZE     PSRAMSIZE / sizeof(SensorReport)

#define BTN1          38      // GPIO38 On board button (V07 board is GPIO39)

#endif


#define ERR_LOWPOWER  0x15  // 00010101
#define INFO_WIFI     0x33  // 00110011
#define INFO_SENSOR   0xAA  // 10101010
#define INFO_NOGPS    0x49  // 01001001

struct SensorConfig
{
  char  ssid[16] = "VESTRONG_S";
  char  password[16] = "";
  int   gps_timeout = 4 * 60;    // wait n seconds to get GPS fix
  int   failedGPSsleep = 5 * 60; // sleep this long if failed to get GPS
  int   reportEvery =      60;   // get sample every n seconds
  int   fromHour = 6;            // between these hours
  int   toHour = 22;             // between these hours
  long  frequency = FREQUENCY;   // LoRa transmit frequency
  int   txpower = TXPOWER;       // LoRa transmit power
  long  preamble = PREAMBLE;     // bits to send before transmition
  int   syncword = SYNCWORD;     // unique packet identifier
  float txvolts = MINBATVOLTS;   // power supply must be delivering this voltage in order to xmit.
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
void getSampleAndSend();
void setupSerial();
void smartDelay(unsigned long ms);

// Sensors
void setupTempSensors();
float readGroundTemp();
float readAirTemp();
float readAirHum();
void getSample(SensorReport *report);

// Radio
void startLoRa();
void stopLoRa();
void sendSampleLora(SensorReport *report);

// GPS
void stopGPS();
void startGPS();
GPSLOCK getGpsLock();

// WiFi
void notFound(AsyncWebServerRequest *request);
void setupWifi();

#endif