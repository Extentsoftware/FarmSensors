#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include "vesoil.h"
#include "ringbuffer.h"

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
#define BUFFER_SIZE                                 60 // Define the payload size here
#define SYNCWORD                                    0

#define BATTLOW                                     3000
#define COLOR_RED                                   0x800000   //color red
#define COLOR_GREEN                                 0x008000   //color green
#define COLOR_WHITE                                 0x808080   //color white
#define COLOR_BLUE                                  0x000080   //color blue

#define COLOR_TXFAIL                                COLOR_RED
#define COLOR_SENDING                               COLOR_GREEN
#define COLOR_RX                                    COLOR_WHITE
#define COLOR_DUPLICATE                             COLOR_BLUE
#define COLOR_DURATION                              300

uint8_t txpacket[BUFFER_SIZE];
unsigned short txLen=0;
bool sending=false;
RingBuffer ringBuffer(16);

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

void setup() {
    boardInitMcu( );
    Serial.begin(115200);
    Serial.printf( "Repeater Started\n");
    turnOnRGB(COLOR_RED, COLOR_DURATION);
    turnOnRGB(COLOR_GREEN, COLOR_DURATION);
    turnOnRGB(COLOR_BLUE, COLOR_DURATION);
    turnOffRGB();    

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, _bandwidth,
                                _spreadingFactor, _codingRate,
                                LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD, LORA_IQ_INVERSION_ON, 36000 );
    
    Radio.SetRxConfig( MODEM_LORA, _bandwidth,
                       _spreadingFactor, _codingRate, 0,
                       LORA_PREAMBLE_LENGTH, 0, LORA_FIX_LENGTH_PAYLOAD_ON, 0, 
                       LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD, LORA_IQ_INVERSION_ON, true );
    Radio.Sleep();
}

void loop() 
{
    Radio.IrqProcess();
    if (sending)
    {
        
    }
    else
    {
        if (txLen==0)
        {
            Radio.Rx( 0 );
        }
        else
        {
            Serial.printf( "OnTx Begin\n");
            turnOnRGB(COLOR_SENDING, 0);
            Radio.Send( txpacket, txLen );    
            txLen = 0;
            sending = true;
        }
    }
}

void OnTxTimeout( void )
{
    Serial.printf( "OnTx Timeout\n");
    sending = false;
    turnOnRGB(COLOR_TXFAIL, COLOR_DURATION);
}

void OnTxDone( void )
{
    Serial.printf( "OnTx Done\n");
    sending = false;
    turnOffRGB();
}

int HashCode (uint8_t* buffer, int size) {
    int h = 0;
    for (uint8_t* ptr = buffer; size>0; --size, ++ptr)
        h = h * 31 + *ptr;
    return h;
}
void OnRxDone( unsigned char* buf, unsigned short a, short b, signed char c)
{
    if (txLen==0 && !sending)
    {        
        Serial.printf( "OnRx Begin %d bytes\n", a);
        int hash = HashCode(buf, a);
        turnOnRGB(COLOR_RX, 0);            
        if (ringBuffer.exists(hash))
        {
            turnOnRGB(COLOR_DUPLICATE, COLOR_DURATION);
            Serial.printf("Duplicate hash %d\n", hash); 
            return;
        }

        memcpy( txpacket, buf, a);    
        txLen=a;
    }
 }
