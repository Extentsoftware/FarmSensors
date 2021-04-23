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
#define TX_OUTPUT_POWER                             22              // dBm
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
#define COLOR_START                                 0x0000FF   //color blue
#define COLOR_SENDING                               0xFFFFFF   //color white
#define COLOR_SENT                                  0x00FF00   //color green
#define COLOR_FAIL_SENSOR                           0xFF0000   // red 
#define COLOR_FAIL_TX                               0xFF00FF
#define COLOR_DURATION                              30

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

#define SECONDS                                     1000
#define MINUTES                                     60 * SECONDS
#define HOURS                                       60 * MINUTES
#define DAYS                                        24 * HOURS
#define TIME_UNTIL_WAKEUP_TEST                      5 * SECONDS
#define TIME_UNTIL_WAKEUP_NORMAL                    3 * HOURS
#define TIME_UNTIL_WAKEUP_LOWPOWER                  1 * DAYS

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


void onSleepNormal()
{
    pinMode(USER_KEY, INPUT);
    onSleep( digitalRead(USER_KEY)==0 ? TIME_UNTIL_WAKEUP_TEST : TIME_UNTIL_WAKEUP_NORMAL);
}

void flash(uint32_t color,uint32_t time)
{
    pinMode(USER_KEY, INPUT);
    if (digitalRead(USER_KEY)==0)
    {    
        turnOnRGB(color, time);
        turnOffRGB();
    }
}

void setup() {
    boardInitMcu( );
    Serial.begin(115200);
    delay(100);
    turnOnRGB(COLOR_START, 500);
    turnOffRGB();    
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

bool SendPacket(float volts) 
{
    digitalWrite(Vext,LOW); //POWER ON
    delay(100);             // stabalise

    float temp = 0;
    float adc =  0;
    float vcc =  0;
    //uint16_t frq =  0;
    
    bool success = ds.search();

    if (!success)
    {
        Serial.printf( "Tmp FAIL\n");
    }
    else
    {
        uint16_t temp1l = ds.performTemp();
        uint16_t adc1l =  ds.performAdc(AT85_ADC3);
        uint16_t vccl =  ds.performAdc(AT85_ADC_VCC);
        temp = temp1l / 1.0;
        adc =  adc1l / 1.0;
        vcc =  1.1 + (vccl / 1024.0);
        Serial.printf( "ADC1=%d ADC2=%d ADC3=%d\n", temp1l,adc1l,vccl);
        //frq =  ds.performFreq();
    }

    digitalWrite(Vext,HIGH); //POWER OFF

    CayenneLPP lpp(32);
    lpp.reset();
    lpp.addPresence(CH_ID_LO,getID() & 0x0000FFFF);     // id of this sensor
    lpp.addPresence(CH_ID_HI,(getID() >> 16 ) & 0x0000FFFF);     // id of this sensor
    lpp.addGenericSensor(CH_Moist1, adc);
    lpp.addTemperature(CH_GndTemp,temp);
    lpp.addVoltage(CH_VoltsR,vcc);
    uint8_t cursor = lpp.addVoltage(CH_VoltsS, volts);

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
                if ( SendPacket(volts/1000.0))
                {
                    
                    flash(COLOR_SENDING, COLOR_DURATION);
                    state= LOWPOWERTX;
                }
                else
                {
                    flash(COLOR_FAIL_SENSOR, COLOR_DURATION);
                    onSleepNormal();
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
    flash(COLOR_SENT, COLOR_DURATION);
    onSleepNormal();
}

void OnTxTimeout( void )
{
    Serial.printf( "OnTxTimeout\n");
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