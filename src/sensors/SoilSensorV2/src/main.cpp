// https://github.com/HelTecAutomation/ASR650x-Arduino 
// https://heltec-automation-docs.readthedocs.io/en/latest/cubecell/capsule-sensor/htcc-ac01/sensor_pinout_diagram.html

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <CayenneLPP.h>
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include "DS18X20.h"
#include "vesoil.h"

#define RF_FREQUENCY                                868E6           // Hz
#define TX_OUTPUT_POWER                             22              // dBm
uint32_t _bandwidth                                 = 0;            // [0: 125 kHz,  4 = LORA_BW_041
uint32_t  _spreadingFactor                          = 12;           // SF_12 [SF7..SF12] - 7
uint8_t  _codingRate                                = 1;            // LORA_CR_4_5; [1: 4/5,    - 1
#define LORA_PREAMBLE_LENGTH                        8               // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0               // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define LORA_CRC                                    true
#define LORA_FREQ_HOP                               false
#define LORA_HOP_PERIOD                             0
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30 // Define the payload size here
#define SYNCWORD                                    0

#define BATTLOW                                     3000
#define COLOR_START                                 0x0000FF   //color blue
#define COLOR_SENDING                               0xFFFFFF   //color white
#define COLOR_SENT                                  0x00FF00   //color green
#define COLOR_FAIL_SENSOR                           0xFF0000   // red 
#define COLOR_FAIL_TX                               0xFF00FF
#define SCHMITT_TRIGGER_PIN_1                       GPIO1  // Pin for soil moisture Schmitt trigger
#define SCHMITT_TRIGGER_PIN_2                       GPIO2  // Pin for air reference Schmitt trigger
#define TIMER_COUNT_PERIOD_MS                       1000   // Count period in milliseconds
#define EXPECTED_FREQUENCY_HZ                       100000 // Expected ~100kHz frequency

// Timer and frequency counting variables
volatile uint32_t timer1_count = 0;
volatile uint32_t timer2_count = 0;
volatile bool timer1_overflow = false;
volatile bool timer2_overflow = false;
uint32_t last_timer1_count = 0;
uint32_t last_timer2_count = 0;
float soil_moisture_frequency = 0.0f;
float air_reference_frequency = 0.0f;
bool sense_m_success = false;

#define SECONDS                                     1000
#define MINUTES                                     60 * SECONDS
#define HOURS                                       60 * MINUTES
#define DAYS                                        24 * HOURS
#define TIME_UNTIL_WAKEUP_TEST                      5 * SECONDS
#define TIME_UNTIL_WAKEUP_NORMAL                    1 * HOURS
//#define TIME_UNTIL_WAKEUP_NORMAL                    5 * SECONDS
#define TIME_UNTIL_WAKEUP_LOWPOWER                  60 * SECONDS // 1 * DAYS

static TimerEvent_t wakeUp;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void onWakeUp();
void onSleep(uint32_t duration);
void initFrequencyCounters();
void calculateFrequencies();
void timer1ISR();
void timer2ISR();

typedef enum
{
    LOWPOWER,
    LOWPOWERTX,
    RX,
    TX
} States_t;

States_t state;
DS18X20 dallas( GPIO0 ); 
bool sense_t_success=false;
bool sense_b_success=false;

bool isDebug()
{
    bool debug = digitalRead(USER_KEY)==0;
    if (debug)
        Serial.printf( "debug mode ON\n");
    return debug;
}

void onSleepNormal()
{
    pinMode(USER_KEY, INPUT);
    onSleep( isDebug() ? TIME_UNTIL_WAKEUP_TEST : TIME_UNTIL_WAKEUP_NORMAL);
}

void flash(uint32_t color,uint32_t time)
{
    pinMode(USER_KEY, INPUT);
    if (isDebug())
    {    
        turnOnRGB(color, time);
        turnOffRGB();
    }
}

