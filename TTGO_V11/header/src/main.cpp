// https://github.com/HelTecAutomation/ASR650x-Arduino 
// https://heltec-automation-docs.readthedocs.io/en/latest/cubecell/capsule-sensor/htcc-ac01/sensor_pinout_diagram.html

#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <CayenneLPP.h>
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include "AT85ADC.h"
#include "vesoil.h"

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

#define BATTLOW                                     3000
#define COLOR_SENDING                               0x020202   //color white
#define COLOR_SENT                                  0x000200   //color green
#define COLOR_FAIL_SENSOR                           0x020000   
#define COLOR_FAIL_TX                               0x020002 
#define COLOR_DURATION                              10

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

#define MICRO_TO_SECS                               1000
#define MICRO_TO_MIN                                60 * MICRO_TO_SECS
#define FIVESECS                                    05 * MICRO_TO_SECS
#define TENSECS                                     10 * MICRO_TO_SECS
#define THIRTYSECS                                  30 * MICRO_TO_SECS
#define THIRTYMINS                                  30 * MICRO_TO_MIN
#define TIME_UNTIL_WAKEUP_NORMAL                    5000
#define TIME_UNTIL_WAKEUP_LOWPOWER                  10000

static TimerEvent_t wakeUp;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void onWakeUp();
void onSleep(uint32_t duration);

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

bool SendTestPacket(float volts) 
{
    digitalWrite(Vext,LOW); //POWER ON
    delay(100);             // stabalise

    float temp = 0;
    float adc =  0;
    //uint16_t frq =  0;
    
    bool success = ds.search();

    if (!success)
    {
        Serial.printf( "Tmp FAIL\n");
        return false;
    }
    else
    {
        temp = ds.performAdc(AT85_TEMP) / 10.0;
        adc =  ds.performAdc(AT85_ADC3) / 1.0;
        //frq =  ds.performFreq();
    }

    digitalWrite(Vext,HIGH); //POWER OFF

    CayenneLPP lpp(32);
    lpp.reset();
    lpp.addPresence(CH_ID_LO,getID() & 0x0000FFFF);     // id of this sensor
    lpp.addPresence(CH_ID_HI,(getID() >> 16 ) & 0x0000FFFF);     // id of this sensor
    lpp.addGenericSensor(CH_Moist1, adc);
    lpp.addTemperature(CH_GndTemp,temp);
    uint8_t cursor = lpp.addVoltage(CH_Volts, volts);

    Serial.printf( "ATTINY 1-Wire success\n");
    Serial.println( cursor );
    Serial.println( adc,4 );
    Serial.println( temp,4 );
    Serial.println( volts,4 );
    //Serial.printf( "Tmp= %g ADC= %g Batt= %g \n", temp, adc, volts);

    Radio.Send( lpp.getBuffer(), lpp.getSize() );    
    return true;
    
}

void loop() 
{
    uint16_t volts;
    switch(state)
	{
		case TX:
            // Check battery voltage ok first
            volts = getBatteryVoltage();
            if (volts < BATTLOW)
            {
                onSleep(TIME_UNTIL_WAKEUP_LOWPOWER);
            }
            else
            {
                if ( SendTestPacket(volts/1000.0))
                {
                    turnOnRGB(COLOR_SENDING, COLOR_DURATION);
                    turnOffRGB();
                    state= LOWPOWERTX;
                }
                else
                {
                    turnOnRGB(COLOR_FAIL_SENSOR, COLOR_DURATION);
                    turnOffRGB();
                    onSleep(TIME_UNTIL_WAKEUP_NORMAL);
                }
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
    Serial.printf( "OnTxDone\n");
    turnOnRGB(COLOR_SENT, COLOR_DURATION);
    turnOffRGB();
    onSleep(TIME_UNTIL_WAKEUP_NORMAL);
}

void OnTxTimeout( void )
{
    Serial.printf( "OnTxTimeout\n");
    turnOnRGB(COLOR_FAIL_TX, COLOR_DURATION);
    turnOffRGB();
    onSleep(TIME_UNTIL_WAKEUP_NORMAL);
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