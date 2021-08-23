/////////////////////////////////////////////////////////////
//
// Attiny85 1-wire ADC and frequency counter.
//
/////////////////////////////////////////////////////////////

//  generate Device ID based on compile date and time
#include <comptime.h>

// initialize oneWireSlave object
#include <OneWireSlave.h>
OneWireSlave ow;

// Device include file must have the functions void setup() and void loop()
#define FAM 0x91

//  Valid Onewire commands
#define CMD_Readbuffer 		0xC0
#define CMD_SetADCChannel   0xC3
#define CMD_StartFrqConv	0xC5
#define CMD_ReadAdc			0xC7

uint8_t channel=0;
static uint8_t scratchpad[2] {0x00, 0x00};
uint8_t id[8] = { FAM, SERIAL_NUMBER, 0x00 };
static volatile int counter=0;

ISR(PCINT0_vect)
{
	++counter;
}

ISR(TIM1_OVF_vect)
{
	// count complete, save results in scratchpad
	TIMSK &= ~(1<<TOIE1);  // stop running interrupt
	TCCR1=0;
	GTCCR=0;
	// disable pin-change triggers
	GIMSK &= ~(1<<PCIE);
	PCMSK = (0<<PCINT5) | (0<<PCINT4) | (0<<PCINT3) | (0<<PCINT2) | (0<<PCINT1) | (0<<PCINT0);
	GIFR &= ~(1<<PCIF);

	scratchpad[0] = counter & 0xFF;
	scratchpad[1] = counter >> 8;
}

static void performCount()
{
	counter=0;
	
	DDRB= (1<<DDB1) ; // ouput pin 1 (real pin 6)
	PLLCSR=(0<<PCKE) | (0<<PLLE) | (0<<PLOCK);

	// Timer Period: 4.096 ms
	TCCR1=(0<<CTC1) | (0<<PWM1A) | (0<<COM1A1) | (0<<COM1A0) | (1<<CS13) | (0<<CS12) | (0<<CS11) | (1<<CS10);
	GTCCR=(0<<TSM) | (0<<PWM1B) | (0<<COM1B1) | (0<<COM1B0) | (0<<PSR1) | (0<<PSR0);
	TCNT1=0x00;
	OCR1A=0x00;
	OCR1B=0x00;
	OCR1C=0x00;

	// turn on pin change interrupt on pin PB0 (real pin 5)
	GIMSK |= (1<<PCIE);
	PCMSK = (0<<PCINT5) | (0<<PCINT4) | (0<<PCINT3) | (0<<PCINT2) | (0<<PCINT1) | (1<<PCINT0);
	GIFR |= (1<<PCIF);

	TIMSK |= (1<<TOIE1);  // continually running

	// and start counting!
	counter=0;
}

static void startAdc()
{
	scratchpad[0] = 0;
	scratchpad[1] = 0;
  	ADMUX = channel; 
  	ADCSRA = 
  		(1 << ADEN)  | 			// enable adc		
		(1 << ADPS2) |  		// set prescaler to 128
		(1 << ADPS1) |
		(1 << ADPS0); 
	ADCSRA |= (1 << ADSC);      // start ADC measurement
}

static void startAdcWithReset()
{
	startAdc();
	ow.reset();
}

static void readAdc()
{ 	
	scratchpad[0] = 0;
	scratchpad[1] = 0;
	
    while (ADCSRA & (1 << ADSC) )
	{
		
	}; 	// wait till conversion complete 

	scratchpad[0] = ADCL;
	scratchpad[1] = ADCH & 0x03;
}

void onCommand(uint8_t cmd) {
  switch (cmd) {
	case CMD_SetADCChannel:
		channel=0;
		ow.read(&channel, 1, &startAdcWithReset);
		break;
    case CMD_ReadAdc: 
		readAdc();
		ow.reset();
		break;
    case CMD_Readbuffer: 
		ow.write(&scratchpad[0], 2, &ow.reset);
		break;
	case CMD_StartFrqConv:
		performCount();
		ow.reset();
		break;
  }
};

int main() {
	TIMSK &= ~(1<<TOIE1);  // stop running interrupt
  	ow.begin(&onCommand, id);
	while (1) {}
}

