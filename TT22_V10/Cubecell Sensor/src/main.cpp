#include <Arduino.h>
#include "vesoil.h"
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"

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

#define COLOR_SENT 0x505000   //color red, light 0x10

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

typedef enum
{
    LOWPOWER,
    RX,
    TX
}States_t;

int16_t txNumber;
States_t state;
bool sleepMode = false;
int16_t Rssi,rxSize;
//CubeCell_NeoPixel pixels(1, RGB, NEO_GRB + NEO_KHZ800);

void setup() {
    boardInitMcu( );
    Serial.begin(115200);

    txNumber=0;
    Rssi=0;

    RadioEvents.TxDone = OnTxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxDone = OnRxDone;

    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    state=TX;
    
    pinMode(Vext,OUTPUT);
    digitalWrite(Vext,LOW); //SET POWER

}

void Send() {
    SensorReport report;
    memset( &report, 0, sizeof(report));
    memcpy( &report.id , "CUBECL", 6);
    report.distance.value = 10.5;
    report.gndTemp.value = 20.5;
    report.moist1.value = 1000;
    report.moist2.value = 2000;
    Serial.printf("TX begin %d bytes ....", sizeof(report));
    Radio.Send( (uint8_t *)&report, sizeof(report) );    
}

void loop() {
    switch(state)
	{
		case TX:
            
            Serial.printf("band %u spread %d coderate %d", _bandwidth, _spreadingFactor, _codingRate);
            Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, _bandwidth,
                                        _spreadingFactor, _codingRate,
                                        LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                        LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD, LORA_IQ_INVERSION_ON, 16000 );

            // setting the sync work breaks the transmission.
            //Radio.SetSyncWord(SYNCWORD);

			delay(1000);
            turnOnRGB(COLOR_SEND,0);
			txNumber++;
            Send();
            turnOnRGB(COLOR_SENT,0);
		    state=LOWPOWER;

            // ++ _codingRate;
            // if (_codingRate>LORA_CR_4_6)
            // {
            //     _codingRate = LORA_CR_4_5;
            //     //++ _spreadingFactor;
            //     //if (_spreadingFactor>LORA_SF12)
            //     //{
            //     //     _spreadingFactor = LORA_SF5;
                    //   ++ _bandwidth;
                    //   if (_bandwidth>4)
                    //   {
                    //       _bandwidth = 0;
                    //   }
                 //}
            // }

		    break;
		case RX:
			Serial.println("into RX mode");
		    Radio.Rx( 0 );
		    state=LOWPOWER;
		    break;
		case LOWPOWER:
			lowPowerHandler();
		    break;
        default:
            break;
	}
    Radio.IrqProcess( );
  // put your main code here, to run repeatedly:

    // for (int i=0; i<255; i+=100)
    // for (int j=0; j<255; j+=100)
    // for (int k=0; k<255; k+=100)
    // {
    //     pixels.setPixelColor(0, pixels.Color(i, j, k));

    //     pixels.show();   // Send the updated pixel colors to the hardware.
    //     uint16_t voltage;
    //     voltage=analogRead(ADC);//return the voltage in mV, max value can be read is 2400mV 
    //     Serial.print(millis());
    //     Serial.print("  ");
    //     Serial.println(voltage);
    //     delay(200);
    // }
}

void OnTxDone( void )
{
	Serial.print("TX done......\n");
	turnOffRGB();
	state=TX;
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    Serial.print("TX Timeout......\n");
    state=TX;
}

void OnRxDone( unsigned char* buf, unsigned short a, short b, signed char c)
{
	state=TX;
}
