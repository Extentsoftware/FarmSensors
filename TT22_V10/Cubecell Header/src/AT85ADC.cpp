#include <AT85ADC.h>

//byte AT85ADC::data[12];
  
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

uint16_t AT85ADC::performConversion(bool adc) 
{
  byte present = 0;
 
  ds.reset();
  ds.select(addr);
  ds.write(adc?0x45:0x44, 1);   // start conversion, with parasite power on at the end
  
  delay(800);                  // maybe 750ms is enough, maybe not
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);               // Read Scratchpad

  for (byte i = 0; i < 2; i++) 
  {    // we need 2 bytes
    data[i] = ds.read();
  }
  ds.depower();
  return (data[1] << 8) | data[0]; 
}