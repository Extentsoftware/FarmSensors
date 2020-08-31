// https://github.com/HelTecAutomation/ASR650x-Arduino 
// https://heltec-automation-docs.readthedocs.io/en/latest/cubecell/capsule-sensor/htcc-ac01/sensor_pinout_diagram.html




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

#define timetillsleep 5000
#define timetillwakeup 5000
static TimerEvent_t wakeUp;

static RadioEvents_t RadioEvents;
void OnTxDone( void );
void OnTxTimeout( void );
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );
void onWakeUp();
void onSleep();

typedef enum
{
    LOWPOWER,
    RX,
    TX,
    TXWAIT
} States_t;

int16_t txNumber;
States_t state;
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
    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, _bandwidth,
                                _spreadingFactor, _codingRate,
                                LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD, LORA_IQ_INVERSION_ON, 16000 );

    // setting the sync work breaks the transmission.
    // Radio.SetSyncWord(SYNCWORD);
    state=TX;

    TimerInit( &wakeUp, onWakeUp );
    
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
    //Serial.printf("TX begin %d bytes ....", sizeof(report));
    Radio.Send( (uint8_t *)&report, sizeof(report) );    
}

void SendTestPacket() {
    txNumber++;

    uint8_t buffer[24];
    memset( buffer, 0, sizeof(buffer));

    int voltage = getBatteryVoltage();
    sprintf( (char *)buffer, "B %u S %d C %d V %d", _bandwidth, _spreadingFactor, _codingRate, voltage);
    Radio.Send( buffer, sizeof(buffer) );
}

void loop() {

    switch(state)
	{
		case TX:
            SendTestPacket();
		    state=TXWAIT;
		    break;
		case TXWAIT:
            break;
		case LOWPOWER:
			lowPowerHandler();
		    break;
        default:
            break;
	}
    //Radio.IrqProcess( );
}

void OnTxDone( void )
{
	onSleep();
}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    state=TX;
}

void OnRxDone( unsigned char* buf, unsigned short a, short b, signed char c)
{
	state=TX;
}

void onSleep()
{
    Radio.Sleep();
    turnOnRGB(0x500000,0);
    turnOffRGB();
    state = LOWPOWER;
    TimerSetValue( &wakeUp, timetillwakeup );
    TimerStart( &wakeUp );
}

void onWakeUp()
{
    state = TX;
}