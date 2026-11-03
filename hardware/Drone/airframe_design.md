# Airframe Design - Compact Farm Surveillance Drone (5kg)

## Design Philosophy
The airframe is designed for maximum endurance and efficiency while maintaining structural integrity and payload capacity at a much smaller scale. The design prioritizes:
- High aspect ratio wing for efficient cruise flight
- Lightweight composite construction
- Modular design for easy maintenance
- Weather resistance for outdoor operations
- **Target Total Weight: 5kg**

## Wing Design

### Wing Geometry
- **Wingspan**: 2.0 meters (scaled from 3.8m)
- **Chord**: 0.2 meters (root), 0.13 meters (tip)
- **Aspect Ratio**: 10:1 (improved for efficiency)
- **Sweep**: 1° forward sweep for stability
- **Dihedral**: 2° for lateral stability
- **Winglets**: 0.08m height, 15° cant angle

### Airfoil Selection
- **Primary Airfoil**: Selig S1223 (high lift, low drag)
- **Thickness**: 10% at root, 6% at tip
- **Camber**: 3.5% for low-speed efficiency
- **Reynolds Number**: 100,000-200,000 (cruise conditions)

### Wing Structure
```
Materials:
- Spar: Carbon fiber tube (8mm diameter)
- Ribs: Balsa wood with carbon fiber reinforcement
- Skin: 0.3mm carbon fiber sheet
- Leading Edge: 1mm carbon fiber rod
- Trailing Edge: 0.5mm carbon fiber strip
```

### Control Surfaces
- **Ailerons**: 18% of wing chord, 50% of wing span
- **Flaps**: 20% of wing chord, 30% of wing span (for takeoff/landing)
- **Spoilers**: 12% of wing chord, 25% of wing span (for landing)

## Fuselage Design

### Fuselage Geometry
- **Length**: 1.2 meters (scaled from 2.2m)
- **Width**: 0.15 meters (maximum)
- **Height**: 0.12 meters (maximum)
- **Cross-section**: Elliptical for low drag

### Internal Layout
```
Forward Section (0-0.3m):
- Nose cone with pitot tube
- Forward battery compartment
- GPS antenna

Central Section (0.3-0.8m):
- Main avionics bay
- Camera gimbal mount
- Primary battery compartment
- Servo mounting points

Aft Section (0.8-1.2m):
- Motor mount
- ESC compartment
- Telemetry antenna
- Emergency parachute
```

### Fuselage Structure
- **Monocoque Construction**: Carbon fiber shell
- **Bulkheads**: 3mm plywood with carbon fiber reinforcement
- **Access Panels**: Removable panels for maintenance
- **Cable Routing**: Internal channels for clean wiring

## Empennage Design

### Horizontal Stabilizer
- **Span**: 0.6 meters
- **Chord**: 0.13 meters
- **Area**: 0.08 m²
- **Incidence**: 0° (adjustable)
- **Material**: Carbon fiber with foam core

### Vertical Stabilizer
- **Height**: 0.2 meters
- **Chord**: 0.15 meters
- **Area**: 0.03 m²
- **Sweep**: 15° for directional stability
- **Material**: Carbon fiber with foam core

### Control Surfaces
- **Elevator**: 30% of horizontal stabilizer chord
- **Rudder**: 25% of vertical stabilizer chord
- **Trim Tabs**: Adjustable for fine-tuning

## Landing Gear

### Design Requirements
- **Weight**: <0.3 kg total
- **Ground Clearance**: 0.15 meters
- **Shock Absorption**: 25mm travel
- **Retraction**: Fixed gear for simplicity

### Components
- **Main Gear**: Carbon fiber struts with foam wheels (80mm diameter)
- **Nose Gear**: Carbon fiber spring with caster wheel (60mm diameter)
- **Shock Absorbers**: Rubber dampers
- **Brakes**: Friction brakes on main wheels

## Payload Integration

### Camera Mount
- **Type**: 2-axis gimbal (reduced complexity)
- **Weight Capacity**: 0.5 kg
- **Stabilization**: ±20° pitch, ±30° roll
- **Vibration Isolation**: Rubber dampers
- **Power**: 12V, 1A continuous

### Equipment Bays
- **Primary Bay**: 100mm × 80mm × 50mm
- **Secondary Bay**: 80mm × 60mm × 40mm
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
1. **Mold Preparation**: 3D-printed molds
2. **Layup**: Carbon fiber prepreg with vacuum bagging
3. **Cure**: 80°C for 1 hour in oven
4. **Assembly**: Bonding with structural adhesive
5. **Quality Control**: Visual inspection

### Fuselage Construction
1. **Mandrel**: Expandable foam mandrel
2. **Layup**: Carbon fiber with wet layup
3. **Cure**: Room temperature cure
4. **Machining**: Hand trimming and drilling
5. **Assembly**: Bonding and riveting

## Weight Breakdown
- **Wing**: 0.8 kg
- **Fuselage**: 0.6 kg
- **Empennage**: 0.2 kg
- **Landing Gear**: 0.3 kg
- **Control Surfaces**: 0.2 kg
- **Total Airframe**: 2.1 kg

## Performance Predictions
- **Stall Speed**: 25 km/h
- **Cruise Speed**: 45 km/h
- **Maximum Speed**: 70 km/h
- **Climb Rate**: 2.5 m/s
- **Glide Ratio**: 20:1 (improved due to higher aspect ratio)
- **Endurance**: 6-8 hours (with optimized power system)

## Maintenance Requirements
- **Pre-flight**: Visual inspection, control surface check
- **Weekly**: Battery check, cable inspection
- **Monthly**: Structural inspection, servo calibration
- **Annually**: Complete disassembly and rebuild

## Scaling Factors Applied
- **Linear Scale**: 0.53 (2.0m/3.8m wingspan)
- **Area Scale**: 0.28 (area scales as square of linear dimension)
- **Volume Scale**: 0.15 (volume scales as cube of linear dimension)
- **Weight Scale**: 0.23 (5kg/22kg estimated large drone airframe weight)
