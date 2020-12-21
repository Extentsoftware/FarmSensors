// https://github.com/HelTecAutomation/ASR650x-Arduino 
// https://heltec-automation-docs.readthedocs.io/en/latest/cubecell/capsule-sensor/htcc-ac01/sensor_pinout_diagram.html

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include "CayenneLPP.h"
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include "AT85ADC.h"
#include "vesoil.h"
#include <OneWire.h>

#define RF_FREQUENCY                                868E6           // Hz
#define TX_OUTPUT_POWER                             20              // dBm
uint32_t _bandwidth                                 = 0;            // [0: 125 kHz,  4 = LORA_BW_041
uint32_t  _spreadingFactor                          = LORA_SF12;        // [SF7..SF12] - 7
uint8_t  _codingRate                                = LORA_CR_4_5;     // [1: 4/5,    - 1
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

#define COLOR_SENT 0xFFFFFF   //color white
#define COLOR_FAIL 0xFF0000   //color red, light 0x10

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

#define MICRO_TO_SECS                               1000
#define MICRO_TO_MIN                                60 * MICRO_TO_SECS
#define FIVESECS                                    05 * MICRO_TO_SECS
#define TENSECS                                     10 * MICRO_TO_SECS
#define THIRTYSECS                                  30 * MICRO_TO_SECS
#define THIRTYMINS                                  30 * MICRO_TO_MIN
#define TIME_UNTIL_WAKEUP                           1000

static TimerEvent_t wakeUp;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void onWakeUp();
void onSleep();
void onRadioSleep();

typedef enum
{
    LOWPOWER,
    LOWPOWERTX,
    RX,
    TX
} States_t;

States_t state;
AT85ADC ds( GPIO0 );

void setup() {
    boardInitMcu( );
    Serial.begin(115200);
    delay(1000);
    
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
}

bool SendTestPacket() {
    digitalWrite(Vext,LOW); //POWER ON
    delay(10);
    
    bool success = ds.search();

    digitalWrite(Vext,HIGH); //POWER OFF

    if (!success)
    {
        Serial.printf( "Tmp FAIL\n");
        turnOnRGB(COLOR_FAIL, 100);
        turnOffRGB();
        return false;
    }
    uint16_t temp = ds.performConversion(false);
    uint16_t adc =  ds.performConversion(true);
    uint16_t volts = getBatteryVoltage();
    Serial.printf( "Tmp %d ADC=%d, V=%d\n", temp, adc, volts);

    CayenneLPP lpp(64);
    lpp.reset();
    lpp.addPresence(CH_ID_LO,getID() & 0x0000FFFF);     // id of this sensor
    lpp.addPresence(CH_ID_HI,(getID() >> 16 ) & 0x0000FFFF);     // id of this sensor
    lpp.addAnalogInput(CH_Moist1,100.0);
    lpp.addAnalogInput(CH_Volts, getBatteryVoltage()/1000.0);
    turnOnRGB(COLOR_SENT, 100);
    Radio.Send( lpp.getBuffer(), lpp.getSize() );    
    return true;
    
}

void loop() 
{
    switch(state)
	{
		case TX:
		    if ( SendTestPacket())
            {
                state= LOWPOWERTX;
            }
            else
            {
                onSleep();
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
    turnOffRGB();
    onRadioSleep();
}

void OnTxTimeout( void )
{
    turnOffRGB();
    onRadioSleep();
}

void OnRxDone( unsigned char* buf, unsigned short a, short b, signed char c)
{
	state=TX;
}

void onRadioSleep()
{
    Radio.Sleep();
    onSleep();
}

void onSleep()
{
    state = LOWPOWER;
    TimerSetValue( &wakeUp, TIME_UNTIL_WAKEUP );
    TimerStart( &wakeUp );
}

void onWakeUp()
{
    state = TX;
}