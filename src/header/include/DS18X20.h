#ifndef DS18X20_H
#define DS18X20_H

#include <Arduino.h>
#include <OneWire.h>

class DS18X20
{
  private:
    OneWire  ds;

  public:
    byte data[2];
    byte addr[8];

    DS18X20();
    DS18X20(uint8_t pin);
    void init(void);
    void init(uint8_t pin);
    float getTemp();
    bool search();
};
#endif