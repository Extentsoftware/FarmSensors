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
      break;
    default:
      return false;
  } 
  return true;
}

uint16_t AT85ADC::performConversion(uint8_t cmd, uint8_t channel, uint32_t delayMs) 
{
  ds.reset();
  ds.select(addr);
  ds.write(cmd);   // start conversion, with parasite power on at the end
  delay(1);        // maybe 750ms is enough, maybe not
  if (channel!=0xFF)
    ds.write(channel);

  delay(delayMs);                  // maybe 750ms is enough, maybe not
  
  ds.reset();
  ds.select(addr);    
  ds.write(0xBE);               // Read Scratchpad

  for (byte i = 0; i < 2; i++) 
  {    // we need 2 bytes
    data[i] = ds.read();
  }
  ds.depower();
  return (data[1] << 8) | data[0]; 
}

uint16_t AT85ADC::performTemp() 
{
  return performConversion(0x44, 0xFF, 800);
}

uint16_t AT85ADC::performAdc(uint8_t channel) 
{
  return performConversion(0x45, channel, 800);
}

uint16_t AT85ADC::performFreq() 
{
  return performConversion(0x46, 0xFF, 800);
}
