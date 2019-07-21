#ifndef __VESOIL_HUB__
#define __VESOIL_HUB__

#include <ESPAsyncWebServer.h>
#include <vesoil.h>

struct SenserConfig
{
  char  ssid[16] = "VESTRONG_H";
  long  frequency = 868E6;      // LoRa transmit frequency
  long  bandwidth = 62.5E3;     // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  int   speadFactor = 12;       // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = 5;         // extra info for CRC
  bool  enableCRC = true;       //
} config;

void onReceive(int packetSize);
void SystemCheck();
void getBatteryVoltage();
void setupBatteryVoltage();
void notFound(AsyncWebServerRequest *request);
void setupWifi();
void showBlock(int packetSize);
void readLoraData(int packetSize);
SensorReport *GetFromStore();
void AddToStore(SensorReport *report);
int GetNextRingBufferPos(int pointer);
void MemoryCheck();

#define BLUELED 14   // GPIO14
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BAND    868E6
#define PSRAM   16    // 8M byte - https://www.electrodragon.com/product/2pcs-ipus-ips6404-iot-ram/
                      // https://drive.google.com/file/d/1-5NtY1bz0l9eYN9U0U4dP3uASwnMmYGP/view        

#define BATTERY_PIN 35    // battery level measurement pin, here is the voltage divider connected
#define BTN1        39    // GPIO39 On board button
#define STORESIZE   30000

#endif