/*
 * CubeCell LoRa Receiver for Soil Moisture Sensor
 * Receives 1-wire data from ATTINY85 and transmits via LoRa
 * 
 * Hardware:
 * - CubeCell (RA-01/RA-02) module
 * - 1-wire communication with ATTINY85
 * - LoRa transmission to gateway
 */

#include "LoRaWan_APP.h"
#include "Arduino.h"

// LoRaWAN settings
#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             14        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz, 1: 250 kHz, 2: 500 kHz, 3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

// 1-wire settings
#define ONE_WIRE_PIN                                GPIO1     // Pin for 1-wire communication
#define ONE_WIRE_RESET_PULSE                        480       // microseconds
#define ONE_WIRE_RESET_WAIT                         70        // microseconds
#define ONE_WIRE_RESET_TIMEOUT                      480       // microseconds
#define ONE_WIRE_WRITE_1_LOW                        6         // microseconds
#define ONE_WIRE_WRITE_1_HIGH                       64        // microseconds
#define ONE_WIRE_WRITE_0_LOW                        60        // microseconds
#define ONE_WIRE_WRITE_0_HIGH                       10        // microseconds
#define ONE_WIRE_READ_LOW                           6         // microseconds
#define ONE_WIRE_READ_WAIT                          9         // microseconds
#define ONE_WIRE_READ_TIMEOUT                       70        // microseconds

// Data structures
struct SensorData {
    uint8_t start_byte;
    uint16_t soil_frequency;
    uint16_t air_frequency;
    uint8_t moisture_percent;
    uint8_t battery_level;
    uint8_t checksum;
};

// Global variables
SensorData sensor_data;
bool data_received = false;
uint32_t last_transmission = 0;
const uint32_t TRANSMISSION_INTERVAL = 30000; // 30 seconds

// Function prototypes
void init_one_wire(void);
bool one_wire_reset(void);
void one_wire_write_bit(uint8_t bit);
uint8_t one_wire_read_bit(void);
void one_wire_write_byte(uint8_t data);
uint8_t one_wire_read_byte(void);
bool receive_sensor_data(void);
void send_lora_data(void);
uint8_t calculate_checksum(uint8_t* data, uint8_t length);
void delayMicroseconds(uint32_t us);

void setup() {
    Serial.begin(115200);
    Serial.println("Soil Moisture Sensor - CubeCell LoRa");
    
    // Initialize 1-wire communication
    init_one_wire();
    
    // Initialize LoRa
    LoRaWAN.begin(LORAWAN_CLASS, LORAWAN_REGION);
    LoRaWAN.setDataRate(DR_3, EU868);
    LoRaWAN.setChannel(0, 868100000, 0, 2);
    LoRaWAN.setChannel(1, 868300000, 0, 2);
    LoRaWAN.setChannel(2, 868500000, 0, 2);
    LoRaWAN.setChannel(3, 867100000, 0, 2);
    LoRaWAN.setChannel(4, 867300000, 0, 2);
    LoRaWAN.setChannel(5, 867500000, 0, 2);
    LoRaWAN.setChannel(6, 867700000, 0, 2);
    LoRaWAN.setChannel(7, 867900000, 0, 2);
    LoRaWAN.setChannel(8, 868800000, 0, 2);
    
    // Set device class
    LoRaWAN.setClass(CLASS_A);
    
    // Set adaptive data rate
    LoRaWAN.setAdaptiveDataRate(false);
    
    // Set duty cycle
    LoRaWAN.setDutyCycle(false);
    
    // Set join method
    LoRaWAN.setJoinMethod(LWOTAA);
    
    // Set device address
    LoRaWAN.setDevAddr(0x12345678);
    
    // Set application session key
    LoRaWAN.setAppSKey("2B7E151628AED2A6ABF7158809CF4F3C");
    
    // Set network session key
    LoRaWAN.setNwkSKey("2B7E151628AED2A6ABF7158809CF4F3C");
    
    // Set device EUI
    LoRaWAN.setDevEUI("0000000000000001");
    
    // Set application EUI
    LoRaWAN.setAppEUI("0000000000000001");
    
    // Set application key
    LoRaWAN.setAppKey("2B7E151628AED2A6ABF7158809CF4F3C");
    
    // Join network
    LoRaWAN.join();
    
    Serial.println("Initialization complete");
}

void loop() {
    // Try to receive data from ATTINY85
    if (receive_sensor_data()) {
        data_received = true;
        Serial.print("Received data - Soil: ");
        Serial.print(sensor_data.soil_frequency);
        Serial.print(" Hz, Air: ");
        Serial.print(sensor_data.air_frequency);
        Serial.print(" Hz, Moisture: ");
        Serial.print(sensor_data.moisture_percent);
        Serial.println("%");
    }
    
    // Send data via LoRa if we have new data and enough time has passed
    if (data_received && (millis() - last_transmission > TRANSMISSION_INTERVAL)) {
        send_lora_data();
        data_received = false;
        last_transmission = millis();
    }
    
    // Handle LoRaWAN events
    LoRaWAN.loop();
    
    delay(100);
}

