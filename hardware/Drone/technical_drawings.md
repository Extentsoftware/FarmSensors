# Technical Drawings - Compact Farm Surveillance Drone

## Drawing Set Overview
This document provides detailed technical drawings for the compact farm surveillance drone with 2.0m wingspan and 1.2m length.

## Drawing 1: Top View (Planform)

### Overall Dimensions
- **Wingspan**: 2000mm
- **Length**: 1200mm
- **Wing Area**: 0.33 m²
- **Aspect Ratio**: 10:1

### Wing Specifications
- **Root Chord**: 200mm at X=400mm
- **Tip Chord**: 130mm at X=1400mm
- **Leading Edge**: 1° forward sweep
- **Trailing Edge**: Straight
- **Winglets**: 80mm height, 15° cant

### Control Surfaces
- **Ailerons**: 36mm chord × 1000mm span
- **Flaps**: 40mm chord × 600mm span
- **Spoilers**: 24mm chord × 500mm span

### Fuselage Outline
- **Nose**: 50mm radius
- **Forward Section**: 100mm width (0-300mm)
- **Central Section**: 150mm width (300-800mm)
- **Aft Section**: 100mm width (800-1200mm)
- **Tail**: 50mm radius

### Empennage
- **Horizontal Stabilizer**: 600mm span × 130mm chord
- **Vertical Stabilizer**: 200mm height × 150mm chord
- **Position**: 1000mm from nose

### Solar Panel Layout
- **8 Panels**: 200mm × 100mm each
- **Coverage**: 85% of wing upper surface
- **Spacing**: 200mm between panels

## Drawing 2: Front View (Cross-Section)

### Fuselage Cross-Section
- **Shape**: Elliptical
- **Major Axis**: 150mm (width)
- **Minor Axis**: 120mm (height)
- **Position**: 500mm from nose

### Wing Dihedral
- **Angle**: 2°
- **Root Height**: 0mm
- **Tip Height**: 35mm

### Landing Gear
- **Main Gear Track**: 800mm
- **Wheel Diameter**: 80mm
- **Ground Clearance**: 150mm
- **Position**: 400mm from nose

### Camera Gimbal
- **Position**: 400mm from nose, 50mm below fuselage
- **Dimensions**: 120mm × 100mm × 80mm
- **Range**: Pan ±180°, Tilt ±60°

## Drawing 3: Side View (Profile)

### Fuselage Profile
- **Nose**: 50mm radius hemisphere
- **Forward Section**: 100mm width (0-300mm)
- **Central Section**: 150mm width (300-800mm)
- **Aft Section**: 100mm width (800-1200mm)
- **Tail**: 50mm radius hemisphere

### Wing Profile
- **Root Chord**: 200mm
- **Tip Chord**: 130mm
- **Sweep**: 1° forward
- **Position**: 400-1400mm from nose

### Empennage Profile
- **Horizontal Stabilizer**: 130mm chord
- **Vertical Stabilizer**: 150mm chord
- **Position**: 1000mm from nose

### Power System
- **Motor**: 80mm diameter × 60mm length
- **Propeller**: 600mm diameter
- **Position**: 1100mm from nose

### Landing Gear Profile
- **Main Gear**: 150mm strut length
- **Nose Gear**: 100mm strut length
- **Wheel Diameter**: 80mm (main), 60mm (nose)

## Drawing 4: Isometric View

### Complete Assembly
- **All Major Components**
- **Scale Reference**: 100mm grid
- **Viewing Angle**: 30° elevation, 45° azimuth

### Component Visibility
- **Fuselage**: White/light gray
- **Wings**: White/light gray
- **Solar Panels**: Dark blue/black
- **Landing Gear**: Silver/aluminum
- **Antennas**: Black
- **Cameras**: Black
- **Propeller**: Carbon fiber (dark gray)

## Drawing 5: Fuselage Cross-Sections

### Section A-A (Forward Section)
- **Position**: 150mm from nose
- **Shape**: Elliptical
- **Dimensions**: 100mm × 80mm
- **Components**: GPS antenna, forward battery

### Section B-B (Central Section)
- **Position**: 500mm from nose
- **Shape**: Elliptical
- **Dimensions**: 150mm × 120mm
- **Components**: Flight controller, main battery, camera gimbal

### Section C-C (Aft Section)
- **Position**: 1000mm from nose
- **Shape**: Elliptical
- **Dimensions**: 100mm × 80mm
- **Components**: Motor mount, telemetry antenna

## Drawing 6: Wing Cross-Sections

