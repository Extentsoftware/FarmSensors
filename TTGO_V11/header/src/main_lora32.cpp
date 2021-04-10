#ifdef TTGO_LORA32_V1

#include <Arduino.h>
#include <OneWire.h>
#include <stdio.h>
#include <stdlib.h>
#include <LoRa.h>         // https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_bt.h>
#include <driver/rtc_io.h>

#include "CayenneLPP.h"
#include "AT85ADC.h"
#include "vesoil.h"

#define S_to_uS_FACTOR          1000000  /* Conversion factor for micro seconds to seconds */
#define FREQUENCY               868E6
#define BAND                    125E3   
#define SPREAD                  12   
#define CODERATE                6
#define SYNCWORD                0xa5a5
#define PREAMBLE                8
#define TXPOWER                 20   // max transmit power
#define SCK                     5    // GPIO5  -- SX1278's SCK
#define MISO                    19   // GPIO19 -- SX1278's MISnO
#define MOSI                    27   // GPIO27 -- SX1278's MOSI
#define SS                      18   // GPIO18 -- SX1278's CS
#define RST                     14   // GPIO14 -- SX1278's RESET
#define DI0                     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BUFFER_SIZE             30   // Define the payload size here
#define TIME_UNTIL_WAKEUP       15
#define BATTERY_PIN             35   // battery level measurement pin, here is the voltage divider connected
#define BUSPWR                  23    // sensor bus power control
#define BLUELED                 2
#define ONE_WIRE_BUS            25   // used for Dallas temp sensor

void OnTxDone( void );
void OnTxTimeout( void );
void onRadioSleep();
void deepSleep(uint64_t timetosleep);

typedef enum
{
    LOWPOWER,
    LOWPOWERTX,
    IN_RX,
    IN_TX
} States_t;

States_t state;


AT85ADC ds( ONE_WIRE_BUS );

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    SPI.begin(SCK,MISO,MOSI,SS);
    LoRa.setPins(SS,RST,DI0);
    int result = LoRa.begin(FREQUENCY);
    if (!result) 
        Serial.printf("Starting LoRa failed: err %d\n", result);
    else
        Serial.println("Started LoRa OK");

    LoRa.setPreambleLength(PREAMBLE);
    LoRa.setSyncWord(SYNCWORD);    
    LoRa.setSignalBandwidth(BAND);
    LoRa.setSpreadingFactor(SPREAD);
    LoRa.setCodingRate4(CODERATE);
    LoRa.enableCrc();
    LoRa.setTxPower(TXPOWER);
    LoRa.idle();
    LoRa.onTxDone(OnTxDone);

    Serial.printf( "Started\n");

    state=IN_TX;
    pinMode(BLUELED, OUTPUT);    
    pinMode(BUSPWR, OUTPUT);
    pinMode(BATTERY_PIN, INPUT);
    
    ds.search();
}

bool SendTestPacket() {

    digitalWrite(BUSPWR,  HIGH);  // SENSE On
    digitalWrite(BLUELED, HIGH);  // LED On
    delay(200);                   // stabalise
    
    bool success = ds.search();
    
    if (!success)
    {
        Serial.printf( "Tmp FAIL\n");
        digitalWrite(BUSPWR,  LOW);  // SENSE On
        return false;
    }
    
    //uint16_t temp = ds.performTemp();
    uint16_t temp = ds.performAdc(AT85_TEMP);
    uint16_t adc =  ds.performAdc(AT85_ADC3);
    uint16_t frq =  ds.performFreq();
    digitalWrite(BUSPWR,  LOW);  // SENSE Off

    uint16_t v = analogRead(BATTERY_PIN);
    float volts = (float)(v) / 4095*2*3.3*1.1;
    Serial.printf( "Tmp %d ADC=%d Frq=%d V=%f v=%d\n", temp, adc, frq, volts, v);

    CayenneLPP lpp(64);
    lpp.reset();
    //lpp.addPresence(CH_ID_LO,getID() & 0x0000FFFF);     // id of this sensor
    //lpp.addPresence(CH_ID_HI,(getID() >> 16 ) & 0x0000FFFF);     // id of this sensor
    lpp.addAnalogInput(CH_Moist1,100.0);
    lpp.addAnalogInput(CH_Volts, volts);
    
    LoRa.beginPacket();
    LoRa.write( lpp.getBuffer(), lpp.getSize());
    LoRa.endPacket(true);

    return true;
}

void loop() 
{
    switch(state)
	{
		case IN_TX:
		    if ( SendTestPacket())
            {
                state= LOWPOWERTX;
            }
            else
            {
                deepSleep(TIME_UNTIL_WAKEUP);
            }
		    break;
		case LOWPOWERTX:
            // wait for TX to complete
		    break;
		case LOWPOWER:
			deepSleep(TIME_UNTIL_WAKEUP);
		    break;
        default:
            break;
	}
}

void OnTxDone( void )
{
    Serial.printf( "Tx done\n");
    state = LOWPOWER;
}

void OnTxTimeout( void )
{
    Serial.printf( "Tx Timeout\n");
    state = LOWPOWER;
}

void OnRxDone( unsigned char* buf, unsigned short a, short b, signed char c)
{
    Serial.printf( "Rx Done\n");
	state=IN_TX;
}

void deepSleep(uint64_t timetosleep)
{
    // turn Off RTC
    LoRa.sleep();
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);

    // turnOffWifi
    esp_wifi_deinit();

    // turnOffBluetooth
    esp_bluedroid_disable();
    esp_bluedroid_deinit();

    esp_bt_controller_disable();
    esp_bt_controller_deinit();

    esp_err_t result;
    do
    {
        uint64_t us = timetosleep * S_to_uS_FACTOR;
        result = esp_sleep_enable_timer_wakeup(us);
        if (result == ESP_ERR_INVALID_ARG)
        {
            if (timetosleep > 60)
                timetosleep = timetosleep - 60;
            else if (timetosleep == 10)
                return; // avoid infinite loop
            else
                timetosleep = 10;
        }
    } while (result == ESP_ERR_INVALID_ARG);

    esp_deep_sleep_start();
}

#endif