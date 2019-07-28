#ifndef __VESOIL_HUB__
#define __VESOIL_HUB__

#include <ESPAsyncWebServer.h>
#include <vesoil.h>

#define ERR_LOWPOWER  0x15  // 00010101
#define INFO_WIFI     0x33  // 00110011
#define INFO_SENSOR   0xAA  // 10101010

#define FREQUENCY 868E6
#define BAND      125E3   
#define SPREAD       12   
#define CODERATE      6
#define SYNCWORD 0xa5a5
#define PREAMBLE      8

struct HubConfig
{
  char  ssid[16] = "VESTRONG_H";
  long  frequency = FREQUENCY;  // LoRa transmit frequency
  long  bandwidth = BAND;       // lower (narrower) bandwidth values give longer range but become unreliable the tx/rx drift in frequency
  long  preamble = PREAMBLE;
  int   syncword = SYNCWORD;
  int   spreadFactor = SPREAD;  // signal processing gain. higher values give greater range but take longer (more power) to transmit
  int   codingRate = CODERATE;  // extra info for CRC
  bool  enableCRC = true;       //
} default_config;

enum STARTUPMODE
{
    NORMAL=0,
    WIFI=1,
    RESET=2
};

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
void getConfig(STARTUPMODE startup_mode);
STARTUPMODE getStartupMode();
void flashlight(char code);
void startLoRa();

#define BLUELED 14   // GPIO14
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define PSRAM   16    // 8M byte - https://www.electrodragon.com/product/2pcs-ipus-ips6404-iot-ram/
                      // https://drive.google.com/file/d/1-5NtY1bz0l9eYN9U0U4dP3uASwnMmYGP/view        

#define BATTERY_PIN 35    // battery level measurement pin, here is the voltage divider connected
#define BTN1        39    // GPIO39 On board button
#define STORESIZE   30000

#endif