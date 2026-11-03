/*
 * Capacitive Soil Moisture Sensor with 1-wire Communication
 * ATTINY85 Firmware
 * 
 * Hardware:
 * - Pin 2 (PB3): 1-wire data line to CubeCell
 * - Pin 3 (PB4): Frequency input from 74HC14 oscillator
 * - Internal 8MHz oscillator
 * - 3.3V supply
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

// Pin definitions
#define ONE_WIRE_PIN PB3
#define SOIL_FREQ_PIN PB4      // Primary soil sensor frequency
#define AIR_FREQ_PIN PB0       // Air reference frequency
#define SENSOR_SELECT_PIN PB1  // Sensor selection (0=soil, 1=air)

// 1-wire timing constants (microseconds)
#define ONE_WIRE_RESET_PULSE 480
#define ONE_WIRE_RESET_WAIT 70
#define ONE_WIRE_RESET_TIMEOUT 480
#define ONE_WIRE_WRITE_1_LOW 6
#define ONE_WIRE_WRITE_1_HIGH 64
#define ONE_WIRE_WRITE_0_LOW 60
#define ONE_WIRE_WRITE_0_HIGH 10
#define ONE_WIRE_READ_LOW 6
#define ONE_WIRE_READ_WAIT 9
#define ONE_WIRE_READ_TIMEOUT 70

// Global variables
volatile uint16_t soil_frequency = 0;
volatile uint16_t air_frequency = 0;
volatile uint8_t measurement_ready = 0;
volatile uint8_t one_wire_busy = 0;
volatile uint8_t current_sensor = 0; // 0=soil, 1=air

// Function prototypes
void init_timer1(void);
void init_pins(void);
void start_frequency_measurement(void);
void measure_soil_sensor(void);
void measure_air_reference(void);
uint16_t get_compensated_frequency(void);
uint8_t calculate_moisture_percentage(uint16_t soil_freq, uint16_t air_freq);
uint8_t one_wire_reset(void);
void one_wire_write_bit(uint8_t bit);
uint8_t one_wire_read_bit(void);
void one_wire_write_byte(uint8_t data);
uint8_t one_wire_read_byte(void);
void send_sensor_data(void);
uint8_t calculate_checksum(uint8_t* data, uint8_t length);

// Timer1 overflow interrupt for frequency measurement
ISR(TIMER1_OVF_vect) {
    // Timer overflow - measurement complete
    measurement_ready = 1;
}

// External interrupt for soil sensor frequency (INT0 on PB4)
ISR(INT0_vect) {
    if (current_sensor == 0) { // Soil sensor
        soil_frequency = TCNT1 + (frequency_count * 65536);
        TCNT1 = 0;
        frequency_count = 0;
    }
}

// External interrupt for air reference frequency (INT1 on PB0)
ISR(INT1_vect) {
    if (current_sensor == 1) { // Air reference
        air_frequency = TCNT1 + (frequency_count * 65536);
        TCNT1 = 0;
        frequency_count = 0;
    }
}

void init_timer1(void) {
    // Configure Timer1 for frequency measurement
    TCCR1 = 0; // Stop timer
    TCNT1 = 0; // Reset counter
    TIMSK |= (1 << TOIE1); // Enable overflow interrupt
    TCCR1 |= (1 << CS10); // Start timer with prescaler 1 (8MHz/1 = 8MHz)
}

void init_pins(void) {
    // Configure pins
    DDRB &= ~(1 << SOIL_FREQ_PIN);  // PB4 as input for soil frequency
    DDRB &= ~(1 << AIR_FREQ_PIN);   // PB0 as input for air frequency
    DDRB |= (1 << SENSOR_SELECT_PIN); // PB1 as output for sensor selection
    DDRB |= (1 << ONE_WIRE_PIN);    // PB3 as output for 1-wire
    
    // Enable external interrupts
    GIMSK |= (1 << INT0); // Enable INT0 (PB4 - soil sensor)
    GIMSK |= (1 << INT1); // Enable INT1 (PB0 - air reference)
    MCUCR |= (1 << ISC00); // Trigger on any change (INT0)
    MCUCR |= (1 << ISC10); // Trigger on any change (INT1)
    
    // Set initial states
    PORTB |= (1 << ONE_WIRE_PIN); // 1-wire line high (idle state)
    PORTB &= ~(1 << SENSOR_SELECT_PIN); // Start with soil sensor selected
}

void measure_soil_sensor(void) {
    current_sensor = 0; // Select soil sensor
    PORTB &= ~(1 << SENSOR_SELECT_PIN); // Enable soil sensor oscillator
    _delay_ms(100); // Allow oscillator to stabilize
    
    // Reset counters
    TCNT1 = 0;
    frequency_count = 0;
    measurement_ready = 0;
    
    // Start timer
    TCCR1 |= (1 << CS10);
    
    // Wait for measurement period (1 second)
    _delay_ms(1000);
    
    // Stop timer
    TCCR1 &= ~(1 << CS10);
    
    // Disable soil sensor
    PORTB |= (1 << SENSOR_SELECT_PIN);
}

void measure_air_reference(void) {
    current_sensor = 1; // Select air reference
    PORTB &= ~(1 << SENSOR_SELECT_PIN); // Enable air reference oscillator
    _delay_ms(100); // Allow oscillator to stabilize
    
    // Reset counters
    TCNT1 = 0;
    frequency_count = 0;
    measurement_ready = 0;
    
    // Start timer
    TCCR1 |= (1 << CS10);
    
    // Wait for measurement period (1 second)
    _delay_ms(1000);
    
    // Stop timer
    TCCR1 &= ~(1 << CS10);
    
    // Disable air reference
    PORTB |= (1 << SENSOR_SELECT_PIN);
}

uint16_t get_compensated_frequency(void) {
    // Calculate temperature-compensated frequency
    // Air reference provides baseline for temperature compensation
    if (air_frequency > 0) {
        // Compensate for temperature drift
        // Higher air frequency = higher temperature
        uint16_t temp_factor = air_frequency / 1000; // Normalize air frequency
        return soil_frequency - (temp_factor * 10); // Apply compensation
    }
    return soil_frequency; // Fallback to raw soil frequency
}

uint8_t calculate_moisture_percentage(uint16_t soil_freq, uint16_t air_freq) {
    // Calculate moisture percentage using both sensors
    // This provides better accuracy and temperature compensation
    
    if (air_freq == 0) {
        // Fallback to simple soil frequency measurement
        if (soil_freq > 200) return 0;      // Very dry
        else if (soil_freq > 150) return 25; // Dry
        else if (soil_freq > 100) return 50; // Moist
        else if (soil_freq > 50) return 75;  // Wet
        else return 100;                     // Very wet
    }
    
    // Temperature-compensated calculation
    uint16_t compensated_freq = get_compensated_frequency();
    
    // Calibrated moisture calculation
    // These values should be calibrated for specific soil types
    if (compensated_freq > 180) return 0;      // Very dry
    else if (compensated_freq > 130) return 25; // Dry
    else if (compensated_freq > 80) return 50;  // Moist
    else if (compensated_freq > 40) return 75;  // Wet
    else return 100;                            // Very wet
}

uint8_t one_wire_reset(void) {
    uint8_t presence = 0;
    
    // Pull line low for reset pulse
    DDRB |= (1 << ONE_WIRE_PIN);
    PORTB &= ~(1 << ONE_WIRE_PIN);
    _delay_us(ONE_WIRE_RESET_PULSE);
    
    // Release line and wait
    DDRB &= ~(1 << ONE_WIRE_PIN);
    _delay_us(ONE_WIRE_RESET_WAIT);
    
    // Check for presence pulse
    if (!(PINB & (1 << ONE_WIRE_PIN))) {
        presence = 1;
    }
    
    // Wait for end of reset timeout
    _delay_us(ONE_WIRE_RESET_TIMEOUT - ONE_WIRE_RESET_WAIT);
    
    return presence;
}

void one_wire_write_bit(uint8_t bit) {
    // Pull line low
    DDRB |= (1 << ONE_WIRE_PIN);
    PORTB &= ~(1 << ONE_WIRE_PIN);
    
    if (bit) {
        // Write '1': short low pulse
        _delay_us(ONE_WIRE_WRITE_1_LOW);
        DDRB &= ~(1 << ONE_WIRE_PIN); // Release line
        _delay_us(ONE_WIRE_WRITE_1_HIGH);
    } else {
        // Write '0': long low pulse
        _delay_us(ONE_WIRE_WRITE_0_LOW);
        DDRB &= ~(1 << ONE_WIRE_PIN); // Release line
        _delay_us(ONE_WIRE_WRITE_0_HIGH);
    }
}

uint8_t one_wire_read_bit(void) {
    uint8_t bit = 0;
    
    // Pull line low
    DDRB |= (1 << ONE_WIRE_PIN);
    PORTB &= ~(1 << ONE_WIRE_PIN);
    _delay_us(ONE_WIRE_READ_LOW);
    
    // Release line and sample
    DDRB &= ~(1 << ONE_WIRE_PIN);
    _delay_us(ONE_WIRE_READ_WAIT);
    
    if (PINB & (1 << ONE_WIRE_PIN)) {
        bit = 1;
    }
    
    _delay_us(ONE_WIRE_READ_TIMEOUT - ONE_WIRE_READ_WAIT);
    
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

uint8_t calculate_checksum(uint8_t* data, uint8_t length) {
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

void send_sensor_data(void) {
    if (one_wire_busy) return;
    
    one_wire_busy = 1;
    
    // Wait for CubeCell to be ready
    if (!one_wire_reset()) {
        one_wire_busy = 0;
        return; // No device present
    }
    
    // Prepare sensor data packet
    uint8_t packet[8];
    
    // Packet format: [START][SOIL_FREQ_H][SOIL_FREQ_L][AIR_FREQ_H][AIR_FREQ_L][MOISTURE][BATTERY][CHECKSUM]
    packet[0] = 0xAA; // Start byte
    packet[1] = (soil_frequency >> 8) & 0xFF; // Soil frequency high byte
    packet[2] = soil_frequency & 0xFF; // Soil frequency low byte
    packet[3] = (air_frequency >> 8) & 0xFF; // Air frequency high byte
    packet[4] = air_frequency & 0xFF; // Air frequency low byte
    
    // Calculate moisture percentage with temperature compensation
    uint8_t moisture = calculate_moisture_percentage(soil_frequency, air_frequency);
    packet[5] = moisture;
    packet[6] = 0x80; // Battery level (simplified)
    
    // Calculate checksum
    packet[7] = calculate_checksum(packet, 7);
    
    // Send packet
    for (uint8_t i = 0; i < 8; i++) {
        one_wire_write_byte(packet[i]);
    }
    
    one_wire_busy = 0;
}

int main(void) {
    // Initialize system
    init_pins();
    init_timer1();
    
    // Enable global interrupts
    sei();
    
    // Main loop
    while (1) {
        // Measure soil sensor
        measure_soil_sensor();
        
        // Measure air reference
        measure_air_reference();
        
        // Send data to CubeCell
        send_sensor_data();
        
        // Wait before next measurement cycle
        _delay_ms(30000); // 30 seconds
    }
    
    return 0;
}
