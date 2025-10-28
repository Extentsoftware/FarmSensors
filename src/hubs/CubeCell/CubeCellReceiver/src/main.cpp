#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <Wire.h>
#include "CubeCell_NeoPixel.h"
#include "LoRaWan_APP.h"
#include <CayenneLPP.h>
#include "vesoil.h"

// Constants
#define RF_FREQUENCY                                868E6           // Hz
#define LORA_PREAMBLE_LENGTH                        8               // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0               // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false
#define LORA_CRC                                    true
#define LORA_FREQ_HOP                               false
#define LORA_HOP_PERIOD                             0
#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 30              // Define the payload size here
#define SYNCWORD                                    0
#define COLOR_DURATION                              30
#define CHANNEL                                     5
#define MAX_I2C_RETRIES                             3
#define I2C_RETRY_DELAY_MS                         100

// Radio configuration
uint32_t _bandwidth                                 = 0;            // [0: 125 kHz,  4 = LORA_BW_041
uint32_t _spreadingFactor                          = 12;           // SF_12 [SF7..SF12]
uint8_t  _codingRate                                = 1;            // LORA_CR_4_5; [1: 4/5]

// Global variables
uint8_t rxpacket[BUFFER_SIZE];
static RadioEvents_t RadioEvents;
static bool radioInitialized = false;

// Function declarations
void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
bool initializeRadio();
bool setupWire();
void flash(uint32_t color, uint32_t time);
bool isDebug();

bool isDebug() {
    bool debug = digitalRead(USER_KEY) == 0;
    if (debug) {
        Serial.printf("Debug mode ON\n");
    }
    return debug;
}

bool setupWire() {
    for (int i = 0; i < MAX_I2C_RETRIES; i++) {
        Serial.printf("Attempting I2C initialization (attempt %d/%d)\n", i + 1, MAX_I2C_RETRIES);
        
        Wire.end(); // Ensure clean state
        delay(I2C_RETRY_DELAY_MS);
        
        if (Wire.begin()) {
            Serial.printf("I2C started successfully, clock: %d Hz\n", Wire.getClock());
            return true;
        }
        
        Serial.printf("I2C initialization failed, retrying...\n");
        delay(I2C_RETRY_DELAY_MS);
    }
    
    Serial.printf("I2C initialization failed after %d attempts\n", MAX_I2C_RETRIES);
    return false;
}

void flash(uint32_t color, uint32_t time) {
    pinMode(USER_KEY, INPUT);
    if (isDebug()) {    
        turnOnRGB(color, time);
        turnOffRGB();
    }
}

bool initializeRadio() {
    if (radioInitialized) {
        return true;
    }

    RadioEvents.RxDone = OnRxDone;
    
    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetRxConfig(MODEM_LORA, _bandwidth,
                     _spreadingFactor, _codingRate, 0,
                     LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT,
                     LORA_FIX_LENGTH_PAYLOAD_ON, 0,
                     LORA_CRC, LORA_FREQ_HOP, LORA_HOP_PERIOD,
                     LORA_IQ_INVERSION_ON, 1);
    
    Radio.Rx(0);
    
    // Verify radio is in receive mode
    if (Radio.GetStatus() != RF_RX_RUNNING) {
        Serial.printf("Radio failed to enter receive mode\n");
        return false;
    }
    
    radioInitialized = true;
    return true;
}

void setup() {
    boardInitMcu();
    Serial.begin(115200);
    delay(500);
    
    Serial.printf("System initialization started\n");
    
    if (!initializeRadio()) {
        Serial.printf("Radio initialization failed, system may not function correctly\n");
    }
    
    if (!setupWire()) {
        Serial.printf("I2C initialization failed, system may not function correctly\n");
    }
    
    Serial.printf("System initialization completed\n");
}

void loop() {
    // Add periodic health check if needed
    delay(100);
}

void OnRxDone(uint8_t* payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (size > BUFFER_SIZE) {
        Serial.printf("Error: Received packet size (%d) exceeds buffer size (%d)\n", size, BUFFER_SIZE);
        return;
    }
    
    Serial.printf("Received packet - RSSI: %d dBm, SNR: %d dB, Length: %d bytes\n", rssi, snr, size);
    
    // Clear buffer before copying new data
    memset(rxpacket, 0, BUFFER_SIZE);
    memcpy(rxpacket, payload, size);
    
    // Debug output
    if (isDebug()) {
        Serial.printf("Packet contents: ");
        for (int i = 0; i < size; i++) {
            Serial.printf("%02X ", rxpacket[i]);
        }
        Serial.printf("\n");
    }
    
    // Visual feedback
    turnOnRGB(COLOR_RECEIVED, 100);
    turnOffRGB();
    
    // I2C transmission with retry logic
    bool transmissionSuccess = false;
    for (int attempt = 0; attempt < MAX_I2C_RETRIES && !transmissionSuccess; attempt++) {
        Wire.beginTransmission(CHANNEL);
        size_t bytesWritten = Wire.write(rxpacket, size);
        
        if (bytesWritten != size) {
            Serial.printf("I2C write error: wrote %d of %d bytes\n", bytesWritten, size);
            continue;
        }
        
        uint8_t err = Wire.endTransmission();
        if (err == 0) {
            transmissionSuccess = true;
            Serial.printf("I2C transmission successful\n");
        } else {
            Serial.printf("I2C transmission failed (attempt %d/%d): %s\n", 
                         attempt + 1, MAX_I2C_RETRIES, Wire.getErrorText(err));
            delay(I2C_RETRY_DELAY_MS);
            
            if (err == 5) { // I2C timeout
                HW_Reset(0);
                // setupWire(); // Reinitialize I2C
            }
        }
    }
    
    if (!transmissionSuccess) {
        Serial.printf("Failed to transmit data after %d attempts\n", MAX_I2C_RETRIES);
    }
    
    // Restart radio reception
    Radio.Rx(0);
}

