# Fusion 360 CAD Guide - Compact Farm Surveillance Drone

## Project Setup

### Create New Project
1. Open Fusion 360
2. Create new project: "Compact Farm Drone"
3. Set units to millimeters
4. Create new design: "Drone_Assembly"

## Step-by-Step CAD Creation

### Step 1: Create Fuselage

#### Fuselage Body
1. **Create Sketch** on XY plane
2. **Draw Ellipse** at origin:
   - Major axis: 150mm
   - Minor axis: 120mm
3. **Extrude** 1200mm in +X direction
4. **Create Nose Cone**:
   - Sketch on front face
   - Draw circle (50mm radius)
   - **Revolve** 180° around X-axis
5. **Create Tail Cone**:
   - Sketch on back face
   - Draw circle (50mm radius)
   - **Revolve** 180° around X-axis

#### Fuselage Sections
1. **Forward Section (0-300mm)**:
   - Create plane at X=300mm
   - Sketch ellipse: 100mm × 80mm
   - **Loft** from nose to this section
2. **Central Section (300-800mm)**:
   - Keep constant 150mm × 120mm ellipse
3. **Aft Section (800-1200mm)**:
   - Create plane at X=800mm
   - Sketch ellipse: 100mm × 80mm
   - **Loft** from central section to tail

#### Access Panels
1. **Top Access Panel**:
   - Sketch on top surface
   - Rectangle: 200mm × 100mm
   - **Extrude Cut** 2mm deep
2. **Side Access Panels**:
   - Sketch on side surfaces
   - Rectangle: 150mm × 80mm
   - **Extrude Cut** 2mm deep

### Step 2: Create Wings

#### Wing Planform
1. **Create Sketch** on XY plane
2. **Draw Wing Outline**:
   - Root chord: 200mm at X=400mm
   - Tip chord: 130mm at X=1400mm
   - Leading edge: 1° forward sweep
   - Trailing edge: straight
3. **Extrude** 20mm in +Z direction
4. **Apply Dihedral**: 2° rotation around X-axis

#### Wing Airfoil
1. **Create Airfoil Sketch**:
   - Use Selig S1223 coordinates
   - Scale to 200mm chord
   - **Revolve** around leading edge
2. **Apply to Wing**:
   - **Sweep** airfoil along wing planform
3. **Create Winglets**:
   - Sketch on wing tip
   - Rectangle: 80mm × 20mm
   - **Extrude** 80mm with 15° cant

#### Control Surfaces
1. **Ailerons**:
   - Sketch on wing trailing edge
   - Rectangle: 36mm × 1000mm
   - **Extrude Cut** 5mm deep
2. **Flaps**:
   - Sketch on wing trailing edge
   - Rectangle: 40mm × 600mm
   - **Extrude Cut** 5mm deep
3. **Spoilers**:
   - Sketch on wing upper surface
   - Rectangle: 24mm × 500mm
   - **Extrude Cut** 3mm deep

### Step 3: Create Empennage

#### Horizontal Stabilizer
1. **Create Sketch** on XY plane at X=1000mm
2. **Draw Rectangle**: 600mm × 130mm
3. **Extrude** 15mm in +Z direction
4. **Create Elevator**:
   - Sketch on trailing edge
   - Rectangle: 39mm × 600mm
   - **Extrude Cut** 5mm deep

#### Vertical Stabilizer
1. **Create Sketch** on XZ plane at X=1000mm
2. **Draw Rectangle**: 200mm × 150mm
3. **Extrude** 15mm in +Y direction
4. **Apply 15° Sweep** to leading edge
5. **Create Rudder**:
   - Sketch on trailing edge
   - Rectangle: 37.5mm × 200mm
   - **Extrude Cut** 5mm deep

### Step 4: Create Landing Gear

#### Main Landing Gear
1. **Create Strut**:
   - Sketch on XZ plane at X=400mm
   - Line: 150mm long at 15° angle
   - **Extrude** 10mm diameter circle
2. **Create Wheel**:
   - Sketch on strut end
   - Circle: 80mm diameter
   - **Extrude** 20mm width
3. **Mirror** to other side

#### Nose Landing Gear
1. **Create Strut**:
   - Sketch on XZ plane at X=100mm
   - Line: 100mm long at 15° angle
   - **Extrude** 8mm diameter circle
2. **Create Wheel**:
   - Sketch on strut end
   - Circle: 60mm diameter
   - **Extrude** 15mm width

### Step 5: Create Power System

#### Motor Mount
1. **Create Ring**:
   - Sketch on fuselage back face
   - Circle: 100mm diameter
   - **Extrude** 20mm in +X direction
2. **Create Motor**:
   - Sketch on mount face
   - Circle: 80mm diameter
   - **Extrude** 60mm in +X direction

#### Propeller
1. **Create Hub**:
   - Sketch on motor face
   - Circle: 20mm diameter
   - **Extrude** 30mm in +X direction
2. **Create Blades**:
   - Sketch on hub face
   - Draw blade profile (300mm long)
   - **Revolve** 180° around X-axis
   - **Pattern** 2 instances

#### Battery Compartments
1. **Forward Battery**:
   - Sketch on fuselage side
   - Rectangle: 200mm × 100mm
   - **Extrude Cut** 60mm deep
2. **Main Battery**:
   - Sketch on fuselage side
   - Rectangle: 300mm × 150mm
   - **Extrude Cut** 80mm deep

### Step 6: Create Solar Panels

#### Panel Layout
1. **Create Panel 1**:
   - Sketch on wing upper surface
   - Rectangle: 200mm × 100mm
   - **Extrude** 2mm in +Z direction
