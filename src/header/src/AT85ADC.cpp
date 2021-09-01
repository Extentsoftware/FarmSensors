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
  
  for (int j=0; j<4; j++)
  {
    memset( addr, 0, sizeof(addr)); 
    ds.reset_search();
  
    for (int i=0; i<4; i++)
    {
        bool found = ds.search(addr, true);
        if (found && addr[0]==0x91 && OneWire::crc8(addr, 7) == addr[7])
        {
          Serial.printf("AT85 %d %d %d %d %d %d %d %d \n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
          return true;
        }
    }
  }

  Serial.printf("AT85 not found \n");
  ds.reset_search();
  return false;
}

uint16_t AT85ADC::performConversion(uint8_t channel, uint32_t delayMs) 
{
  byte present = 0;
 
  ds.reset();
  ds.select(addr);    
  ds.write(CMD_ADCLowNoise, 1);         // set the adc channel (ADMUX on ATTINY85)
  ds.write(channel, 1);                 // and start conversion
  delay(delayMs);                       // for adc this should be maybe 10ms
  present = ds.reset();
  ds.select(addr);    
  ds.write(CMD_Readbuffer, 1);          // Read Scratchpad
  data[0]=ds.read();
  data[1]=ds.read();

  data[1] &= 0x03;

  ds.depower();
  return (data[1] << 8) | data[0]; 
}

uint16_t AT85ADC::performTemp() 
{
  return performAdc(AT85_TEMP);
}

uint16_t AT85ADC::average(uint16_t samples[])
{ 	
	uint16_t min=65535;
	uint16_t max=0;
	uint32_t sum=0;
	uint8_t index_min, index_max;

	for(uint8_t i=0;i<6;i++)
	{
		if (min>samples[i])
		{
			index_min = i;
			min = samples[i];
		}
	}

	for(uint8_t i=0;i<6;i++)
	{
		if (max<samples[i])
		{
			index_max = i;
			max = samples[i];
		}
	}

	// sum all except min and max
	for(uint8_t i=0;i<6;i++)
	{
		if (i!=index_max && i!=index_min)
			sum += samples[i];
	}
	sum  = sum >> 2; // divide by four to get average
  return sum;
}


uint16_t AT85ADC::performAdc(uint8_t channel) 
{
  uint16_t result=0;
  uint16_t samples[6];
  int sample=0;
  do
  {
    int tries=8;
    do
    {
      result = performConversion(channel, 1);
      --tries;
      Serial.printf("AT85 chan %d try %d result %d sample %d\n", channel, tries, result, sample);
    } while ((result==0 || result==65535 || result==1023) && tries>0);
    samples[sample++] = result;
  } while (sample<6);
  return average(samples);
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
