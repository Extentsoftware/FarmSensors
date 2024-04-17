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

#define LORA_SCK          5    // GPIO5  -- SX1278's SCK
#define LORA_MISO         19   // GPIO19 -- SX1278's MISO
#define LORA_MOSI         27   // GPIO27 -- SX1278's MOSI
#define LORA_SS           18   // GPIO18 -- SX1278's CS
#define LORA_RST          23   // SX1278's RESET
#define LORA_DI0          26   // SX1278's IRQ(Interrupt Request)

#define DEFAULT_BROKER            "iot.vestrong.eu"

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

} default_config;

void GetMyMacAddress();

void onReceive(int packetSize);
void SystemCheck();
void getConfig();
void startLoRa();
void SendMQTTBinary(uint8_t *report, int packetSize);
void setup();
