# Airframe Design - Farm Surveillance Drone

## Design Philosophy
The airframe is designed for maximum endurance and efficiency while maintaining structural integrity and payload capacity. The design prioritizes:
- High aspect ratio wing for efficient cruise flight
- Lightweight composite construction
- Modular design for easy maintenance
- Weather resistance for outdoor operations

## Wing Design

### Wing Geometry
- **Wingspan**: 3.8 meters
- **Chord**: 0.4 meters (root), 0.25 meters (tip)
- **Aspect Ratio**: 9.5:1
- **Sweep**: 2° forward sweep for stability
- **Dihedral**: 3° for lateral stability
- **Winglets**: 0.15m height, 15° cant angle

### Airfoil Selection
- **Primary Airfoil**: Selig S1223 (high lift, low drag)
- **Thickness**: 12% at root, 8% at tip
- **Camber**: 4.2% for low-speed efficiency
- **Reynolds Number**: 200,000-400,000 (cruise conditions)

### Wing Structure
```
Materials:
- Spar: Carbon fiber tube (20mm diameter)
- Ribs: Balsa wood with carbon fiber reinforcement
- Skin: 0.5mm carbon fiber sheet
- Leading Edge: 2mm carbon fiber rod
- Trailing Edge: 1mm carbon fiber strip
```

### Control Surfaces
- **Ailerons**: 20% of wing chord, 60% of wing span
- **Flaps**: 25% of wing chord, 40% of wing span (for takeoff/landing)
- **Spoilers**: 15% of wing chord, 30% of wing span (for landing)

## Fuselage Design

### Fuselage Geometry
- **Length**: 2.2 meters
- **Width**: 0.3 meters (maximum)
- **Height**: 0.25 meters (maximum)
- **Cross-section**: Elliptical for low drag

### Internal Layout
```
Forward Section (0-0.5m):
- Nose cone with pitot tube
- Forward battery compartment
- GPS antenna

Central Section (0.5-1.5m):
- Main avionics bay
- Camera gimbal mount
- Primary battery compartment
- Servo mounting points

Aft Section (1.5-2.2m):
- Motor mount
- ESC compartment
- Telemetry antenna
- Emergency parachute
```

### Fuselage Structure
- **Monocoque Construction**: Carbon fiber shell
- **Bulkheads**: 5mm plywood with carbon fiber reinforcement
- **Access Panels**: Removable panels for maintenance
- **Cable Routing**: Internal channels for clean wiring

## Empennage Design

### Horizontal Stabilizer
- **Span**: 1.2 meters
- **Chord**: 0.25 meters
- **Area**: 0.3 m²
- **Incidence**: 0° (adjustable)
- **Material**: Carbon fiber with foam core

### Vertical Stabilizer
- **Height**: 0.4 meters
- **Chord**: 0.3 meters
- **Area**: 0.12 m²
- **Sweep**: 20° for directional stability
- **Material**: Carbon fiber with foam core

### Control Surfaces
- **Elevator**: 30% of horizontal stabilizer chord
- **Rudder**: 25% of vertical stabilizer chord
- **Trim Tabs**: Adjustable for fine-tuning

## Landing Gear

### Design Requirements
- **Weight**: <2 kg total
- **Ground Clearance**: 0.3 meters
- **Shock Absorption**: 50mm travel
- **Retraction**: Fixed gear for simplicity

### Components
- **Main Gear**: Aluminum struts with rubber wheels (150mm diameter)
- **Nose Gear**: Steel spring with caster wheel (100mm diameter)
- **Shock Absorbers**: Rubber dampers
- **Brakes**: Friction brakes on main wheels

## Payload Integration

### Camera Mount
- **Type**: 3-axis gimbal
- **Weight Capacity**: 2 kg
- **Stabilization**: ±30° pitch, ±45° roll, 360° yaw
- **Vibration Isolation**: Rubber dampers
- **Power**: 12V, 2A continuous

### Equipment Bays
- **Primary Bay**: 200mm × 150mm × 100mm
- **Secondary Bay**: 150mm × 100mm × 80mm
- **Cable Management**: Waterproof connectors
- **Cooling**: Ventilation slots with filters

## Structural Analysis

### Load Cases
1. **Normal Flight**: 1.5g maneuvering load
2. **Gust Load**: 3.0g vertical acceleration
3. **Landing**: 2.0g vertical, 0.5g horizontal
4. **Emergency**: 5.0g ultimate load

### Safety Factors
- **Normal Loads**: 2.0 safety factor
- **Ultimate Loads**: 1.5 safety factor
- **Fatigue**: 10^6 cycle endurance

### Materials Properties
```
Carbon Fiber (T300):
- Tensile Strength: 3,500 MPa
- Compressive Strength: 1,400 MPa
- Modulus: 230 GPa
- Density: 1.76 g/cm³

Balsa Wood:
- Tensile Strength: 30 MPa
- Compressive Strength: 15 MPa
- Modulus: 4 GPa
- Density: 0.16 g/cm³
```

## Manufacturing Process

### Wing Construction
1. **Mold Preparation**: CNC-machined aluminum molds
2. **Layup**: Carbon fiber prepreg with vacuum bagging
3. **Cure**: 120°C for 2 hours in autoclave
4. **Assembly**: Bonding with structural adhesive
5. **Quality Control**: Ultrasonic inspection

### Fuselage Construction
1. **Mandrel**: Expandable foam mandrel
2. **Layup**: Carbon fiber with wet layup
3. **Cure**: Room temperature cure
4. **Machining**: CNC trimming and drilling
5. **Assembly**: Bonding and riveting

## Weight Breakdown
- **Wing**: 3.5 kg
- **Fuselage**: 2.8 kg
- **Empennage**: 0.8 kg
- **Landing Gear**: 1.2 kg
- **Control Surfaces**: 0.7 kg
- **Total Airframe**: 9.0 kg

## Performance Predictions
- **Stall Speed**: 28 km/h
- **Cruise Speed**: 50 km/h
- **Maximum Speed**: 80 km/h
- **Climb Rate**: 3 m/s
- **Glide Ratio**: 18:1
- **Endurance**: 10+ hours (with solar power)

## Maintenance Requirements
- **Pre-flight**: Visual inspection, control surface check
- **Weekly**: Battery check, cable inspection
- **Monthly**: Structural inspection, servo calibration
- **Annually**: Complete disassembly and rebuild
