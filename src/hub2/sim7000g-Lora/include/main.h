#ifndef __VESOIL_HUB__
#define __VESOIL_HUB__

#define FREQUENCY 868E6
#define BAND      125E3   
#define SPREAD       12   
#define CODERATE      5
#define SYNCWORD      0 
#define PREAMBLE      8
#define LED_PIN       12
#define TOUCH_PIN     32
#define BAT_ADC       35
#define PWR_PIN       4

#define SCK           5    // GPIO5  -- SX1278's SCK
#define MISO          19   // GPIO19 -- SX1278's MISO
#define MOSI          27   // GPIO27 -- SX1278's MOSI
#define SS            18   // GPIO18 -- SX1278's CS
#define RST           14   // GPIO14 -- SX1278's RESET
#define DI0           26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

#define DEFAULT_BROKER            "bongomqtt.uksouth.cloudapp.azure.com"
#define DEFAULT_APN_VODAPHONE     "TM"
#define DEFAULT_APN_ECONET        "econet.net"
#define DEFAULT_APN_WOWWW         "webapn.at"
#define DEFAULT_SIMPIN_VODAPHONE  ""

#define DEFAULT_APN               DEFAULT_APN_VODAPHONE
#define DEFAULT_SIMPIN            DEFAULT_SIMPIN_VODAPHONE
#define DEFAULT_GPRSUSER          ""
#define DEFAULT_GPRSPWD           ""


struct HubConfig
{
  // LoRA settings
  long  frequency = FREQUENCY;  // LoRa transmit frequency
  long  bandwidth = BAND;       // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  long  preamble = PREAMBLE;
  unsigned char   syncword = SYNCWORD;
  int   spreadFactor = SPREAD;  // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = CODERATE;  // extra info for CRC
  bool  enableCRC = true;       //

  // MQTT Settings
  char  broker[42] = DEFAULT_BROKER;

  // GPRS Settings
  bool  gprsEnabled =  true;
  char  apn[32]    =   DEFAULT_APN;      // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
  char  gprsUser[16] = DEFAULT_GPRSUSER;      // GPRS User
  char  gprsPass[16] = DEFAULT_GPRSPWD;      // GPRS Password
  char  simPin[16] =   DEFAULT_SIMPIN;      // GPRS Password

} default_config;

void GetMyMacAddress();
void onReceive(int packetSize);
void SystemCheck();
void getConfig();
void startLoRa();
void SendMQTTBinary(uint8_t *report, int packetSize);
void setup();

#endif