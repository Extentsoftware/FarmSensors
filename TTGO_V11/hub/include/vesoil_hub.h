#ifndef __VESOIL_HUB__
#define __VESOIL_HUB__

#define ERR_LOWPOWER  0x15  // 00010101
#define INFO_WIFI     0x33  // 00110011
#define INFO_NORMAL   0xAA  // 10101010

#define FREQUENCY 868E6
#define BAND      125E3   
#define SPREAD       12   
#define CODERATE      5
#define SYNCWORD      0 
#define PREAMBLE      8

#define SCK           5    // GPIO5  -- SX1278's SCK
#define MISO          19   // GPIO19 -- SX1278's MISO
#define MOSI          27   // GPIO27 -- SX1278's MOSI
#define SS            18   // GPIO18 -- SX1278's CS
#define RST           14   // GPIO14 -- SX1278's RESET
#define DI0           26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#ifdef TTGO_LORA32_V1

#define HAS_OLED
#define OLED_SDA      4
#define OLED_SCL      15
#define OLED_RST      16
#define OLED_ADDR     0x3C

#define PWRSDA        21
#define PWRSCL        22
#define BATTERY_PIN   35      // battery level measurement pin, here is the voltage divider connected
#define BUSPWR        4       // GPIO04 -- sensor bus power control

#define AT_RX        13
#define AT_TX        12

#undef  HASPSRAM

#define BTN1          0 
#define BTN2          36 

#endif


#ifdef TTGO_TBEAM_V07

#define PWRSDA        21    
#define PWRSCL        22
#define BATTERY_PIN   35      // battery level measurement pin, here is the voltage divider connected
#define BUSPWR        4       // GPIO04 -- sensor bus power control

#define HASPSRAM
#define PSRAMSIZE     64000000
#define STORESIZE     PSRAMSIZE / sizeof(SensorReport)

#define BTN1          38      // GPIO38 On board button (V07 board is GPIO39)

#endif

#ifdef TTGO_TBEAM_V10

#define PWRSDA        21    
#define PWRSCL        22
#define BATTERY_PIN   35      // battery level measurement pin, here is the voltage divider connected
#define BUSPWR        4       // GPIO04 -- sensor bus power control

#define HASPSRAM
#define PSRAMSIZE     64000000
#define STORESIZE     PSRAMSIZE / sizeof(SensorReport)

#define BTN1          38      // GPIO38 On board button (V07 board is GPIO39)

#endif


struct HubConfig
{
  char  ssid[16] = "VESTRONG_H";
  char  password[16] = "";
  long  frequency = FREQUENCY;  // LoRa transmit frequency
  long  bandwidth = BAND;       // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  long  preamble = PREAMBLE;
  unsigned char   syncword = SYNCWORD;
  int   spreadFactor = SPREAD;  // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = CODERATE;  // extra info for CRC
  bool  enableCRC = true;       //
  char  broker[42] = "bongomqtt.uksouth.cloudapp.azure.com";

  char  apn[32]    = "TM"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
  char  gprsUser[16] = "";    // GPRS User
  char  gprsPass[16] = "";    // GPRS Password
  
  //char apn[32]    = "wap.vodafone.co.uk"; // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
  //char gprsUser[16] = "wap";    // GPRS User
  //char gprsPass[16] = "wap";    // GPRS Password
  
  float EmptyHeight =  220;     // ultrasound reads 2.2m when the tank is empty
  float FullHeight  = 20;       // ultrasound reads 20cm when the tank is full
} default_config;

enum STARTUPMODE
{
    NORMAL=0,
    RESET=2
};

void GetMyMacAddress();
int detectDebouncedBtnPush();
void onReceive(int packetSize);
void SystemCheck();
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void exitWifi();
void toggleWifi();
void readLoraData(int packetSize);
void getConfig(STARTUPMODE startup_mode);
STARTUPMODE getStartupMode();
void flashlight(char code);
void startLoRa();
void doModemStart();
bool doNetworkConnect();
void doSetupMQTT();
void SendMQTTBinary(uint8_t *report, int packetSize);
void InitOLED();
void DisplayPage(int page);
void ShowNextPage();
void ModemCheck();
void setup();

#endif