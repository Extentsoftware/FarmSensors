# Assembly Instructions for Soil Moisture Sensor

## Required Tools

### Soldering Equipment
- Soldering iron (30-40W, fine tip)
- Solder wire (0.6mm, lead-free)
- Flux (optional, for better joints)
- Solder wick (for desoldering mistakes)
- Helping hands or PCB holder

### Programming Tools
- USBasp programmer (for ATTINY85)
- Arduino IDE (for CubeCell)
- USB cable (for CubeCell programming)

### Testing Equipment
- Multimeter
- Oscilloscope (optional)
- Power supply (3.3V, 1A)

## Step-by-Step Assembly

### Step 1: Prepare the PCB
1. **Inspect the PCB** for any defects or damage
2. **Clean the PCB** with isopropyl alcohol
3. **Identify component locations** using the silkscreen labels

### Step 2: Solder Passive Components
1. **Resistors** (R1, R2):
   - R1: 10kΩ (brown-black-orange-gold)
   - R2: 10kΩ (brown-black-orange-gold)
   - Bend leads to fit PCB holes
   - Solder and trim excess leads

2. **Capacitors** (C1, C3, C4):
   - C1: 100nF ceramic capacitor
   - C3: 100nF ceramic capacitor  
   - C4: 100nF ceramic capacitor
   - Note: Ceramic capacitors are not polarized

3. **Electrolytic Capacitors** (C2, C5):
   - C2: 100µF electrolytic capacitor
   - C5: 100µF electrolytic capacitor
   - **Important**: Observe polarity (negative lead is shorter)
   - Solder with correct orientation

### Step 3: Solder ICs
1. **74HC14 (U1)**:
   - Insert into DIP-14 socket (recommended) or solder directly
   - Align pin 1 with the dot on the PCB
   - Solder all pins carefully
   - Check for solder bridges

2. **ATTINY85 (U2)**:
   - Insert into DIP-8 socket (recommended) or solder directly
   - Align pin 1 with the dot on the PCB
   - Solder all pins carefully
   - Check for solder bridges

### Step 4: Install Voltage Regulator
1. **AMS1117-3.3V (U3)**:
   - Align with PCB markings
   - Solder all pins
   - Attach small heatsink if available

### Step 5: Install CubeCell Module
1. **CubeCell Module (U4)**:
   - This requires reflow soldering or hot air station
   - Apply solder paste to pads
   - Place module carefully
   - Heat with hot air gun (300°C) or reflow oven
   - **Alternative**: Use breakout board with headers

### Step 6: Install Connectors
1. **Power Connector (J1)**:
   - 2-pin header for power input
   - Solder securely

2. **Programming Header (J2)**:
   - 6-pin ISP header for ATTINY85 programming
   - Solder all pins

3. **Antenna Connector (J3)**:
   - Optional SMA connector for external antenna
   - Solder if using external antenna

### Step 7: Sensor Area Preparation
1. **Clean sensor area** thoroughly
2. **Apply conformal coating** to protect from moisture
3. **Avoid coating** the actual sensor traces
4. **Test sensor area** for continuity

## Programming and Testing

### Step 8: Program ATTINY85
1. **Connect USBasp programmer** to J2 header
2. **Compile firmware**:
   ```bash
   make clean
   make
   ```
3. **Program microcontroller**:
   ```bash
   make program
   ```
4. **Set fuse bits**:
   ```bash
   make fuses
   ```

### Step 9: Program CubeCell
1. **Connect CubeCell** to computer via USB
2. **Open Arduino IDE** with CubeCell board support
3. **Upload firmware** from `cubecell_lora_code.ino`
4. **Configure LoRaWAN parameters** in the code

### Step 10: Initial Testing
1. **Power on the system** with 3.3V supply
2. **Check voltages**:
   - VCC: 3.3V ±0.1V
   - Ground: 0V
3. **Test oscillator**:
   - Measure frequency at 74HC14 output
   - Should be 1-10 kHz depending on soil moisture
4. **Test 1-wire communication**:
   - Use oscilloscope to verify data transmission
5. **Test LoRa transmission**:
   - Use LoRaWAN gateway to receive data

## Calibration Procedure

### Step 11: Sensor Calibration
1. **Dry soil test**:
   - Place sensor in completely dry soil
   - Record frequency reading
   - This should be the highest frequency

2. **Wet soil test**:
   - Place sensor in saturated soil
   - Record frequency reading
   - This should be the lowest frequency

3. **Update calibration**:
   - Modify constants in ATTINY85 firmware
   - Recompile and reprogram
   - Test with known moisture levels

## Final Assembly

### Step 12: Enclosure (Optional)
1. **Choose waterproof enclosure** (IP65 or better)
2. **Drill holes** for sensor area and antenna
3. **Install PCB** in enclosure
4. **Seal all openings** with waterproof sealant
5. **Test waterproofing** before final installation

### Step 13: Installation
1. **Choose location** with good LoRaWAN coverage
2. **Install sensor** in soil at desired depth
3. **Connect power** (battery or solar panel)
4. **Verify operation** with gateway

## Troubleshooting

### Common Assembly Issues

#### Solder Bridges
- **Symptom**: Short circuits between pins
- **Solution**: Use solder wick to remove excess solder
- **Prevention**: Use appropriate soldering iron tip size

#### Cold Joints
- **Symptom**: Poor electrical connections
- **Solution**: Reheat joint and add fresh solder
- **Prevention**: Ensure proper temperature and flux

#### Component Damage
- **Symptom**: Components not working
- **Solution**: Replace damaged components
- **Prevention**: Handle components carefully, use ESD protection

#### Programming Issues
- **Symptom**: Cannot program microcontroller
- **Solution**: Check connections, verify programmer
- **Prevention**: Use quality programming cables

### Testing Issues

#### No Power
- **Check**: Power supply voltage and connections
- **Verify**: Regulator output voltage
- **Test**: Continuity of power traces

#### No Oscillator Output
- **Check**: 74HC14 power and connections
- **Verify**: RC component values
- **Test**: Input signal to 74HC14

#### No Data Transmission
- **Check**: 1-wire connection between ATTINY85 and CubeCell
- **Verify**: LoRaWAN network configuration
- **Test**: Antenna connection

## Safety Notes

### Electrical Safety
- **Disconnect power** before making changes
- **Use appropriate voltage** (3.3V maximum)
- **Check polarity** before connecting power
- **Avoid short circuits**

### Soldering Safety
- **Work in well-ventilated area**
- **Use appropriate temperature** (300-350°C)
- **Avoid breathing solder fumes**
- **Wear safety glasses**

### Installation Safety
- **Follow local regulations** for outdoor installations
- **Use appropriate enclosures** for weather protection
- **Secure all connections** to prevent damage
- **Test thoroughly** before final installation

## Maintenance

### Regular Checks
- **Inspect sensor area** for contamination
- **Check power supply** voltage
- **Verify data transmission**
- **Clean sensor area** if needed

### Long-term Maintenance
- **Replace battery** as needed
- **Update firmware** for improvements
- **Recalibrate sensor** if readings drift
- **Check enclosure** for weather damage

## Support

If you encounter issues during assembly:
1. **Check this guide** for common solutions
2. **Review the schematic** for correct connections
3. **Test each section** individually
4. **Contact support** with specific error details