void setup() {
    boardInitMcu( );
    Serial.begin(115200);
    delay(500);
    
    // turnOnRGB(COLOR_START, 500);
    // turnOffRGB();    
    
    Serial.printf( "Started\n");
    state=TX;
    TimerInit( &wakeUp, onWakeUp );

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, _bandwidth,
                                _spreadingFactor, _codingRate,
                                LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD, LORA_IQ_INVERSION_ON, 36000 );

    // setting the sync work breaks the transmission.
    // Radio.SetSyncWord(SYNCWORD); 
    
    // Initialize frequency counters for Schmitt trigger inputs
    initFrequencyCounters();
    
    digitalWrite(Vext,HIGH); //POWER OFF
    
}

void SendPacket(float volts) 
{
    float temp = 0.0f;
    float adc =  0.0f;
    float vcc =  0.0f;

    uint16_t adc1l=0;
    uint16_t vccl=0;

    digitalWrite(Vext,LOW); // POWER ON
    delay(100);             // stabilise

    // Start frequency counting for Schmitt trigger inputs
    // Reset counters and start counting
    timer1_count = 0;
    timer2_count = 0;
    timer1_overflow = false;
    timer2_overflow = false;
    
    // Enable timer interrupts for counting
    attachInterrupt(digitalPinToInterrupt(SCHMITT_TRIGGER_PIN_1), timer1ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(SCHMITT_TRIGGER_PIN_2), timer2ISR, RISING);
    
    // Count for specified period
    delay(TIMER_COUNT_PERIOD_MS);
    
    // Disable interrupts and calculate frequencies
    detachInterrupt(digitalPinToInterrupt(SCHMITT_TRIGGER_PIN_1));
    detachInterrupt(digitalPinToInterrupt(SCHMITT_TRIGGER_PIN_2));
    
    calculateFrequencies();
    
    // Estimate soil moisture based on frequency difference
    // Higher frequency = drier soil, lower frequency = wetter soil
    float moisture_level_1 = 0.0f;
    if (air_reference_frequency > 0 && soil_moisture_frequency > 0) {
        // Calculate moisture as percentage based on frequency ratio
        // This is a simplified calculation - adjust based on your sensor characteristics
        float frequency_ratio = soil_moisture_frequency / air_reference_frequency;
        moisture_level_1 = (1.0f - frequency_ratio) * 100.0f; // Convert to percentage
        moisture_level_1 = constrain(moisture_level_1, 0.0f, 100.0f); // Clamp to 0-100%
        sense_m_success = true;
    } else {
        sense_m_success = false;
    }
    
    sense_t_success = dallas.search();
    if (!sense_t_success)
    {
        Serial.printf( "Temp FAIL\n");
    }
    else
    {
        temp = dallas.getTemp();
        sense_t_success = (temp>-200);
    }

    digitalWrite(Vext,HIGH); //POWER OFF

    CayenneLPP lpp(64);
    lpp.reset();
    uint64_t id = getID();
    uint32_t l = id & 0x0000FFFF;
    uint32_t h = (id >> 16 ) & 0x0000FFFF;
    
    Serial.printf("ID = %lx\n", id);
    Serial.printf("ID = (%lu)\n", id);
    Serial.printf("ID = (%u)\n", l);
    Serial.printf("ID = (%u)\n", h);
    lpp.addGenericSensor(CH_ID_LO,l);     // id of this sensor
    lpp.addGenericSensor(CH_ID_HI,h);     // id of this sensor
    
    lpp.addGenericSensor(CH_Moist1, moisture_level_1);
    
    // Add frequency data for debugging/calibration
    lpp.addGenericSensor(CH_Moist2, soil_moisture_frequency);
    lpp.addGenericSensor(CH_Moist3, air_reference_frequency);

    if (sense_t_success)
        lpp.addTemperature(CH_GndTemp,temp);

    lpp.addVoltage(CH_VoltsS, volts);

    Serial.printf( "vr ");
    Serial.print( vcc);
    Serial.printf( " Vs " );
    Serial.print( volts);
    Serial.printf( " moist ");
    Serial.print( moisture_level_1);
    Serial.printf( " temp ");
    Serial.print( temp);
    Serial.printf( " freq1 ");
    Serial.print( soil_moisture_frequency);
    Serial.printf( " freq2 ");
    Serial.print( air_reference_frequency);
    Serial.printf( " Tx Size=%d .. ", lpp.getSize());

    Radio.Send( lpp.getBuffer(), lpp.getSize() );    
}

