#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <Wire.h>
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include <CayenneLPP.h>
#include <CayenneLPPDecode.h>
#include "../../../common/vesoil.h"

#define RF_FREQUENCY                                868E6           // Hz
uint32_t _bandwidth                                 = 0;            // [0: 125 kHz,  4 = LORA_BW_041
uint32_t _spreadingFactor                          = 12;           // SF_12 [SF7..SF12] - 7
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

#define COLOR_DURATION                              30
#define CHANNEL                                     5

uint8_t rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

bool isDebug()
{
    bool debug = digitalRead(USER_KEY)==0;
    if (debug)
        Serial.printf( "debug mode ON\n");
    return debug;
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
    
    Serial.printf( "Started\n");

    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );

    Radio.SetRxConfig( MODEM_LORA, _bandwidth,
                                _spreadingFactor, _codingRate, 0,
                                LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON, 0, 
                                LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD, LORA_IQ_INVERSION_ON, true );

    Radio.Rx( 0 );
    if (Wire.begin())
        Serial.printf("I2C started, clock %d\n", Wire.getClock());
}

void loop() 
{
}

void OnRxDone( unsigned char* payload, unsigned short size, short rssi, signed char snt)
{
    Serial.printf( "OnRxDone\n");
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    for (int i=0;i<size;i++)
        Serial.printf("%02X ", rxpacket[i]);
    Serial.printf( "\n");
    
    turnOnRGB(COLOR_RECEIVED, 100);
    turnOffRGB();
    Serial.printf("\r\nreceived packet with Rssi %d , length %d\r\n", rssi, size);

    // https://forum.arduino.cc/t/send-text-string-over-i2c-between-two-arduinos/326052
    Wire.beginTransmission(CHANNEL);          // transmit to device #9
    Wire.write(rxpacket, size);
    uint8_t err = Wire.endTransmission();    // stop transmitting
    Serial.printf("I2C ERROR %d %s\n", err);
}

