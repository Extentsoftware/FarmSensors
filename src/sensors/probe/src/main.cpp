/////////////////////////////////////////////////////////////
//
// Attiny85 1-wire ADC and frequency counter.
//
/////////////////////////////////////////////////////////////

//  generate Device ID based on compile date and time
#include <comptime.h>

// initialize oneWireSlave object
#include <OneWireSlave.h>
#include <avr/sleep.h>

OneWireSlave ow;

// Device include file must have the functions void setup() and void loop()
#define FAM 0x91

//  Valid Onewire commands
#define CMD_Readbuffer 		  0xC0	// read the ADC buffer
#define CMD_ReadAvg 		  0xC1	// computer average into scratchpad and return it
#define CMD_ReadScratchpad    0xC2	// read the scratchpad
#define CMD_ADCSingle		  0xC3	// start standard single ADC
#define CMD_ADCLowNoise		  0xC4	// start single interrupt-driven low-noise ADC
#define CMD_ADCContinous	  0xC5	// start continous interrupt-driven ADC, filling the buffer
#define CMD_StartFrqConv	  0xC6	// start frequency conversion

#define BUFFER_SIZE_SHIFT	5
#define BUFFER_SIZE			(1 << BUFFER_SIZE_SHIFT) + 2

uint16_t adc_results[BUFFER_SIZE];
uint8_t channel=0;
static uint16_t scratchpad;
uint8_t id[8] = { FAM, SERIAL_NUMBER, 0x00 };
static volatile int counter=0;
volatile uint8_t adc_results_index = 0;
volatile uint8_t adc_conversion_done = 0;

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

	scratchpad = counter;
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

ISR(ADC_vect)
{  
 	scratchpad = ADC;
	adc_results[adc_results_index++] = scratchpad;
	if (adc_results_index >= BUFFER_SIZE)
	{
		adc_results_index = 0;
		adc_conversion_done = 1;
		/* Disable ADC and clear interrupt flag. we are done */
		ADCSRA = 0;
		ADCSRA = (1 << ADIF);
	}
} 

static void clearAdcBuffer()
{
	for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
		adc_results[i] = 0;
	}
	adc_conversion_done = 0;
	scratchpad = 0;
}

static void startAdc(bool withInterrupt, bool continuous)
{
	if (continuous)
		withInterrupt = true;

	clearAdcBuffer();
  	ADMUX = channel; 
  	ADCSRA = 
  		(1 << ADEN)  | 			// enable adc		
		(1 << ADPS2) |  		// set prescaler to 128
		(1 << ADPS1) |
		(1 << ADPS0); 
	
	if (withInterrupt)
		ADCSRA |= (1 << ADIE);  

	if (continuous)
	{
		ADCSRB = 0;				 // Trigger source free-running
		ADCSRA |= (1 << ADATE);  // Auto Trigger Enable
	}

	DIDR0 = 1 << ADC3D;			// disable digital input on the ADC3 (Pin2)
	ADCSRA |= (1 << ADSC);      // start ADC measurement
}


// block until adc complete
static void readAdc()
{ 	
    while (ADCSRA & (1 << ADSC) ) {}; // wait till conversion complete 
	scratchpad = ADC;
}

// Compute scratchpad as an average of the adc buffer
static void computeAdcAvg()
{ 	
	uint16_t min=65535;
	uint16_t max=0;
	uint32_t sum=0;
	uint8_t index_min, index_max;

	for(uint8_t i=0; i<BUFFER_SIZE; i++)
	{
		if (min > adc_results[i])
		{
			index_min = i;
			min = adc_results[i];
		}
	}

	for(uint8_t i=0; i<BUFFER_SIZE; i++)
	{
		if (max < adc_results[i])
		{
			index_max = i;
			max = adc_results[i];
		}
	}

	// sum all except min and max
	for(uint8_t i=0; i<BUFFER_SIZE; i++)
	{
		if (i!=index_max && i!=index_min)
			sum += adc_results[i];
	}
	scratchpad = sum >> BUFFER_SIZE_SHIFT; // divide to get average
}

static void startAdcWithReset()
{
	startAdc(false, false);
	ow.reset();
	readAdc();
}

static void startLowNoiseAdc()
{
	startAdc(true, false);
	ow.reset();
    sleep_cpu();
}

static void startContinousAdc()
{
	startAdc(true, true);
	ow.reset();
}

void onCommand(uint8_t cmd) {
  switch (cmd) {
	case CMD_ADCSingle:
		channel=0;
		ow.read(&channel, 1, &startAdcWithReset);
		break;

    case CMD_ADCLowNoise: 
		channel=0;
		ow.read(&channel, 1, &startLowNoiseAdc);
		break;

    case CMD_ADCContinous: 
		channel=0;
		ow.read(&channel, 1, &startContinousAdc);
		break;
	
    case CMD_ReadAvg: 
		computeAdcAvg();
		ow.write((uint8_t*)&scratchpad, 2, &ow.reset);
		break;

    case CMD_ReadScratchpad: 
		ow.write((uint8_t*)&scratchpad, 2, &ow.reset);
		break;
    
	case CMD_Readbuffer: 
		ow.write((uint8_t*)&adc_results, sizeof(adc_results), &ow.reset);
		break;

	case CMD_StartFrqConv:
		performCount();
		ow.reset();
		break;
  }
};

int main() {
	set_sleep_mode(SLEEP_MODE_ADC);	
	sleep_enable();
	sei();

	TIMSK &= ~(1<<TOIE1);  // stop running interrupt
  	ow.begin(&onCommand, id);
	while (1) {}
}

