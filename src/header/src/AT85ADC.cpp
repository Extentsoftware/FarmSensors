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
  memset( addr, 0, sizeof(addr));
  
  ds.reset_search();
  ds.target_search(0x91);  
  
  if ( !ds.search(addr, true)) {
    Serial.println("search - nothing found");
    delay(200);
    ds.reset_search();
    return false;
  }

  Serial.printf("%d %d %d %d %d %d %d %d \n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);

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
