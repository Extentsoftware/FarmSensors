// https://github.com/HelTecAutomation/ASR650x-Arduino 
// https://heltec-automation-docs.readthedocs.io/en/latest/cubecell/capsule-sensor/htcc-ac01/sensor_pinout_diagram.html

#include <Arduino.h>

#include <stdio.h>
#include <stdlib.h>
#include <CayenneLPP.h>
#include <Wire.h>
#include <OneWire.h>

#ifdef HAS_DHT
#include <DHT.h>
#include <DHT_U.h>
#endif

#ifdef HAS_BME280
#include <SPI.h>
#include "bme280.h"
#endif

#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include "vesoil.h"

#define RF_FREQUENCY                                868E6           // Hz
#define TX_OUTPUT_POWER                             22              // dBm
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

#define BATTLOW                                     3000
#define COLOR_START                                 0x0000FF   //color blue
#define COLOR_SENDING                               0xFFFFFF   //color white
#define COLOR_SENT                                  0x00FF00   //color green
#define COLOR_FAIL_SENSOR                           0xFF0000   // red 
#define COLOR_FAIL_TX                               0xFF00FF
#define COLOR_DURATION                              100

char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

#define SECONDS                                     1000
#define MINUTES                                     60 * SECONDS
#define HOURS                                       60 * MINUTES
#define DAYS                                        24 * HOURS
#define TIME_UNTIL_WAKEUP_TEST                      10 * MINUTES
#define TIME_UNTIL_WAKEUP_NORMAL                    30 * MINUTES
#define TIME_UNTIL_WAKEUP_LOWPOWER                  60 * MINUTES


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

#ifdef HAS_DHT
#define DHTPIN      GPIO0
#define DHTTYPE     DHT22
DHT_Unified dht(DHTPIN, DHTTYPE);
#endif

#ifdef HAS_BME280
BME280 bme;
#endif

bool sense_t_success=false;
bool sense_h_success=false;

bool isDebug()
{
    return false;
    pinMode(USER_KEY, INPUT);
    bool debug = digitalRead(USER_KEY)==0;

    Serial.printf( "debug mode %d\n", debug);
    return debug;
}

void onSleepNormal()
{    
    onSleep( isDebug() ? TIME_UNTIL_WAKEUP_TEST : TIME_UNTIL_WAKEUP_NORMAL);
}

void flash(uint32_t color,uint32_t time)
{
    if (isDebug())
    {    
        turnOnRGB(color, time);
        turnOffRGB();
    }
}

void setup() {
    boardInitMcu( );
    Serial.begin(115200);
    delay(250);
    Serial.printf( "Start\n");    

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
    digitalWrite(Vext,HIGH); //POWER OFF    
}

#ifdef HAS_DHT
void SendDhtPacket(float volts) 
{
    float temp = 0.0f;
    float hum = 0.0f;
    
    digitalWrite(Vext,LOW); // POWER ON
    delay(500);             // stabilise

    dht.begin();
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    Serial.println(F("------------------------------------"));
    Serial.println(F("Temperature Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
    Serial.println(F("------------------------------------"));
    // Print humidity sensor details.
    dht.humidity().getSensor(&sensor);
    Serial.println(F("Humidity Sensor"));
    Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
    Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
    Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
    Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
    Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
    Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
    Serial.println(F("------------------------------------"));
    // Set delay between sensor readings based on sensor details.
    int delayMS = sensor.min_delay / 1000;
    delay(delayMS);
    sensors_event_t event;
    dht.temperature().getEvent(&event);
    sense_t_success = !isnan(event.temperature);
    if (!sense_t_success)
    {
        Serial.println(F("Error reading temperature!"));
    }
    else {
        temp = event.temperature;
        hum = event.relative_humidity;
        Serial.print(F("Temperature: "));
        Serial.print(temp);
        Serial.println(F("°C"));
        sense_t_success = true;
    }
    delay(delayMS);
    dht.humidity().getEvent(&event);
    sense_h_success = !isnan(event.relative_humidity) && event.relative_humidity != 1.0;
    if (!sense_h_success)
    {
        Serial.println(F("Error reading humidity!"));
    }
    else {
        hum = event.relative_humidity;
        Serial.print(F("Humidity: "));
        Serial.print(hum);
        Serial.println("");
        sense_h_success = true;
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
    
    if (sense_t_success)
        lpp.addTemperature(CH_AirTemp,temp);

    if (sense_h_success)
        lpp.addRelativeHumidity(CH_AirHum,hum);

    lpp.addVoltage(CH_VoltsS, volts);

    Serial.printf( " Vs " );
    Serial.print( volts);
    Serial.printf( " temp ");
    Serial.print( temp);
    Serial.printf( " Tx Size=%d .. ", lpp.getSize());


    Radio.Send( lpp.getBuffer(), lpp.getSize() );    
}
#endif

#ifdef HAS_BME280
void SendBM280Packet(float volts) 
{
    unsigned status;
    digitalWrite(Vext, LOW); // POWER ON
    delay(500);             // stabilise

    Serial.println("Init sensor");
    status = bme.init();  
    Serial.println("Init sensor complete");

    // You can also pass in a Wire library object like &Wire2
    // status = bme.begin(0x76, &Wire2)
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        //Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        sense_t_success = false;
        return;
    }
    Serial.println("sensor found");
    delay(100);
    
    CayenneLPP lpp(64);
    lpp.reset();
    uint64_t id = getID();
    uint32_t l = id & 0x0000FFFF;
    uint32_t h = (id >> 16 ) & 0x0000FFFF;
    
    float temp = bme.getTemperature();
    float press = bme.getPressure() / 100.0F + 43;
    uint32 hum = bme.getHumidity();

    Wire.end();
    digitalWrite(Vext,HIGH); // POWER OFF

    lpp.addGenericSensor(CH_ID_LO,l);     // id of this sensor
    lpp.addGenericSensor(CH_ID_HI,h);     // id of this sensor
    lpp.addTemperature(CH_AirTemp, temp);
    lpp.addBarometricPressure(CH_AirPressure, press);

    Serial.printf("ID = %lx  %f v %f °C %f hPa %f %  ..",id, volts, temp, press, hum);
    sense_t_success = true;

    Radio.Send( lpp.getBuffer(), lpp.getSize() );    
}
#endif

void SendPacket(float volts) 
{
#ifdef HAS_DHT
    SendDhtPacket(volts);
#endif

#ifdef HAS_BME280
    SendBM280Packet(volts);
#endif
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
                SendPacket(volts/1000.0);

                if ( sense_t_success )
                {                    
                    flash(COLOR_SENDING, COLOR_DURATION);
                    state= LOWPOWERTX;
                }
                else
                {
                    flash(COLOR_FAIL_SENSOR, COLOR_DURATION);
                    onSleepNormal();
                    state= LOWPOWER;
                }
                
            }
		    break;
		case LOWPOWERTX:
            // wait for TX to complete
            Radio.IrqProcess( );
		    break;
		case LOWPOWER:
            Serial.printf( " lp\n");
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
    Serial.printf( " onSleep %d\n", duration);
    state = LOWPOWER;
    Radio.Sleep();
    TimerSetValue( &wakeUp, duration );
    TimerStart( &wakeUp );
}

void onWakeUp()
{
    state = TX;
}

