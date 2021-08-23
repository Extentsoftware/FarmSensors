// https://github.com/HelTecAutomation/ASR650x-Arduino 
// https://heltec-automation-docs.readthedocs.io/en/latest/cubecell/capsule-sensor/htcc-ac01/sensor_pinout_diagram.html

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <CayenneLPP.h>
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include "AT85ADC.h"
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
#define COLOR_DURATION                              30

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

#define SECONDS                                     1000
#define MINUTES                                     60 * SECONDS
#define HOURS                                       60 * MINUTES
#define DAYS                                        24 * HOURS
#define TIME_UNTIL_WAKEUP_TEST                      5 * SECONDS
//#define TIME_UNTIL_WAKEUP_NORMAL                    1 * HOURS
#define TIME_UNTIL_WAKEUP_NORMAL                    5 * SECONDS
#define TIME_UNTIL_WAKEUP_LOWPOWER                  60 * SECONDS // 1 * DAYS

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
DS18X20 dallas( GPIO0 ); 
bool sense_m_success=false;
bool sense_t_success=false;
bool sense_b_success=false;

bool isDebug()
{
    return digitalRead(USER_KEY)==0;
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
    delay(2000);
    //turnOnRGB(COLOR_START, 500);
    //turnOffRGB();    
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
    digitalWrite(Vext,LOW); //POWER ON
    
}

void SendPacket(float volts) 
{
    float temp = 0.0f;
    float adc =  0.0f;
    float vcc =  0.0f;

    uint16_t adc1l=0;
    uint16_t vccl=0;

    digitalWrite(Vext,LOW); // POWER ON
    delay(200);             // stabilise

    sense_m_success = ds.search();
    if (!sense_m_success)
    {
        Serial.printf( "Moist FAIL\n");
    }
    else
    {
        // vccl =  ds.performAdc(AT85_ADC_VCC);
        // vcc =  (1.1 * 1023.0) / vccl;
        // sense_b_success = true;

        adc1l =  ds.performAdc(AT85_ADC3);
        adc =  adc1l / 1.0;
        sense_m_success = adc1l < 1024;

        // F0 Search
        // 33 read rom
        // 55 match rom 
        // CC skip rom
        // EC alarm
        // 44  4E  BE 48 B8 B4

        // BE 
        // 45-53 
    }

    // sense_t_success = dallas.search();
    // if (!sense_t_success)
    // {
    //     Serial.printf( "Temp FAIL\n");
    // }
    // else
    // {
    //     temp = dallas.getTemp();
    //     sense_t_success = true;
    // }

    digitalWrite(Vext,HIGH); //POWER OFF

    CayenneLPP lpp(64);
    lpp.reset();
    lpp.addPresence(CH_ID_LO,getID() & 0x0000FFFF);     // id of this sensor
    lpp.addPresence(CH_ID_HI,(getID() >> 16 ) & 0x0000FFFF);     // id of this sensor
    
    if (sense_m_success)
        lpp.addGenericSensor(CH_Moist1, adc);

    if (sense_t_success)
        lpp.addTemperature(CH_GndTemp,temp);

    if (sense_b_success)
        lpp.addVoltage(CH_VoltsR,vcc);

    lpp.addVoltage(CH_VoltsS, volts);

    Serial.printf( "vr ");
    Serial.print( vcc);
    Serial.printf( " Vs " );
    Serial.print( volts);
    Serial.printf( " moistl ");
    Serial.print( adc1l );
    Serial.printf( " moist ");
    Serial.print( adc);
    Serial.printf( " temp ");
    Serial.print( temp);
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