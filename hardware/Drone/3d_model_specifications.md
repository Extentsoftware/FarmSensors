# 3D Model Specifications - Compact Farm Surveillance Drone

## Aircraft Overview
This document provides detailed specifications for creating a 3D model of the compact farm surveillance drone with 2.0m wingspan and 1.2m length.

## Main Dimensions

### Overall Aircraft
- **Wingspan**: 2000mm
- **Length**: 1200mm
- **Height**: 300mm (at fuselage)
- **Wing Chord**: 200mm (root), 130mm (tip)
- **Wing Area**: 0.33 m²
- **Aspect Ratio**: 10:1

## Fuselage Specifications

### Fuselage Geometry
- **Length**: 1200mm
- **Maximum Width**: 150mm
- **Maximum Height**: 120mm
- **Cross-section**: Elliptical (major axis 150mm, minor axis 120mm)

### Fuselage Sections
```
Forward Section (0-300mm):
- Nose cone: 50mm radius hemisphere
- Width: 100mm tapering to 80mm
- Height: 80mm tapering to 60mm

Central Section (300-800mm):
- Width: 150mm (constant)
- Height: 120mm (constant)
- Elliptical cross-section

Aft Section (800-1200mm):
- Width: 150mm tapering to 100mm
- Height: 120mm tapering to 80mm
- Motor mount: 100mm diameter
```

## Wing Specifications

### Wing Geometry
- **Span**: 2000mm
- **Root Chord**: 200mm
- **Tip Chord**: 130mm
- **Sweep**: 1° forward
- **Dihedral**: 2°
- **Winglets**: 80mm height, 15° cant

### Wing Airfoil
- **Type**: Selig S1223
- **Root Thickness**: 20mm (10% of chord)
- **Tip Thickness**: 13mm (10% of chord)
- **Camber**: 7mm at root, 4.5mm at tip

### Control Surfaces
- **Ailerons**: 36mm chord (18% of wing chord), 1000mm span
- **Flaps**: 40mm chord (20% of wing chord), 600mm span
- **Spoilers**: 24mm chord (12% of wing chord), 500mm span

## Empennage Specifications

### Horizontal Stabilizer
- **Span**: 600mm
- **Chord**: 130mm
- **Area**: 0.08 m²
- **Incidence**: 0°
- **Position**: 1000mm from nose

### Vertical Stabilizer
- **Height**: 200mm
- **Chord**: 150mm
- **Area**: 0.03 m²
- **Sweep**: 15°
- **Position**: 1000mm from nose

### Control Surfaces
- **Elevator**: 39mm chord (30% of stabilizer chord)
- **Rudder**: 37.5mm chord (25% of stabilizer chord)

## Landing Gear

### Main Landing Gear
- **Position**: 400mm from nose
- **Track**: 800mm
- **Wheel Diameter**: 80mm
- **Wheel Width**: 20mm
- **Strut Length**: 150mm
- **Shock Travel**: 25mm

### Nose Landing Gear
- **Position**: 100mm from nose
- **Wheel Diameter**: 60mm
- **Wheel Width**: 15mm
- **Strut Length**: 100mm
- **Caster Angle**: 15°

## Power System Layout

### Motor
- **Type**: BLDC outrunner
- **Diameter**: 80mm
- **Length**: 60mm
- **Position**: 1100mm from nose
- **Mounting**: 100mm diameter ring

### Propeller
- **Diameter**: 600mm
- **Blades**: 2-blade
- **Pitch**: 300mm
- **Material**: Carbon fiber
- **Position**: 1200mm from nose

### Battery Compartments
- **Forward Battery**: 200mm × 100mm × 60mm
- **Main Battery**: 300mm × 150mm × 80mm
- **Position**: 200-500mm from nose

## Surveillance System Layout

### Camera Gimbal
- **Type**: 2-axis brushless gimbal
- **Dimensions**: 120mm × 100mm × 80mm
- **Position**: 400mm from nose, 50mm below fuselage
- **Range**: Pan ±180°, Tilt ±60°

### Cameras
- **Primary Camera**: 80mm × 60mm × 40mm
- **Thermal Camera**: 60mm × 50mm × 30mm
- **Wide-Angle Camera**: 50mm × 40mm × 25mm

### IR Illuminators
- **Type**: LED arrays
- **Dimensions**: 100mm × 20mm × 15mm each
- **Position**: 2x on gimbal mount
- **Power**: 10W each

## Avionics Layout

### Flight Controller
- **Type**: Pixhawk 2.1
- **Dimensions**: 80mm × 50mm × 20mm
- **Position**: 300mm from nose, centerline

