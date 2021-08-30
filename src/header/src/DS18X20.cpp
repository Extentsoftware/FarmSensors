#include <DS18X20.h>
 
DS18X20::DS18X20() 
{
}

DS18X20::DS18X20(uint8_t pin) 
{
    ds.begin(pin);
}

void DS18X20::init(uint8_t pin) {
    ds.begin(pin);
}

void DS18X20::init(void) {
}

bool DS18X20::search() 
{
  byte type_s;  
  float celsius, fahrenheit;
  
  for (int j=0; j<4; j++)
  {
    memset( addr, 0, sizeof(addr)); 
    ds.reset_search();
  
    for (int i=0; i<4; i++)
    {
        bool found = ds.search(addr, true);
        if (found && addr[0]==0x28 && OneWire::crc8(addr, 7) == addr[7])
        {
          Serial.printf("DS18 %d %d %d %d %d %d %d %d \n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
          return true;
        }
    }
  }
  Serial.printf("DS18 not found \n");
  ds.reset_search();
  return false;
}

float DS18X20::getTemp() 
{
  byte present = 0;
  for (int i=0; i<3; i++)
  {
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);      // start conversion, with parasite power on at the end
    delay(1000);            // maybe 750ms is enough, maybe not

    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad

    for ( int i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
    }

    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
    float celsius = (float)raw / 16.0;
    ds.depower();
    if (celsius>1)
      return celsius;
  }
  
  return -200; 
}

