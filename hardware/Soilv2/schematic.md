# Capacitive Soil Moisture Sensor Circuit Design

## Overview
This circuit uses a capacitive soil moisture sensor with a 74HC14 Schmitt trigger RC oscillator, ATTINY85 microcontroller, and CubeCell LoRa transmitter for wireless data transmission.

## Circuit Components

### 1. Capacitive Soil Moisture Sensor
- **Primary Sensor**: Interdigitated copper electrodes with soldermask overlay
- **Air Reference**: Separate interdigitated electrodes in air for temperature compensation
- **Guard Ring**: Copper trace surrounding sensor area, connected to oscillator output
- **Protection**: Soldermask over copper traces, waterproof epoxy over entire sensor area
- **Capacitance Range**: 5-50pF (dry soil) to 100-300pF (wet soil)
- **Temperature Compensation**: Air reference electrode provides baseline measurement

### 2. Schmitt Trigger Oscillator (74HC14)
- **IC**: 74HC14 Hex Schmitt Trigger Inverter
- **Configuration**: RC oscillator using one inverter
- **Frequency Range**: 1-10 kHz (varies with soil moisture)
- **Components**:
  - R1: 10kΩ resistor (timing resistor)
  - C1: Variable capacitor (soil sensor + fixed capacitor)
  - C2: 100nF decoupling capacitor

### 3. ATTINY85 Microcontroller
- **MCU**: ATTINY85-20PU (8-pin DIP)
- **Clock**: Internal 8MHz oscillator
- **Power**: 3.3V (compatible with CubeCell)
- **Functions**:
  - Frequency measurement from 74HC14
  - 1-wire protocol implementation
  - Data processing and calibration

### 4. CubeCell LoRa Module
- **Module**: CubeCell (RA-01/RA-02)
- **Communication**: 1-wire protocol from ATTINY85
- **Power**: 3.3V supply
- **Antenna**: Built-in or external antenna

## Circuit Connections

### Power Supply
- **Voltage**: 3.3V regulated supply
- **Current**: ~50mA total consumption
- **Regulator**: AMS1117-3.3V (if using 5V input)

### 74HC14 Oscillator Circuit
```
VCC (3.3V) ----+---- 74HC14 Pin 14 (VCC)
                |
                +---- 100nF ---- GND
                |
                +---- 10kΩ ---- Pin 1 (Input)
                |
                +---- Soil Sensor Capacitor ---- Pin 2 (Output)
                |         (Interdigitated)
                +---- Air Reference Capacitor ---- Pin 3 (Input)
                |         (Temperature Comp.)
                +---- Guard Ring ---- Pin 2 (Output)
                |         (Bootstrap)
                +---- 100nF ---- GND
```

### ATTINY85 Connections
- **Pin 1 (Reset)**: Pull-up resistor + programming header
- **Pin 2 (PB3)**: 1-wire data line to CubeCell
- **Pin 3 (PB4)**: Frequency input from 74HC14 Pin 2
- **Pin 4 (GND)**: Ground
- **Pin 5 (PB0)**: Unused (can be used for LED indicator)
- **Pin 6 (PB1)**: Unused
- **Pin 7 (PB2)**: Unused
- **Pin 8 (VCC)**: 3.3V supply

### CubeCell LoRa Module
- **VCC**: 3.3V supply
- **GND**: Ground
- **Data**: 1-wire communication line to ATTINY85 Pin 2

## PCB Layout Considerations
- **Primary Sensor Area**: 25mm x 15mm interdigitated electrodes with soldermask
- **Air Reference Area**: 10mm x 10mm interdigitated electrodes (no soldermask)
- **Guard Ring**: 2mm wide trace surrounding sensor areas, connected to oscillator output
- **Electrode Design**: 
  - Finger width: 0.5mm
  - Finger spacing: 0.5mm
  - Finger length: 10mm (primary), 5mm (reference)
- **Component Placement**: Minimize trace lengths for oscillator
- **Ground Plane**: Solid ground plane for stability (avoid under sensor areas)
- **Power Decoupling**: 100nF capacitors near each IC
- **Sensor Protection**: Soldermask over copper, waterproof epoxy over entire sensor area
