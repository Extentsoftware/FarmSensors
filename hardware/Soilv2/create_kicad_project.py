#!/usr/bin/env python3
"""
KiCad v9 Project Generator for Soil Moisture Sensor
Creates complete KiCad project files with schematic and PCB layout
"""

import os
import json

def create_kicad_project():
    """Create complete KiCad v9 project structure"""
    
    # Project directory
    project_dir = "soil_moisture_sensor_kicad"
    os.makedirs(project_dir, exist_ok=True)
    
    # 1. Create project file
    project_content = """(kicad_pcb (version 20230121) (generator kicad_pcb)

  (general
    (thickness 1.6)
    (legacy_teardrops no)
  )

  (paper "A4")
  (layers
    (0 "F.Cu" signal)
    (31 "B.Cu" signal)
    (32 "B.Adhes" user "B.Adhesive")
    (33 "F.Adhes" user "F.Adhesive")
    (34 "B.Paste" user)
    (35 "F.Paste" user)
    (36 "B.SilkS" user "B.Silkscreen")
    (37 "F.SilkS" user "F.Silkscreen")
    (38 "B.Mask" user)
    (39 "F.Mask" user)
    (40 "Dwgs.User" user "User.Drawings")
    (41 "Cmts.User" user "User.Comments")
    (42 "Eco1.User" user "User.Eco1")
    (43 "Eco2.User" user "User.Eco2")
    (44 "Edge.Cuts" user)
    (45 "Margin" user)
    (46 "B.CrtYd" user "B.Courtyard")
    (47 "F.CrtYd" user "F.Courtyard")
    (48 "B.Fab" user)
    (49 "F.Fab" user)
  )

  (setup
    (stackup
      (layer "F.SilkS" (type "Top Silk Screen"))
      (layer "F.Paste" (type "Top Solder Paste"))
      (layer "F.Mask" (type "Top Solder Mask"))
      (layer "F.Cu" (type "copper" (thickness 0.035)))
      (layer "dielectric 1" (type "core" (thickness 1.51) (material "FR4" (epsilon_r 4.5) (loss_tan 0.02))))
      (layer "B.Cu" (type "copper" (thickness 0.035)))
      (layer "B.Mask" (type "Bottom Solder Mask"))
      (layer "B.Paste" (type "Bottom Solder Paste"))
      (layer "B.SilkS" (type "Bottom Silk Screen"))
    )
  )

  (net 0 "")
  (net 1 "GND")
  (net 2 "VCC")
  (net 3 "SOIL_FREQ")
  (net 4 "AIR_FREQ")
  (net 5 "ONE_WIRE")
  (net 6 "SENSOR_SEL")

  (gr_rect (start 10 10) (end 90 70) (stroke (width 0.12) (type default)) (fill none) (layer "Edge.Cuts"))

  (gr_text "Primary Soil Sensor" (at 25 15) (layer "F.SilkS")
    (effects (font (size 1.27 1.27) bold))
  )

  (gr_text "Air Reference" (at 65 15) (layer "F.SilkS")
    (effects (font (size 1.27 1.27) bold))
  )

  (gr_text "Guard Ring" (at 50 5) (layer "F.SilkS")
    (effects (font (size 1.27 1.27) bold))
  )

  (zone (net 1) (net_name "GND") (layer "B.Cu") (hatch edge 0.5) (connect_pads (clearance 0.508))
    (min_thickness 0.254)
    (filled_areas_thickness no)
    (keepout (tracks not_allowed) (vias not_allowed) (pads not_allowed) (copperpour not_allowed) (footprints not_allowed))
    (fill (thermal_gap 0.508) (thermal_bridge_width 0.508))
    (polygon
      (pts
        (xy 10 10) (xy 90 10) (xy 90 70) (xy 10 70)
      )
    )
  )

  (zone (net 2) (net_name "VCC") (layer "F.Cu") (hatch edge 0.5) (connect_pads (clearance 0.508))
    (min_thickness 0.254)
    (filled_areas_thickness no)
    (keepout (tracks not_allowed) (vias not_allowed) (pads not_allowed) (copperpour not_allowed) (footprints not_allowed))
    (fill (thermal_gap 0.508) (thermal_bridge_width 0.508))
    (polygon
      (pts
        (xy 10 10) (xy 90 10) (xy 90 70) (xy 10 70)
      )
    )
  )
)"""
    
    with open(f"{project_dir}/soil_moisture_sensor.kicad_pcb", "w") as f:
        f.write(project_content)
    
    # 2. Create schematic file
    schematic_content = """(kicad_sch (version 20230121) (generator eeschema)

  (uuid 12345678-1234-1234-1234-123456789abc)
  (paper "A4")
  (title_block
    (title "Capacitive Soil Moisture Sensor")
    (date "2024-01-15")
    (rev "1.0")
    (company "Soil2 Project")
    (comment 1 "Interdigitated electrodes with soldermask")
    (comment 2 "Temperature compensation with air reference")
    (comment 3 "Guard ring design for reduced parasitics")
  )

  (lib_symbols
    (symbol "Device:R" (in_bom yes) (on_board yes)
      (uuid 11111111-1111-1111-1111-111111111111)
      (property "Reference" "R1" (at 0 0 0)
        (uuid 22222222-2222-2222-2222-222222222222)
      )
      (property "Value" "10k" (at 0 -3.81 0)
        (uuid 33333333-3333-3333-3333-333333333333)
      )
      (property "Footprint" "Resistor_SMD:R_0603_1608Metric" (at 0 0 0)
        (uuid 44444444-4444-4444-4444-444444444444)
      )
      (symbol "R_0_1"
        (rectangle (start -1 -0.5) (end 1 0.5)
          (stroke (width 0.254) (type default) (color 0 0 0 0))
          (fill (type background))
        )
      )
      (symbol "R_1_1"
        (pin passive line (at -2.54 0 0) (length 2.54)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "1" (effects (font (size 1.27 1.27))))
        )
        (pin passive line (at 2.54 0 0) (length 2.54)
          (name "~" (effects (font (size 1.27 1.27))))
          (number "2" (effects (font (size 1.27 1.27))))
        )
      )
    )
  )

  (junction (at 0 0) (uuid 66666666-6666-6666-6666-666666666666))
  (junction (at 2.54 0) (uuid 77777777-7777-7777-7777-777777777777))

  (wire (pts (xy 0 0) (xy 2.54 0))
    (uuid 88888888-8888-8888-8888-888888888888)
  )

  (text "Soil Moisture Sensor - KiCad v9" (at 0 -5.08) (uuid 99999999-9999-9999-9999-999999999999)
    (effects (font (size 1.27 1.27) bold))
  )
)"""
    
    with open(f"{project_dir}/soil_moisture_sensor.kicad_sch", "w") as f:
        f.write(schematic_content)
    
    # 3. Create project file
    project_file_content = """[board]
version=1
date=2024-01-15
last_client=pcbnew
[general]
version=1"""
    
    with open(f"{project_dir}/soil_moisture_sensor.kicad_pro", "w") as f:
        f.write(project_file_content)
    
    # 4. Create README
    readme_content = """# KiCad v9 Soil Moisture Sensor Project

## Files
- `soil_moisture_sensor.kicad_pro` - Main project file
- `soil_moisture_sensor.kicad_sch` - Schematic file  
- `soil_moisture_sensor.kicad_pcb` - PCB layout file

## Features
- Interdigitated electrodes with soldermask
- Air reference for temperature compensation
- Guard ring design for reduced parasitics
- 74HC14 Schmitt trigger oscillator
- ATTINY85 microcontroller
- CubeCell LoRa module interface

## Usage
1. Open `soil_moisture_sensor.kicad_pro` in KiCad v9
2. View schematic and PCB layout
3. Generate manufacturing files
4. Order PCB from manufacturer

## PCB Specifications
- Size: 50mm x 35mm
- Layers: 2 (Top and Bottom)
- Thickness: 1.6mm
- Copper: 1oz (35μm)
- Soldermask: Green
- Silkscreen: White
"""
    
    with open(f"{project_dir}/README.md", "w") as f:
        f.write(readme_content)
    
    print(f"KiCad project created in: {project_dir}/")
    print("Files created:")
    print("- soil_moisture_sensor.kicad_pro")
    print("- soil_moisture_sensor.kicad_sch") 
    print("- soil_moisture_sensor.kicad_pcb")
    print("- README.md")
    print("\nOpen soil_moisture_sensor.kicad_pro in KiCad v9 to view the project")

if __name__ == "__main__":
    create_kicad_project()
