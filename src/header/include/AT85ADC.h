#ifndef AT85ADC_H
#define AT85ADC_H

#include <Arduino.h>
#include <OneWire.h>


#define AT85_ADC0 0
#define AT85_ADC1 1
#define AT85_ADC2 2
#define AT85_ADC3 3
#define AT85_TEMP 0x8F
#define AT85_ADC_VCC 0x0c

#define AT85_ADC_PIN1 AT85ADC0
#define AT85_ADC_PIN7 AT85ADC1
#define AT85_ADC_PIN3 AT85ADC2
#define AT85_ADC_PIN2 AT85ADC3

#define CMD_Readbuffer 		  0xC0	// read the ADC buffer
#define CMD_ReadAvg 		    0xC1	// computer average into scratchpad and return it
#define CMD_ReadScratchpad  0xC2	// read the scratchpad
#define CMD_ADCSingle		    0xC3	// start standard single ADC
#define CMD_ADCLowNoise		  0xC4	// start single interrupt-driven low-noise ADC
#define CMD_ADCContinous	  0xC5	// start continous interrupt-driven ADC, filling the buffer
#define CMD_StartFrqConv	  0xC6	// start frequency conversion

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
    uint16_t performConversion(uint8_t channel, uint32_t delayMs);
    void performContConversion(uint8_t channel, uint32_t delayMs);
    uint16_t performAvgConversion(uint8_t channel, uint32_t delayMs);
    uint16_t performAdc(uint8_t channel, uint8_t delay);
    uint16_t performFreq();
    uint16_t average(uint16_t samples[]);
    uint16_t readscratchpad(); 
};
#endif