//  generate Device ID based on compile date and time
#include <comptime.h>

// initialize oneWireSlave object
#include <OneWireSlave.h>
OneWireSlave ow;

// Device include file must have the functions void setup() and void loop()
#define FAM 0x91

//  Valid Onewire commands
#define CMD_Readbuffer 		0xBE
#define CMD_Writebuffer 	0x4E
#define CMD_Copybuffer 		0x48
#define CMD_RecallMemory 	0xB8
#define CMD_StartTmpConv    0x44
#define CMD_StartAdcConv    0x45


uint8_t control[4] {0x00, 0x00, 0x00, 0x00};
uint8_t scratchpad[2] {0x00, 0x00};

uint8_t id[8] = { FAM, SERIAL_NUMBER, 0x00 };

static void StartPWM()
{
	    /*
    Control Register for Timer/Counter-1 (Timer/Counter-1 is configured with just one register: this one)
    TCCR1 is 8 bits: [CTC1:PWM1A:COM1A1:COM1A0:CS13:CS12:CS11:CS10]
    0<<PWM1A: bit PWM1A remains clear, which prevents Timer/Counter-1 from using pin OC1A (which is shared with OC0B)
    0<<COM1A0: bits COM1A0 and COM1A1 remain clear, which also prevents Timer/Counter-1 from using pin OC1A (see PWM1A above)
    1<<CS10: sets bit CS11 which tells Timer/Counter-1  to not use a prescalar
    */
    //TCCR1 = 0<<PWM1A | 0<<COM1A0 | 1<<CS10;

    /*
    General Control Register for Timer/Counter-1 (this is for Timer/Counter-1 and is a poorly named register)
    GTCCR is 8 bits: [TSM:PWM1B:COM1B1:COM1B0:FOC1B:FOC1A:PSR1:PSR0]
    1<<PWM1B: sets bit PWM1B which enables the use of OC1B (since we disabled using OC1A in TCCR1)
    2<<COM1B0: sets bit COM1B1 and leaves COM1B0 clear, which (when in PWM mode) clears OC1B on compare-match, and sets at BOTTOM
    */
    //GTCCR = 1<<PWM1B | 2<<COM1B0;

	// Timer/Counter 1 initialization
	// Clock source: System Clock
	// Clock value: 16000.000 kHz
	// Mode: Normal top=0xFF
	// OC1A output: Toggle on compare match
	// OC1B output: Disconnected
	// Timer Period: 0.016 ms
	// Output Pulse(s):
	// OC1A Period: 0.032 ms Width: 0.016 ms
	// Timer1 Overflow Interrupt: Off
	// Compare A Match Interrupt: Off
	// Compare B Match Interrupt: Off
	DDRB= (1<<DDB1) ; // ouput pin 1
	PLLCSR=(0<<PCKE) | (0<<PLLE) | (0<<PLOCK);

	TCCR1=(0<<CTC1) | (0<<PWM1A) | (0<<COM1A1) | (1<<COM1A0) | (0<<CS13) | (0<<CS12) | (0<<CS11) | (1<<CS10);
	GTCCR=(0<<TSM) | (0<<PWM1B) | (0<<COM1B1) | (0<<COM1B0) | (0<<PSR1) | (0<<PSR0);
	TCNT1=0x00;
	OCR1A=0x00;
	OCR1B=0x00;
	OCR1C=0x00;
	
}

static void initTemp()
{
	ADMUX =
		(0 << REFS2) |     // Sets ref. voltage to 1.1v
		(1 << REFS1) |     // Sets ref. voltage to 1.1v
		(0 << REFS0) |     // Sets ref. voltage to 1.1v
	    (0 << ADLAR) |     // do not left shift result (for 10-bit values)
		(1 << MUX3)  |     // 
		(1 << MUX2)  |     // Set of internal temp sensor
		(1 << MUX1)  |     // 
		(1 << MUX0);       // 

  	ADCSRA = 
		(1 << ADEN)  |     // Enable ADC 
		(1 << ADPS2) |     // set prescaler to 128
		(1 << ADPS1) |
		(1 << ADPS0); 
}

static void initADC2()
{
  	ADMUX =
		(0 << REFS2) |     // Sets ref. voltage to Vcc, bit 2
		(0 << REFS1) |     // Sets ref. voltage to Vcc, bit 1   
		(0 << REFS0) |     // Sets ref. voltage to Vcc, bit 0
	    (0 << ADLAR) |     // do not left shift result (for 10-bit values)
		(0 << MUX3)  |     // use ADC2 for input (PB0), MUX bit 3
		(0 << MUX2)  |     // use ADC2 for input (PB0), MUX bit 2
		(1 << MUX1)  |     // use ADC2 for input (PB0), MUX bit 1
		(1 << MUX0);       // use ADC2 for input (PB0), MUX bit 0

  	ADCSRA = 
		(1 << ADEN)  |     // Enable ADC 
		(1 << ADPS2) |     // set prescaler to 128
		(1 << ADPS1) |
		(1 << ADPS0); 
}

static void performConversion(bool adc){
	scratchpad[0] = 0;
	scratchpad[1] = 0;
	
	if (adc)
		initADC2();
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

int main() {
	StartPWM();
  	ow.begin(&onCommand, id);
	while (1) {}
}