2. **Pattern Panels**:
   - **Rectangular Pattern** 8 instances
   - Spacing: 200mm in X direction
3. **Apply to Wing**:
   - **Project** panel outlines onto wing surface
   - **Extrude** 2mm normal to surface

### Step 7: Create Avionics

#### Flight Controller
1. **Create Box**:
   - Sketch on fuselage floor at X=300mm
   - Rectangle: 80mm × 50mm
   - **Extrude** 20mm in +Z direction

#### GPS Module
1. **Create Box**:
   - Sketch on fuselage floor at X=250mm
   - Rectangle: 60mm × 40mm
   - **Extrude** 15mm in +Z direction
2. **Create Antenna**:
   - Sketch on top face
   - Circle: 50mm diameter
   - **Extrude** 20mm in +Z direction

#### Telemetry Radio
1. **Create Box**:
   - Sketch on fuselage floor at X=350mm
   - Rectangle: 100mm × 60mm
   - **Extrude** 25mm in +Z direction
2. **Create Antenna**:
   - Sketch on top face
   - Line: 200mm long
   - **Extrude** 5mm diameter circle

### Step 8: Create Surveillance System

#### Camera Gimbal
1. **Create Base**:
   - Sketch on fuselage bottom at X=400mm
   - Rectangle: 120mm × 100mm
   - **Extrude** 20mm in -Z direction
2. **Create Gimbal Arms**:
   - Sketch on base face
   - Draw arm profile
   - **Extrude** 10mm in -Z direction

#### Cameras
1. **Primary Camera**:
   - Sketch on gimbal
   - Rectangle: 80mm × 60mm
   - **Extrude** 40mm in -Z direction
2. **Thermal Camera**:
   - Sketch on gimbal
   - Rectangle: 60mm × 50mm
   - **Extrude** 30mm in -Z direction
3. **Wide-Angle Camera**:
   - Sketch on gimbal
   - Rectangle: 50mm × 40mm
   - **Extrude** 25mm in -Z direction

#### IR Illuminators
1. **Create Illuminator**:
   - Sketch on gimbal
   - Rectangle: 100mm × 20mm
   - **Extrude** 15mm in -Z direction
2. **Pattern** 2 instances

### Step 9: Assembly

#### Create Assembly
1. **Create New Component**: "Drone_Assembly"
2. **Insert All Parts**:
   - Fuselage
   - Wings
   - Empennage
   - Landing Gear
   - Power System
   - Solar Panels
   - Avionics
   - Surveillance System

#### Apply Constraints
1. **Mate Constraints**:
   - Wing to fuselage
   - Empennage to fuselage
   - Landing gear to fuselage
   - All other components

2. **Angle Constraints**:
   - Wing dihedral: 2°
   - Wing sweep: 1°
   - Stabilizer incidence: 0°

### Step 10: Technical Drawings

#### Create Drawing
1. **New Drawing** from assembly
2. **Add Views**:
   - Top view
   - Front view
   - Side view
   - Isometric view
3. **Add Dimensions**:
   - Overall dimensions
   - Component dimensions
   - Critical measurements

#### Drawing Views
1. **Top View**:
   - Complete planform
   - Wing and tail dimensions
   - Control surface positions
2. **Front View**:
   - Wing dihedral
   - Fuselage cross-section
   - Landing gear track
3. **Side View**:
   - Fuselage profile
   - Wing and tail side view
   - Motor and propeller
4. **Isometric View**:
   - Complete assembly
   - All major components

## File Organization

### Component Structure
```
Drone_Assembly
├── Fuselage
│   ├── Fuselage_Body
│   ├── Nose_Cone
│   └── Tail_Cone
├── Wings
│   ├── Left_Wing
│   ├── Right_Wing
│   └── Winglets
├── Empennage
│   ├── Horizontal_Stabilizer
│   └── Vertical_Stabilizer
├── Landing_Gear
│   ├── Main_Gear_Left
│   ├── Main_Gear_Right
│   └── Nose_Gear
├── Power_System
│   ├── Motor
│   ├── Propeller
│   └── Battery_Compartments
├── Solar_Panels
│   └── Solar_Panel_Array
├── Avionics
│   ├── Flight_Controller
│   ├── GPS_Module
│   └── Telemetry_Radio
└── Surveillance_System
    ├── Camera_Gimbal
    ├── Cameras
    └── IR_Illuminators
```

## Export Settings

### CAD Files
- **Fusion 360**: .f3d (native format)
- **STEP**: .step (for other CAD software)
- **IGES**: .iges (for legacy software)
- **STL**: .stl (for 3D printing)

### Drawings
- **PDF**: .pdf (for documentation)
- **DWG**: .dwg (for AutoCAD)
- **DXF**: .dxf (for 2D CAD)

### Renders
- **PNG**: .png (high resolution)
- **JPG**: .jpg (compressed)
- **TIFF**: .tiff (uncompressed)

## Quality Control

### Check Dimensions
1. **Overall Length**: 1200mm
2. **Wingspan**: 2000mm
3. **Height**: 300mm
4. **Wing Area**: 0.33 m²
5. **Weight**: 7kg (estimated)

### Verify Assembly
1. **All Components Fit**
2. **No Interferences**
3. **Proper Constraints**
4. **Complete Assembly**

### Test Renders
1. **Multiple Angles**
2. **Different Lighting**
3. **Material Appearance**
4. **Scale Reference**

This guide provides step-by-step instructions for creating the complete CAD model in Fusion 360. Each step includes specific dimensions and techniques to ensure accuracy and completeness.
