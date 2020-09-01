#include <Arduino.h>
#include <OneWire.h>

class D18B20
{
  private:
    OneWire  ds;

  public:
    D18B20();
    D18B20(uint8_t pin);
    void init(void);
    void init(uint8_t pin);
    float getSample(void);
};
