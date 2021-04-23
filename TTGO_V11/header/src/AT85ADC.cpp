#include <AT85ADC.h>
 
AT85ADC::AT85ADC() 
{
}

AT85ADC::AT85ADC(uint8_t pin) 
{
    ds.begin(pin);
}

void AT85ADC::init(uint8_t pin) {
    ds.begin(pin);
}

void AT85ADC::init(void) {
}

bool AT85ADC::search() 
{
  byte type_s;  
  float celsius, fahrenheit;
  
  ds.reset_search();
  
  if ( !ds.search(addr, true)) {
    delay(200);
    ds.reset_search();
    return false;
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      return false;
  }

    // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x91:
      type_s = 0;
      break;
    default:
      return false;
  } 
  return true;
}

uint16_t AT85ADC::performConversion(uint8_t channel, uint32_t delayMs) 
{
  byte present = 0;
 
  ds.reset();
  ds.select(addr);
  ds.write(CMD_SetADCChannel);      // set the adc channel (ADMUX on ATTINY85)
  ds.write(channel);

  delay(delayMs);                   // for adc this should be maybe 10ms

  present = ds.reset();
  ds.select(addr);    
  ds.write(CMD_ReadAdc);            // Read ADC

  delay(1);                         // wait for adc read to complete
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(CMD_Readbuffer);         // Read Scratchpad
  data[0]=ds.read();
  data[1]=ds.read();

  ds.depower();
  return (data[1] << 8) | data[0]; 
}

uint16_t AT85ADC::performTemp() 
{
  return performConversion(AT85_TEMP, 100);
}

uint16_t AT85ADC::performAdc(uint8_t channel) 
{
  return performConversion(channel, 100);
}

uint16_t AT85ADC::performFreq() 
{
  byte present = 0;
 
  ds.reset();
  ds.select(addr);
  ds.write(CMD_StartFrqConv);  
  delay(1);                         // maybe 750ms is enough, maybe not

  present = ds.reset();
  ds.select(addr);    
  ds.write(CMD_Readbuffer);         // Read Scratchpad

  data[0]=ds.read();
  data[1]=ds.read();

  ds.depower();
  Serial.printf( "0=%d 1=%d\n", data[0],data[1]);
  return (data[1] << 8) | data[0]; 
}