void init_one_wire(void) {
    pinMode(ONE_WIRE_PIN, OUTPUT);
    digitalWrite(ONE_WIRE_PIN, HIGH); // Idle state
}

bool one_wire_reset(void) {
    bool presence = false;
    
    // Pull line low for reset pulse
    pinMode(ONE_WIRE_PIN, OUTPUT);
    digitalWrite(ONE_WIRE_PIN, LOW);
    delayMicroseconds(ONE_WIRE_RESET_PULSE);
    
    // Release line and wait
    pinMode(ONE_WIRE_PIN, INPUT);
    delayMicroseconds(ONE_WIRE_RESET_WAIT);
    
    // Check for presence pulse
    if (digitalRead(ONE_WIRE_PIN) == LOW) {
        presence = true;
    }
    
    // Wait for end of reset timeout
    delayMicroseconds(ONE_WIRE_RESET_TIMEOUT - ONE_WIRE_RESET_WAIT);
    
    return presence;
}

void one_wire_write_bit(uint8_t bit) {
    // Pull line low
    pinMode(ONE_WIRE_PIN, OUTPUT);
    digitalWrite(ONE_WIRE_PIN, LOW);
    
    if (bit) {
        // Write '1': short low pulse
        delayMicroseconds(ONE_WIRE_WRITE_1_LOW);
        pinMode(ONE_WIRE_PIN, INPUT); // Release line
        delayMicroseconds(ONE_WIRE_WRITE_1_HIGH);
    } else {
        // Write '0': long low pulse
        delayMicroseconds(ONE_WIRE_WRITE_0_LOW);
        pinMode(ONE_WIRE_PIN, INPUT); // Release line
        delayMicroseconds(ONE_WIRE_WRITE_0_HIGH);
    }
}

uint8_t one_wire_read_bit(void) {
    uint8_t bit = 0;
    
    // Pull line low
    pinMode(ONE_WIRE_PIN, OUTPUT);
    digitalWrite(ONE_WIRE_PIN, LOW);
    delayMicroseconds(ONE_WIRE_READ_LOW);
    
    // Release line and sample
    pinMode(ONE_WIRE_PIN, INPUT);
    delayMicroseconds(ONE_WIRE_READ_WAIT);
    
    if (digitalRead(ONE_WIRE_PIN) == HIGH) {
        bit = 1;
    }
    
    delayMicroseconds(ONE_WIRE_READ_TIMEOUT - ONE_WIRE_READ_WAIT);
    
    return bit;
}

void one_wire_write_byte(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        one_wire_write_bit(data & 0x01);
        data >>= 1;
    }
}

uint8_t one_wire_read_byte(void) {
    uint8_t data = 0;
    
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;
        if (one_wire_read_bit()) {
            data |= 0x80;
        }
    }
    
    return data;
}

bool receive_sensor_data(void) {
    // Check if ATTINY85 is present
    if (!one_wire_reset()) {
        return false; // No device present
    }
    
    // Read data packet (8 bytes for dual sensor)
    uint8_t packet[8];
    for (uint8_t i = 0; i < 8; i++) {
        packet[i] = one_wire_read_byte();
    }
    
    // Verify start byte
    if (packet[0] != 0xAA) {
        return false;
    }
    
    // Verify checksum
    if (packet[7] != calculate_checksum(packet, 7)) {
        return false;
    }
    
    // Parse data
    sensor_data.start_byte = packet[0];
    sensor_data.soil_frequency = (packet[1] << 8) | packet[2];
    sensor_data.air_frequency = (packet[3] << 8) | packet[4];
    sensor_data.moisture_percent = packet[5];
    sensor_data.battery_level = packet[6];
    sensor_data.checksum = packet[7];
    
    return true;
}

void send_lora_data(void) {
    // Prepare LoRa payload (8 bytes for dual sensor data)
    uint8_t payload[8];
    payload[0] = (sensor_data.soil_frequency >> 8) & 0xFF;
    payload[1] = sensor_data.soil_frequency & 0xFF;
    payload[2] = (sensor_data.air_frequency >> 8) & 0xFF;
    payload[3] = sensor_data.air_frequency & 0xFF;
    payload[4] = sensor_data.moisture_percent;
    payload[5] = sensor_data.battery_level;
    payload[6] = 0x01; // Sensor type (soil moisture)
    payload[7] = 0x00; // Reserved
    
    // Send via LoRaWAN
    LoRaWAN.send(8, payload, 2, false);
    
    Serial.print("Data sent via LoRa - Soil: ");
    Serial.print(sensor_data.soil_frequency);
    Serial.print(" Hz, Air: ");
    Serial.print(sensor_data.air_frequency);
    Serial.print(" Hz, Moisture: ");
    Serial.print(sensor_data.moisture_percent);
    Serial.println("%");
}

uint8_t calculate_checksum(uint8_t* data, uint8_t length) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

void delayMicroseconds(uint32_t us) {
    // Simple delay implementation
    // Note: This is not precise, but sufficient for 1-wire timing
    for (uint32_t i = 0; i < us; i++) {
        __asm__ __volatile__ ("nop");
    }
}
