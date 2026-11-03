# KiCad v9 Soil Moisture Sensor Project

## Project Overview
Complete KiCad v9 project for a capacitive soil moisture sensor with interdigitated electrodes, temperature compensation, and LoRa transmission.

## Files
- `soil_moisture_sensor.kicad_pro` - Main project file
- `soil_moisture_sensor.kicad_sch` - Schematic file  
- `soil_moisture_sensor.kicad_pcb` - PCB layout file

## Key Features
- **Interdigitated Electrodes**: Advanced capacitive sensing with soldermask protection
- **Temperature Compensation**: Air reference electrode for accurate measurements
- **Guard Ring Design**: Bootstrap guard ring reduces parasitic capacitance
- **74HC14 Oscillator**: Schmitt trigger RC oscillator for frequency measurement
- **ATTINY85 Microcontroller**: Dual sensor reading and 1-wire communication
- **CubeCell LoRa**: Wireless data transmission module

## PCB Specifications
- **Size**: 50mm x 35mm
- **Layers**: 2 (Top and Bottom)
- **Thickness**: 1.6mm
- **Copper Weight**: 1oz (35μm)
- **Soldermask**: Green
- **Silkscreen**: White
- **Surface Finish**: HASL or ENIG

## Electrode Design
- **Primary Sensor**: 25mm x 15mm interdigitated electrodes
- **Air Reference**: 10mm x 10mm temperature compensation
- **Finger Width**: 0.5mm
- **Finger Spacing**: 0.5mm
- **Guard Ring**: 2mm wide, connected to oscillator output

## Usage Instructions

### 1. Open in KiCad v9
1. Launch KiCad v9
2. Open `soil_moisture_sensor.kicad_pro`
3. The project will load with schematic and PCB files

### 2. View Schematic
1. Click "Open Schematic" or double-click `.kicad_sch` file
2. View complete circuit design
3. Check net connections and component values

### 3. View PCB Layout
1. Click "Open PCB" or double-click `.kicad_pcb` file
2. View 2D/3D PCB layout
3. Check component placement and routing

### 4. Generate Manufacturing Files
1. In PCB Editor: File → Fabrication Outputs → Gerbers
2. Select layers: F.Cu, B.Cu, F.Mask, B.Mask, F.SilkS, B.SilkS, Edge.Cuts
3. Generate drill files and pick-and-place files

### 5. Order PCB
1. Use generated Gerber files
2. Specify: 2-layer, 1.6mm thickness, 1oz copper
3. Include soldermask and silkscreen

## Component List
- **U1**: 74HC14 (DIP-14) - Schmitt trigger oscillator
- **U2**: ATTINY85-20PU (DIP-8) - Microcontroller
- **U3**: AMS1117-3.3V (SOT-223) - Voltage regulator
- **U4**: CubeCell Module (SMD) - LoRa transmitter
- **R1**: 10kΩ (0603) - Timing resistor
- **C1**: 100nF (0603) - Decoupling capacitor
- **J1**: Power Connector (2-pin) - Power input
- **J2**: Programming Header (6-pin) - ISP programming

## Net Assignments
- **VCC**: 3.3V power rail
- **GND**: Ground plane
- **SOIL_FREQ**: Soil sensor frequency input
- **AIR_FREQ**: Air reference frequency input
- **ONE_WIRE**: 1-wire communication line
- **SENSOR_SEL**: Sensor selection control

## Manufacturing Notes
- **Assembly**: CubeCell requires reflow soldering
- **Protection**: Apply conformal coating over sensor area
- **Programming**: Use 6-pin ISP header for ATTINY85
- **Testing**: Verify oscillator frequency and 1-wire communication

## Calibration
1. **Dry Soil**: Measure frequency in completely dry soil
2. **Wet Soil**: Measure frequency in saturated soil
3. **Air Reference**: Measure baseline frequency
4. **Update Firmware**: Adjust calibration constants

## Support
For technical support and questions:
- Check the main project documentation
- Review the assembly instructions
- Contact the maintainers

## License
This project is open source. See LICENSE file for details.