void loop() 
{
    uint16_t volts;
    switch(state)
	{
		case TX:
            // Check battery voltage ok first
            volts = getBatteryVoltage();
            Serial.printf( "Battery %d\n", volts);
            if (volts < BATTLOW)
            {
                onSleep(TIME_UNTIL_WAKEUP_LOWPOWER);
            }
            else
            {
                SendPacket(volts/1000.0);

                if ( sense_m_success && sense_t_success )
                {                    
                    flash(COLOR_SENDING, COLOR_DURATION);
                }
                else
                {
                    flash(COLOR_FAIL_SENSOR, COLOR_DURATION);
                }
                state= LOWPOWERTX;
            }
		    break;
		case LOWPOWERTX:
            // wait for TX to complete
            Radio.IrqProcess( );
		    break;
		case LOWPOWER:
			lowPowerHandler();
		    break;
        default:
            break;
	}
}

void OnTxDone( void )
{
    Serial.printf( " TX OK\n");
    flash(COLOR_SENT, COLOR_DURATION);
    onSleepNormal();
}

void OnTxTimeout( void )
{
    Serial.printf( " TX Failed\n");
    flash(COLOR_FAIL_TX, COLOR_DURATION);
    onSleepNormal();
}

void OnRxDone( unsigned char* buf, unsigned short a, short b, signed char c)
{
    Serial.printf( "OnRxDone\n");
	state=TX;
}

void onSleep(uint32_t duration)
{
    state = LOWPOWER;
    Radio.Sleep();
    TimerSetValue( &wakeUp, duration );
    TimerStart( &wakeUp );
}

void onWakeUp()
{
    state = TX;
}

// Initialize frequency counters for Schmitt trigger inputs
void initFrequencyCounters()
{
    // Configure GPIO pins as inputs with pull-up resistors
    pinMode(SCHMITT_TRIGGER_PIN_1, INPUT_PULLUP);
    pinMode(SCHMITT_TRIGGER_PIN_2, INPUT_PULLUP);
    
    Serial.printf("Frequency counters initialized\n");
}

// Calculate frequencies from timer counts
void calculateFrequencies()
{
    // Calculate frequency in Hz based on counts over the measurement period
    // Frequency = (counts / measurement_period_ms) * 1000
    soil_moisture_frequency = (float)timer1_count * 1000.0f / TIMER_COUNT_PERIOD_MS;
    air_reference_frequency = (float)timer2_count * 1000.0f / TIMER_COUNT_PERIOD_MS;
    
    Serial.printf("Timer1 count: %lu, Timer2 count: %lu\n", timer1_count, timer2_count);
    Serial.printf("Soil freq: %.2f Hz, Air freq: %.2f Hz\n", soil_moisture_frequency, air_reference_frequency);
}

// Interrupt service routine for timer 1 (soil moisture Schmitt trigger)
void timer1ISR()
{
    timer1_count++;
    
    // Check for overflow (if count exceeds reasonable limit)
    if (timer1_count > 200000) { // Max ~200kHz for 1 second measurement
        timer1_overflow = true;
    }
}

// Interrupt service routine for timer 2 (air reference Schmitt trigger)
void timer2ISR()
{
    timer2_count++;
    
    // Check for overflow (if count exceeds reasonable limit)
    if (timer2_count > 200000) { // Max ~200kHz for 1 second measurement
        timer2_overflow = true;
    }
}