# Capacitive Soil Moisture Sensor with LoRa Transmission

A complete soil moisture monitoring system using capacitive sensing, Schmitt trigger oscillator, ATTINY85 microcontroller, and CubeCell LoRa module for wireless data transmission.

## Features

- **Interdigitated Capacitive Sensing**: Advanced electrode design with soldermask protection
- **Temperature Compensation**: Air reference electrode for accurate measurements
- **Guard Ring Design**: Bootstrap guard ring reduces parasitic capacitance
- **Schmitt Trigger Oscillator**: 74HC14-based RC oscillator for frequency measurement
- **ATTINY85 Microcontroller**: Low-power 8-bit MCU for dual sensor reading and 1-wire communication
- **1-wire Protocol**: Simple communication between ATTINY85 and CubeCell
- **LoRa Transmission**: Wireless data transmission using CubeCell module
- **Low Power**: Optimized for battery operation
- **Waterproof Design**: Protected sensor area for outdoor use

## Hardware Components

### Main Components
- **74HC14**: Hex Schmitt Trigger Inverter (DIP-14)
- **ATTINY85**: 8-bit Microcontroller (DIP-8)
- **CubeCell**: LoRa Module (RA-01/RA-02)
- **AMS1117-3.3V**: Voltage Regulator
- **Resistors**: 10kΩ (2x), various values
- **Capacitors**: 100nF (4x), 100µF (2x)
- **PCB**: Custom 40mm x 30mm board

### Optional Components
- **Battery**: 3.7V Li-ion or 2x AA batteries
- **Antenna**: External antenna for CubeCell (optional)
- **Enclosure**: Waterproof housing
- **Programming Header**: 6-pin ISP header

## Circuit Operation

### 1. Interdigitated Capacitive Sensing
The soil moisture sensor uses interdigitated copper electrodes with soldermask overlay. The primary sensor has 12 finger pairs (0.5mm width, 0.5mm spacing) for maximum sensitivity. An air reference sensor provides temperature compensation.

### 2. RC Oscillator
The 74HC14 Schmitt trigger forms an RC oscillator with the soil sensor as the variable capacitor. The frequency output is proportional to soil moisture content.

### 3. Dual Frequency Measurement
The ATTINY85 measures both soil sensor and air reference frequencies using Timer1 and external interrupts. The air reference provides temperature compensation, while the soil sensor measures actual moisture content.

### 4. Data Transmission
The ATTINY85 communicates with the CubeCell module using a 1-wire protocol, sending sensor data every 30 seconds.

### 5. Temperature Compensation
The air reference electrode provides a baseline measurement that compensates for temperature drift and oscillator variations. This significantly improves measurement accuracy across different environmental conditions.

### 6. LoRa Transmission
The CubeCell module receives data from the ATTINY85 and transmits it via LoRaWAN to a gateway or receiver.

## Software Architecture

### ATTINY85 Firmware
- **Dual Frequency Measurement**: Timer1-based frequency counting for both sensors
- **Temperature Compensation**: Air reference-based drift correction
- **1-wire Protocol**: Master implementation for CubeCell communication
- **Data Processing**: Compensated frequency to moisture percentage conversion
- **Power Management**: Sleep modes for battery optimization

### CubeCell Firmware
- **1-wire Protocol**: Slave implementation for ATTINY85 communication
- **LoRaWAN**: Standard LoRaWAN protocol implementation
- **Data Forwarding**: Receives sensor data and transmits via LoRa

## Installation and Setup

### 1. Hardware Assembly
1. Solder all components to the PCB according to the schematic
2. Install the CubeCell module (requires reflow soldering)
3. Apply waterproof coating to the sensor area
4. Connect power supply (3.3V regulated)

### 2. Programming ATTINY85
```bash
# Compile firmware
make clean
make

# Program microcontroller
make program

# Set fuse bits
make fuses
```

### 3. Programming CubeCell
1. Install Arduino IDE with CubeCell board support
2. Upload the provided Arduino sketch
3. Configure LoRaWAN parameters (DevEUI, AppEUI, AppKey)

### 4. Calibration
1. Measure frequency in completely dry soil
2. Measure frequency in saturated soil
3. Update calibration constants in firmware
4. Test with known moisture levels

## Usage

### Power On
1. Connect power supply (3.3V)
2. LED indicators show system status
3. Sensor begins measuring after 1 second

### Data Transmission
- Sensor data is transmitted every 30 seconds
- Data includes soil frequency, air frequency, moisture percentage, and battery level
- LoRaWAN payload format: [SOIL_FREQ_H][SOIL_FREQ_L][AIR_FREQ_H][AIR_FREQ_L][MOISTURE][BATTERY][TYPE][RESERVED]

### Monitoring
- Use LoRaWAN gateway to receive data
- Data can be forwarded to cloud platforms
- Real-time monitoring and alerting possible

## Troubleshooting

### Common Issues

#### No Data Transmission
- Check power supply voltage (3.3V)
- Verify 1-wire connection between ATTINY85 and CubeCell
- Check LoRaWAN network configuration
- Verify antenna connection

#### Inaccurate Readings
- Calibrate sensor for specific soil type
- Check sensor area for contamination
- Verify oscillator frequency range
- Update calibration constants

#### Power Issues
- Check battery voltage
- Verify regulator output
- Check for short circuits
- Optimize sleep modes

### Debugging Tools
- Serial monitor for CubeCell debugging
- Oscilloscope for frequency measurement
- Multimeter for voltage checks
- Logic analyzer for 1-wire communication

## Specifications

### Electrical
- **Supply Voltage**: 3.3V ±5%
- **Current Consumption**: ~50mA active, ~10µA sleep
- **Frequency Range**: 1-10 kHz (varies with moisture)
- **Resolution**: 0.5% moisture content
- **Accuracy**: ±2% (with temperature compensation)
- **Temperature Range**: -20°C to +60°C (compensated)

### Physical
- **PCB Size**: 50mm x 35mm (increased for better electrode design)
- **Primary Sensor Area**: 25mm x 15mm (interdigitated electrodes)
- **Air Reference Area**: 10mm x 10mm (temperature compensation)
- **Operating Temperature**: -20°C to +60°C
- **Humidity**: 0-100% RH (non-condensing)
- **Protection**: IP65 (with proper enclosure)

### LoRaWAN
- **Frequency**: 868 MHz (EU868)
- **Power**: 14 dBm
- **Range**: Up to 5 km (line of sight)
- **Data Rate**: DR3 (SF7, BW125)
- **Payload Size**: 8 bytes (dual sensor data)

## License

This project is open source. See LICENSE file for details.

## Contributing

Contributions are welcome! Please submit pull requests or open issues for bugs and feature requests.

## Support

For technical support and questions:
- Create an issue on GitHub
- Check the troubleshooting section
- Review the documentation
- Contact the maintainers

## Changelog

### Version 1.0.0
- Initial release
- Basic capacitive sensing
- 1-wire communication
- LoRaWAN transmission
- Complete documentation
