# KiCad v9 Project Files for Soil Moisture Sensor

## Project Structure
```
soil_moisture_sensor/
├── soil_moisture_sensor.kicad_pro    # Main project file
├── soil_moisture_sensor.kicad_sch    # Schematic file
├── soil_moisture_sensor.kicad_pcb    # PCB layout file
├── soil_moisture_sensor-cache.lib    # Symbol cache
├── soil_moisture_sensor.kicad_prl    # Project local settings
└── fp-lib-table                      # Footprint library table
```

## Key Features

### Schematic Design
- **74HC14 Schmitt Trigger**: Hex inverter with RC oscillator
- **ATTINY85 Microcontroller**: 8-pin DIP for sensor control
- **Power Supply**: 3.3V regulated supply with AMS1117
- **Sensor Interface**: Dual electrode measurement
- **1-wire Communication**: CubeCell LoRa module interface

### PCB Layout
- **Size**: 50mm x 35mm (2-layer)
- **Primary Sensor**: 25mm x 15mm interdigitated electrodes
- **Air Reference**: 10mm x 10mm temperature compensation
- **Guard Ring**: 2mm wide trace connected to oscillator output
- **Soldermask**: Applied over primary sensor only

### Component Placement
- **U1**: 74HC14 (DIP-14) - Center left
- **U2**: ATTINY85 (DIP-8) - Bottom left  
- **U3**: AMS1117-3.3V (SOT-223) - Top left
- **U4**: CubeCell Module (SMD) - Bottom right
- **J1**: Power Connector (2-pin) - Top left
- **J2**: Programming Header (6-pin) - Bottom left

### Net Assignments
- **VCC**: 3.3V power rail
- **GND**: Ground plane
- **SOIL_FREQ**: Soil sensor frequency input
- **AIR_FREQ**: Air reference frequency input
- **ONE_WIRE**: 1-wire communication line
- **SENSOR_SEL**: Sensor selection control

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

## Design Notes

### Interdigitated Electrodes
- **Finger Width**: 0.5mm
- **Finger Spacing**: 0.5mm  
- **Finger Length**: 10mm (primary), 5mm (reference)
- **Soldermask**: Applied over primary sensor only

### Guard Ring Design
- **Width**: 2mm
- **Connection**: Connected to oscillator output (not ground)
- **Purpose**: Reduces parasitic capacitance and fringe effects

### Temperature Compensation
- **Air Reference**: Separate electrode pair in air
- **No Soldermask**: Exposed copper for air reference
- **Compensation**: Software-based temperature drift correction

## Manufacturing Considerations

### PCB Specifications
- **Layers**: 2 (Top and Bottom)
- **Thickness**: 1.6mm
- **Copper Weight**: 1oz (35μm)
- **Soldermask**: Green
- **Silkscreen**: White
- **Surface Finish**: HASL or ENIG

### Assembly Notes
- **CubeCell Module**: Requires reflow soldering
- **Other Components**: Can be hand-soldered
- **Sensor Area**: Apply conformal coating after assembly
- **Programming**: Use 6-pin ISP header for ATTINY85

## Testing and Calibration

### Initial Testing
1. **Power Supply**: Verify 3.3V output
2. **Oscillator**: Check frequency output (1-10 kHz)
3. **Communication**: Test 1-wire protocol
4. **LoRa**: Verify transmission

### Calibration
1. **Dry Soil**: Measure frequency in completely dry soil
2. **Wet Soil**: Measure frequency in saturated soil
3. **Air Reference**: Measure baseline frequency
4. **Update Firmware**: Adjust calibration constants

## Troubleshooting

### Common Issues
- **No Oscillation**: Check RC component values
- **Poor Sensitivity**: Verify electrode design
- **Temperature Drift**: Check air reference measurement
- **Communication Errors**: Verify 1-wire connections

### Design Improvements
- **Add LED Indicators**: Status and power indicators
- **Battery Monitoring**: Voltage level sensing
- **Sleep Modes**: Power optimization
- **Enclosure**: Weatherproof housing design
