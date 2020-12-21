#include <Arduino.h>
#include <OneWire.h>

class AT85ADC
{
  private:
    OneWire  ds;

  public:
    byte data[2];
    byte addr[8];

    AT85ADC();
    AT85ADC(uint8_t pin);
    void init(void);
    void init(uint8_t pin);
    bool search();
    uint16_t performConversion(bool adc);
};
