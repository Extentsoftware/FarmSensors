//  generate Device ID based on compile date and time
#include <comptime.h>

// initialize oneWireSlave object
#include <OneWireSlave.h>
OneWireSlave ow;

// Device include file must have the functions void setup() and void loop()
#define FAM 0x91

//  I2C constatants
#define SDA 0 //  SDA-port  (pin 5)
#define SCL 1 //  SCL-port  (pin 6)
#define SRF_ADDRESS 0x70    // Address of the SRF02
#define CMD         0x00    // Command byte, values of 0 being sent with write have to be masked as a byte to stop them being misinterpreted as NULL this is a bug with arduino 1.0
#define RANGEBYTE   0x02    // Byte for start of ranging data

//  Valid Onewire commands
#define CMD_ReadPio 		0xF5
#define CMD_WritePio 		0x5A
#define CMD_Readbuffer 		0xBE
#define CMD_Writebuffer 	0x4E
#define CMD_Copybuffer 		0x48
#define CMD_RecallMemory 	0xB8
#define CMD_StartTmpConv    0x44
#define CMD_StartAdcConv    0x45


uint8_t control[4] {0x00, 0x00, 0x00, 0x00};
uint8_t scratchpad[2] {0x00, 0x00};

uint8_t id[8] = { FAM, SERIAL_NUMBER, 0x00 };

static void initTemp()
{
  ADMUX =
		(0 << REFS2) |     // Sets ref. voltage to Vcc, bit 2
		(1 << REFS1) |     // Sets ref. voltage to Vcc, bit 1   
		(0 << REFS0) |     // Sets ref. voltage to Vcc, bit 0
	    (0 << ADLAR) |     // do not left shift result (for 10-bit values)
		(1 << MUX3)  |     // use ADC2 for input (PB4), MUX bit 3
		(1 << MUX2)  |     // use ADC2 for input (PB4), MUX bit 2
		(1 << MUX1)  |     // use ADC2 for input (PB4), MUX bit 1
		(1 << MUX0);       // use ADC2 for input (PB4), MUX bit 0

  ADCSRA = 
		(1 << ADEN)  |     // Enable ADC 
		(1 << ADPS2) |     // set prescaler to 16, bit 2 
		(0 << ADPS1) |     // set prescaler to 16, bit 1 
		(0 << ADPS0) ;     // set prescaler to 16, bit 0  
}

static void initADC()
{
  ADMUX =
		(0 << REFS2) |     // Sets ref. voltage to Vcc, bit 2
		(0 << REFS1) |     // Sets ref. voltage to Vcc, bit 1   
		(0 << REFS0) |     // Sets ref. voltage to Vcc, bit 0
	    (0 << ADLAR) |     // do not left shift result (for 10-bit values)
		(0 << MUX3)  |     // use ADC2 for input (PB4), MUX bit 3
		(0 << MUX2)  |     // use ADC2 for input (PB4), MUX bit 2
		(1 << MUX1)  |     // use ADC2 for input (PB4), MUX bit 1
		(0 << MUX0);       // use ADC2 for input (PB4), MUX bit 0

  ADCSRA = 
		(1 << ADEN)  |     // Enable ADC 
		(1 << ADPS2) |     // set prescaler to 16, bit 2 
		(0 << ADPS1) |     // set prescaler to 16, bit 1 
		(0 << ADPS0);      // set prescaler to 16, bit 0  
}

static void performConversion(bool adc){
	scratchpad[0] = 0;
	scratchpad[1] = 0;
	
	if (adc)
		initADC();
	else
		initTemp();
	
	ADCSRA |= (1 << ADSC);         // start ADC measurement
    while (ADCSRA & (1 << ADSC) ); // wait till conversion complete 
	// for 10-bit resolution:
	scratchpad[0] = ADCL;
	scratchpad[1] = ADCH;
}


void onCommand(uint8_t cmd) {
  switch (cmd) {
    case CMD_Readbuffer: 
		ow.write(&scratchpad[0], 2, &ow.reset);
		break;
    case CMD_Writebuffer: 
    	ow.read(&control[0], 2, &ow.reset);
		break;
	case CMD_StartTmpConv:
		performConversion(false);
		ow.reset();
		break;
	case CMD_StartAdcConv:
		performConversion(true);
		ow.reset();
		break;
    case CMD_RecallMemory:
		ow.reset();
	 	break;
  }
};

void loop()
{
}

int main() {
  ow.begin(&onCommand, id);
  while (1) {
    loop();
  }
}