### GPS Module
- **Dimensions**: 60mm × 40mm × 15mm
- **Position**: 250mm from nose, centerline
- **Antenna**: 50mm diameter, 20mm height

### Telemetry Radio
- **Dimensions**: 100mm × 60mm × 25mm
- **Position**: 350mm from nose, centerline
- **Antenna**: 200mm length, 5mm diameter

## Solar Panel Layout

### Panel Specifications
- **Type**: Flexible monocrystalline
- **Dimensions**: 200mm × 100mm × 2mm each
- **Coverage**: 85% of wing upper surface
- **Total Panels**: 8 panels
- **Total Area**: 1.6 m²

### Panel Positions
- **Wing Upper Surface**: 8 panels
- **Panel 1**: 100-300mm from root, 0-200mm from leading edge
- **Panel 2**: 300-500mm from root, 0-200mm from leading edge
- **Panel 3**: 500-700mm from root, 0-200mm from leading edge
- **Panel 4**: 700-900mm from root, 0-200mm from leading edge
- **Panel 5**: 900-1100mm from root, 0-200mm from leading edge
- **Panel 6**: 1100-1300mm from root, 0-200mm from leading edge
- **Panel 7**: 1300-1500mm from root, 0-200mm from leading edge
- **Panel 8**: 1500-1700mm from root, 0-200mm from leading edge

## 3D Modeling Guidelines

### Coordinate System
- **Origin**: Nose tip
- **X-axis**: Forward (positive toward tail)
- **Y-axis**: Right wing (positive toward right)
- **Z-axis**: Up (positive upward)

### Modeling Scale
- **Units**: Millimeters
- **Precision**: 1mm
- **Tolerance**: ±0.5mm

### Surface Details
- **Smooth Surfaces**: All external surfaces should be smooth
- **Panel Lines**: Include access panel lines
- **Rivets**: Include rivet details on access panels
- **Antennas**: Include all antenna protrusions

### Color Scheme
- **Fuselage**: White/light gray
- **Wings**: White/light gray
- **Solar Panels**: Dark blue/black
- **Landing Gear**: Silver/aluminum
- **Antennas**: Black
- **Cameras**: Black
- **Propeller**: Carbon fiber (dark gray)

## Assembly Sequence for 3D Model

### Step 1: Fuselage
1. Create elliptical cross-section
2. Extrude along centerline
3. Add nose cone
4. Add tail cone
5. Add access panels

### Step 2: Wings
1. Create wing planform
2. Apply airfoil cross-section
3. Add dihedral and sweep
4. Add control surfaces
5. Add winglets

### Step 3: Empennage
1. Create horizontal stabilizer
2. Create vertical stabilizer
3. Add control surfaces
4. Position relative to fuselage

### Step 4: Landing Gear
1. Create main gear struts
2. Create nose gear strut
3. Add wheels
4. Position relative to fuselage

### Step 5: Power System
1. Add motor mount
2. Add propeller
3. Add battery compartments
4. Add solar panels

### Step 6: Avionics
1. Add flight controller
2. Add GPS module
3. Add telemetry radio
4. Add antennas

### Step 7: Surveillance System
1. Add camera gimbal
2. Add cameras
3. Add IR illuminators
4. Add transmission antennas

## Technical Drawings Required

### Top View
- Complete aircraft planform
- Wing and tail dimensions
- Control surface positions
- Solar panel layout

### Side View
- Fuselage profile
- Wing and tail side view
- Landing gear positions
- Motor and propeller

### Front View
- Wing dihedral
- Fuselage cross-section
- Landing gear track
- Camera gimbal position

### Isometric View
- Complete aircraft assembly
- All major components
- Scale reference

## Software Recommendations

### CAD Software
- **Fusion 360**: Free for personal use
- **SolidWorks**: Professional grade
- **Inventor**: Professional grade
- **FreeCAD**: Open source alternative

### Rendering Software
- **Blender**: Free, excellent for visualization
- **Keyshot**: Professional rendering
- **V-Ray**: High-end rendering

### Animation Software
- **Blender**: For flight animations
- **Maya**: Professional animation
- **3ds Max**: Professional animation

## File Formats

### CAD Files
- **Primary**: .f3d (Fusion 360)
- **Export**: .step, .iges, .stl
- **Rendering**: .obj, .fbx

### Image Outputs
- **Technical Drawings**: .dwg, .pdf
- **Renders**: .png, .jpg (4K resolution)
- **Animations**: .mp4, .mov

This specification provides all the necessary information to create an accurate 3D model of the compact farm surveillance drone. The dimensions are based on the airframe design document and scaled appropriately for the 2.0m wingspan target.