### Root Section
- **Position**: 400mm from nose
- **Chord**: 200mm
- **Thickness**: 20mm (10% of chord)
- **Airfoil**: Selig S1223
- **Components**: Aileron, flap, spoiler

### Tip Section
- **Position**: 1400mm from nose
- **Chord**: 130mm
- **Thickness**: 13mm (10% of chord)
- **Airfoil**: Selig S1223
- **Components**: Winglet

## Drawing 7: Control Surface Details

### Aileron
- **Chord**: 36mm (18% of wing chord)
- **Span**: 1000mm
- **Hinge Line**: 18mm from trailing edge
- **Deflection**: ±20°

### Flap
- **Chord**: 40mm (20% of wing chord)
- **Span**: 600mm
- **Hinge Line**: 20mm from trailing edge
- **Deflection**: 0° to 30°

### Spoiler
- **Chord**: 24mm (12% of wing chord)
- **Span**: 500mm
- **Position**: 12mm from trailing edge
- **Deflection**: 0° to 45°

### Elevator
- **Chord**: 39mm (30% of stabilizer chord)
- **Span**: 600mm
- **Hinge Line**: 19.5mm from trailing edge
- **Deflection**: ±20°

### Rudder
- **Chord**: 37.5mm (25% of stabilizer chord)
- **Height**: 200mm
- **Hinge Line**: 18.75mm from trailing edge
- **Deflection**: ±25°

## Drawing 8: Power System Layout

### Motor Assembly
- **Motor**: 80mm diameter × 60mm length
- **Mount**: 100mm diameter ring
- **Position**: 1100mm from nose
- **Cooling**: Heat sinks and fans

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

### Solar Panels
- **Type**: Flexible monocrystalline
- **Dimensions**: 200mm × 100mm × 2mm each
- **Total Panels**: 8 panels
- **Coverage**: 85% of wing upper surface

## Drawing 9: Avionics Layout

### Flight Controller
- **Type**: Pixhawk 2.1
- **Dimensions**: 80mm × 50mm × 20mm
- **Position**: 300mm from nose, centerline
- **Mounting**: Vibration isolation

### GPS Module
- **Dimensions**: 60mm × 40mm × 15mm
- **Position**: 250mm from nose, centerline
- **Antenna**: 50mm diameter, 20mm height

### Telemetry Radio
- **Dimensions**: 100mm × 60mm × 25mm
- **Position**: 350mm from nose, centerline
- **Antenna**: 200mm length, 5mm diameter

### Emergency Beacon
- **Dimensions**: 80mm × 60mm × 30mm
- **Position**: 400mm from nose, centerline
- **Antenna**: 150mm length, 3mm diameter

## Drawing 10: Surveillance System Layout

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

### Transmission Antennas
- **Video Transmitter**: 5.8GHz
- **Dimensions**: 50mm × 20mm × 10mm
- **Position**: 450mm from nose, centerline
- **Antenna**: 100mm length, 3mm diameter

## Drawing Standards

### Line Types
- **Visible Lines**: 0.5mm solid
- **Hidden Lines**: 0.25mm dashed
- **Center Lines**: 0.25mm dash-dot
- **Dimension Lines**: 0.25mm solid
- **Leader Lines**: 0.25mm solid

### Text Standards
- **Title Block**: 5mm height
- **Dimensions**: 3mm height
- **Notes**: 2.5mm height
- **Font**: Arial or similar

### Dimensioning
- **Units**: Millimeters
- **Precision**: 1mm
- **Tolerance**: ±0.5mm
- **Reference Dimensions**: (X.X)

### Scale
- **Main Views**: 1:10
- **Detail Views**: 1:5 or 1:2
- **Assembly Views**: 1:20

## Drawing Notes

### General Notes
1. All dimensions in millimeters
2. Tolerances ±0.5mm unless otherwise specified
3. All external surfaces smooth
4. All internal surfaces as specified
5. All fasteners metric unless otherwise specified

### Material Notes
1. Fuselage: Carbon fiber composite
2. Wings: Carbon fiber composite with foam core
3. Landing Gear: Aluminum and carbon fiber
4. Solar Panels: Flexible monocrystalline silicon
5. Avionics: Commercial off-the-shelf components

### Assembly Notes
1. All components to be assembled per assembly drawing
2. All fasteners to be torqued to specification
3. All electrical connections to be tested
4. All control surfaces to be balanced
5. All systems to be tested before flight

This technical drawing set provides all the necessary information for creating accurate CAD models and manufacturing the compact farm surveillance drone.
