#include <Arduino.h>
#include <Preferences.h>
#include <main.h>
#include <LoRa.h>
#include <CayenneLPP.h>
#include "messagebuffer.h"
#include "BleServer.h"
#include <vesoil.h>

#ifdef HAS_OLED
#include <SSD1306.h>
#endif

BleServer bt;

#define BUFFER_SIZE 128
uint8_t rxpacket[BUFFER_SIZE];
unsigned short txLen=0;
bool sending=false;
MessageBuffer messageBuffer(128);
struct HubConfig config;

void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

#ifdef HAS_OLED
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif

int packetcount=0;
int duppacketcount=0;
float batteryVolts=0;

void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton Hub");
}

#ifdef HAS_DISPLAY

#define L1 0
#define L2 24
#define L3 47
#define C1 0
#define C2 48
#define C3 72

void DisplayPage()
{
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.drawString(C1, L1, "Pkts");
    display.drawString(C2, L1, String(packetcount,DEC));


    display.drawString(C1, L2, "Dups");
    display.drawString(C2, L2, String(duppacketcount,DEC));

    display.display();
}

void InitOLED() {
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(OLED_RST, HIGH); // while OLED is running, must set GPIO16 in high、
  delay(50); 
  display.init();
  display.displayOn();
  display.clear();
  display.drawString(0, 0, "Starting..");
}

#endif

void LoraReceive(int packetSize) 
{  
    Serial.printf("got bytes:%d\n", packetSize);
    Serial.printf("lora data %d bytes\n",packetSize);
    float snr = LoRa.packetSnr();
    float rssi = LoRa.packetRssi();
    long pfe = LoRa.packetFrequencyError();
    CayenneLPP lpp(packetSize + 20);
    Serial.printf("Got %d snr:%f rssi:%f pfe:%ld\n",packetSize, snr, rssi, pfe); 

    LoRa.readBytes(lpp._buffer, packetSize);

    // add signal strength etc
    lpp._cursor = packetSize;
    lpp.addGenericSensor(CH_SNR, snr);
    lpp.addGenericSensor(CH_RSSI, rssi);
    lpp.addGenericSensor(CH_PFE, (float)pfe);  

    // convert to base64
    int size = lpp._cursor * 2;
    uint8_t * base64_buffer = (uint8_t *)malloc(size);
    memset(base64_buffer,0,size);
    int base64_length = encode_base64(lpp._buffer, lpp._cursor, base64_buffer);
    Serial.printf("set value %d bytes %s\n", base64_length, base64_buffer); 

    if (messageBuffer.add(base64_buffer))
    {
      ++packetcount;
      bt.sendData(base64_buffer);
    }
    else
    {
      Serial.printf("Duplicate detected\n"); 
      ++duppacketcount;
      free(base64_buffer);
      return;
    }

    DisplayPage();
}

void startLoRa() {

  Serial.printf("\nStarting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu txpower:%d\n", config.frequency, config.enableCRC, config.codingRate, config.spreadFactor, config.bandwidth, config.txpower);

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DI0);

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
  LoRa.receive();
  
  if (!result) {
    Serial.printf("Starting LoRa failed: err %d", result);
  }  
}

// void checkBattery() {
//   adcAttachPin(35);
//   analogReadResolution(10); // Default of 12 is not very linear. Recommended to use 10 or 11 depending on needed resolution.
//   float v = analogRead(35) * 2.0 * (3.3 / 1024.0);
//   if (batteryVolts != v)
//   {
//     batteryVolts = v;
//     bt.sendVolts(batteryVolts);
//   }
// }

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize>0)
    LoraReceive(packetSize);
//  checkBattery();
}

void setup() {
  // Begin Serial communication at a baudrate of 9600:
  setupSerial();

  #ifdef HAS_OLED
  InitOLED();
  DisplayPage();
  #endif

  bt.init("Lora Hub", NULL);
  startLoRa();
}
