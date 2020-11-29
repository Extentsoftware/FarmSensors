#include "Arduino.h"
#include "OWSlave.h"

byte MyROM[7] = { 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
//const byte MyROM[7] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff };

Pin datawire(2);
Pin debug_(4);

const byte DS18B20_START_CONVERSION = 0x44;
const byte DS18B20_READ_SCRATCHPAD = 0xBE;
const byte DS18B20_WRITE_SCRATCHPAD = 0x4E;

enum DeviceState
{
	DS_WaitingReset,
	DS_WaitingCommand,
	DS_ConvertingTemperature,
	DS_TemperatureConverted,
};

volatile DeviceState state = DS_WaitingReset;
volatile unsigned long conversionStartTime = 0;
byte scratchpad[9];

void owReceive(OWSlave::ReceiveEvent evt, byte data);

void setup()
{
	OneWireSlave.setReceiveCallback(&owReceive);
	OneWireSlave.begin(MyROM, datawire);
}


void loop()
{
	//       // get here if duration is bad.
    // delayMicroseconds(1000);
	// if (OneWireSlave.debugValue>0)
	// {
    // 	debug_.writeHigh();
    // 	delayMicroseconds(OneWireSlave.debugValue*10);
    // 	debug_.writeLow();
	// }
}

void owReceive(OWSlave::ReceiveEvent evt, byte data)
{
	switch (evt)
	{
	case OWSlave::RE_Byte:
		switch (state)
		{
		case DS_ConvertingTemperature:
			break;
		case DS_TemperatureConverted:
			break;
		case DS_WaitingReset:
			break;
		case DS_WaitingCommand:
			switch (data)
			{
			case DS18B20_START_CONVERSION:
				state = DS_ConvertingTemperature;
				conversionStartTime = millis();
				OneWireSlave.writeByte(0); // send zeros as long as the conversion is not finished
				break;

			case DS18B20_READ_SCRATCHPAD:
				state = DS_WaitingReset;
				OneWireSlave.writeData(scratchpad, 9);
				break;

			case DS18B20_WRITE_SCRATCHPAD:
				
				break;
			}
			break;
		}
		break;

	case OWSlave::RE_Reset:
		state = DS_WaitingCommand;
		break;

	case OWSlave::RE_Error:
		state = DS_WaitingReset;
		break;
	}
}