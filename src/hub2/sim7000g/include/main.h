#define FREQUENCY 868E6
#define BAND      125E3   
#define SPREAD       12   
#define CODERATE      5
#define SYNCWORD      0 
#define PREAMBLE      8
#define LED_PIN       12
#define TOUCH_PIN     34
#define BAT_ADC       35
#define PWR_PIN       4

// on TTG Lora32 1.3 - https://ae01.alicdn.com/kf/H098cb5d5159841b398fcfd4ebf705725W.jpg 
#define LORA32_SCK           5    // GPIO5  -- SX1278's SCK
#define LORA32_MISO          19   // GPIO19 -- SX1278's MISO
#define LORA32_MOSI          27   // GPIO27 -- SX1278's MOSI
#define LORA32_SS            18   // GPIO18 -- SX1278's CS
#define LORA32_RST           14   // GPIO14 -- SX1278's RESET
#define LORA32_DI0           26   // GPIO26 -- SX1278's IRQ(Interrupt Request)

// on TTGO SIM7000g
#define LORA_SS             5  // --> 18 grey x
#define LORA_SCK            18 // -->  5 white x
#define LORA_MISO           19 // --> 19 black x
#define LORA_MOSI           23 // --> 27 brown x
#define LORA_RST            12 // --> 14 red x
#define LORA_DI0            32 // --> 26 orange


#define DEFAULT_BROKER            "bongomqtt.uksouth.cloudapp.azure.com"

#define DEFAULT_APN_VODAPHONE     ""
#define DEFAULT_APN_THREE         "three.co.uk"
#define DEFAULT_APN_ECONET        "econet.net"
#define DEFAULT_APN_WOWWW         "webapn.at"

#define DEFAULT_SIMPIN_VODAPHONE  ""
#define DEFAULT_SIMPIN_THREE      ""

#define DEFAULT_APN        DEFAULT_APN_VODAPHONE
#define DEFAULT_SIMPIN     DEFAULT_SIMPIN_VODAPHONE
#define DEFAULT_GPRSUSER   ""
#define DEFAULT_GPRSPWD    ""


struct HubConfig
{
  // LoRA settings
  long  frequency         = FREQUENCY;  // LoRa transmit frequency
  long  bandwidth         = BAND;       // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  long  preamble          = PREAMBLE;
  int   spreadFactor      = SPREAD;     // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate        = CODERATE;   // extra info for CRC
  bool  enableCRC         = true;       //
  unsigned char syncword  = SYNCWORD;
  int   txpower           = 20;

  // MQTT Settings
  char  broker[42]        = DEFAULT_BROKER;

  // GPRS Settings
  bool  gprsEnabled       = true;
  char  apn[32]           = DEFAULT_APN;        // APN (example: internet.vodafone.pt) use https://wiki.apnchanger.org
  char  gprsUser[16]      = DEFAULT_GPRSUSER;   // GPRS User
  char  gprsPass[16]      = DEFAULT_GPRSPWD;    // GPRS Password
  char  simPin[16]        = DEFAULT_SIMPIN;     // GPRS Password

} default_config;

void GetMyMacAddress();
void onReceive(int packetSize);
void SystemCheck();
void getConfig();
void startLoRa();
void SendMQTTBinary(uint8_t *report, int packetSize);
void setup();
