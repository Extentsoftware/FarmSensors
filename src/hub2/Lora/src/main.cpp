#include <Arduino.h>
#include <SPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include <main.h>
#include <LoRa.h>
#include "ringbuffer.h"
#include <IotWebConf.h>
#include <IotWebConfUsing.h> // This loads aliases for easier class names.

// 4 SPIs on esp32, SPI0 and SPI1 are used for internal flash.
// HSPI/VSPI has 3 CS line each, to drive 3 devices each.

//TTGO lora32 has SPI pins:
//        MISO    MOSI     SCK     CS
//SD      IO02    IO15     IO14    IO13
//Lora    IO19    IO27     IO05    IO18

#define SIM_MISO 2
#define SIM_MOSI 15
#define SIM_SCLK 14
#define SIM_CS   13

SPIClass spiSim7000g(HSPI);

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
#include <SSD1306.h>
SSD1306 display(OLED_ADDR, OLED_SDA, OLED_SCL);
#endif

#ifdef HAS_WIFI
const char thingName[] = "Hub";               // -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char wifiInitialApPassword[] = "";      // -- Initial password to connect to the Thing, when it creates an own Access Point.
const char configVersion[] = "1.2";
DNSServer dnsServer;
WebServer server(80);
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, configVersion);
#endif

int packetcount=0;
bool wifiConnected=false;
String wifiStatus = "Off";

void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.println("VESTRONG LaPoulton Hub");
}

#ifdef HAS_DISPLAY

#define L1 0
#define L2 12
#define L3 24
#define L4 36
#define L5 48
#define C1 0
#define C2 32
#define C3 64
#define C4 96

void DisplayPage()
{
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_LEFT);

    display.drawString(C1, L2, "WiFi");
    display.drawString(C2, L2, wifiStatus);

    display.drawString(C1, L3, "LORA");
    display.drawString(C2, L3, "Pkts");
    display.drawString(C3, L3, String(packetcount,DEC));
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

int HashCode (uint8_t* buffer, int size) {
    int h = 0;
    for (uint8_t* ptr = buffer; size>0; --size, ++ptr)
        h = h * 31 + *ptr;
    return h;
}

void LoraReceive(int packetSize) 
{  
    Serial.printf("got bytes:%d\n", packetSize);
    LoRa.readBytes(rxpacket, packetSize);
    int hash = HashCode(rxpacket, packetSize);

    if (ringBuffer.exists(hash))
    {
      Serial.printf("Duplicate detected hash %d\n", hash); 
      return;
    }
    Serial.printf("Unique Packet hash %d\n", hash); 
    ringBuffer.add( hash );
    ++packetcount;

    spiSim7000g.writeBytes(rxpacket, packetSize); 

    DisplayPage();
}

void startSim() {
  spiSim7000g.begin(SIM_SCK, SIM_MISO, SIM_MOSI, SIM_SS);
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

  // startSim();
  startLoRa();
}
