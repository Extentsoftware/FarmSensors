#include <Arduino.h>
#include <Preferences.h>
#include <main.h>
#include <LoRa.h>
#include "ringbuffer.h"
#include "base64.hpp"

#ifdef HAS_OLED
#include <SSD1306.h>
#endif

#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;

#define STX 0x02
#define ETX 0x03

#define BUFFER_SIZE 128
uint8_t rxpacket[BUFFER_SIZE];
unsigned short txLen=0;
bool sending=false;
RingBuffer ringBuffer(16);
struct HubConfig config;

void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

#ifdef HAS_OLED
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif

int packetcount=0;
int duppacketcount=0;

void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton Hub");
}

int HashCode (uint8_t* buffer, int size) {
    int h = 0;
    for (uint8_t* ptr = buffer; size>0; --size, ++ptr)
        h = h * 31 + *ptr;
    return h;
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
    LoRa.readBytes(rxpacket, packetSize);
    int hash = HashCode(rxpacket, packetSize);

    if (ringBuffer.exists(hash))
    {
      Serial.printf("Duplicate detected hash %d\n", hash); 
      ++duppacketcount;
      return;
    }
    Serial.printf("Unique Packet hash %d\n", hash); 
    ringBuffer.add( hash );
    ++packetcount;

    if (SerialBT.connected())
    {
      unsigned char base64_text[128];
      int base64_length = encode_base64(rxpacket,packetSize,base64_text);      
      SerialBT.write(STX);      
      for (int i=0;i<base64_length; i++)
      {
        SerialBT.write(base64_text[i]);
      }      
      SerialBT.write(ETX);
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

void startBT() {
  SerialBT.begin("Lora Hub"); //Bluetooth device name
}

void loopBT() {
  if (SerialBT.available()) {
    Serial.write(SerialBT.read());
  }
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize>0)
    LoraReceive(packetSize);
}

void setup() {
  // Begin Serial communication at a baudrate of 9600:
  setupSerial();

  #ifdef HAS_OLED
  InitOLED();
  DisplayPage();
  #endif

  startBT();
  startLoRa();
}
