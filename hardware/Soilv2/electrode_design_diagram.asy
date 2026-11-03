// Interdigitated Electrode Design for Soil Moisture Sensor
// Enhanced design with soldermask, air reference, and guard ring

import graph;
import geometry;
import patterns;

// Set up the canvas
size(16cm, 12cm);

// Define colors
pen black = black;
pen red = red;
pen blue = blue;
pen green = green;
pen gray = gray;
pen orange = orange;

// Title
label("Interdigitated Soil Moisture Sensor Design", (8, 11.5), fontsize(14pt));

// Primary Soil Sensor Area (with soldermask)
label("Primary Soil Sensor (with soldermask)", (4, 10.5), fontsize(12pt));
label("25mm x 15mm", (4, 10.2), fontsize(10pt));

// Draw primary sensor area
draw((1, 8) -- (6, 8) -- (6, 9.5) -- (1, 9.5) -- cycle, black);
fill((1, 8) -- (6, 8) -- (6, 9.5) -- (1, 9.5) -- cycle, lightgray);

// Draw interdigitated electrodes
// Electrode A (connected to oscillator input)
for (int i = 0; i < 12; ++i) {
    real x = 1.2 + i * 0.4;
    draw((x, 8.1) -- (x, 9.4), red);
    draw((x-0.1, 8.1) -- (x+0.1, 8.1), red);
    draw((x-0.1, 9.4) -- (x+0.1, 9.4), red);
}

// Electrode B (connected to oscillator output)
for (int i = 0; i < 12; ++i) {
    real x = 1.4 + i * 0.4;
    draw((x, 8.1) -- (x, 9.4), blue);
    draw((x-0.1, 8.1) -- (x+0.1, 8.1), blue);
    draw((x-0.1, 9.4) -- (x+0.1, 9.4), blue);
}

// Guard ring around primary sensor
draw((0.5, 7.5) -- (6.5, 7.5) -- (6.5, 10) -- (0.5, 10) -- cycle, orange);
label("Guard Ring", (3.5, 7.3), fontsize(8pt));

// Air Reference Area (without soldermask)
label("Air Reference (no soldermask)", (12, 10.5), fontsize(12pt));
label("10mm x 10mm", (12, 10.2), fontsize(10pt));

// Draw air reference area
draw((9, 8) -- (11, 8) -- (11, 9) -- (9, 9) -- cycle, black);

// Draw air reference electrodes
// Reference Electrode A
for (int i = 0; i < 8; ++i) {
    real x = 9.1 + i * 0.25;
    draw((x, 8.05) -- (x, 8.95), red);
    draw((x-0.05, 8.05) -- (x+0.05, 8.05), red);
    draw((x-0.05, 8.95) -- (x+0.05, 8.95), red);
}

// Reference Electrode B
for (int i = 0; i < 8; ++i) {
    real x = 9.2 + i * 0.25;
    draw((x, 8.05) -- (x, 8.95), blue);
    draw((x-0.05, 8.05) -- (x+0.05, 8.05), blue);
    draw((x-0.05, 8.95) -- (x+0.05, 8.95), blue);
}

// Guard ring around air reference
draw((8.5, 7.5) -- (11.5, 7.5) -- (11.5, 9.5) -- (8.5, 9.5) -- cycle, orange);
label("Guard Ring", (10, 7.3), fontsize(8pt));

// Cross-section view
label("Cross-Section View", (8, 6.5), fontsize(12pt));

// PCB layers
draw((2, 4) -- (14, 4), black); // Top copper
draw((2, 3.8) -- (14, 3.8), gray); // Soldermask
draw((2, 3.6) -- (14, 3.6), black); // Bottom copper
draw((2, 3.4) -- (14, 3.4), gray); // Solder mask

// Labels for layers
label("Top Copper (Electrodes)", (15, 4), fontsize(8pt));
label("Soldermask (Green)", (15, 3.8), fontsize(8pt));
label("Bottom Copper (Ground)", (15, 3.6), fontsize(8pt));
label("Soldermask (Green)", (15, 3.4), fontsize(8pt));

// Primary sensor cross-section
draw((3, 4) -- (7, 4), red); // Electrode A
draw((3.2, 4) -- (6.8, 4), blue); // Electrode B
draw((3, 3.8) -- (7, 3.8), gray); // Soldermask
label("Primary Sensor", (5, 3.6), fontsize(8pt));

// Air reference cross-section
draw((9, 4) -- (11, 4), red); // Reference Electrode A
draw((9.1, 4) -- (10.9, 4), blue); // Reference Electrode B
// No soldermask over air reference
label("Air Reference", (10, 3.6), fontsize(8pt));

// Electrical connections
label("Electrical Connections", (8, 2.5), fontsize(12pt));

// Connection diagram
draw((1, 1) -- (3, 1), red);
label("Electrode A → 74HC14 Pin 1", (3.2, 1), fontsize(8pt));

draw((1, 0.8) -- (3, 0.8), blue);
label("Electrode B → 74HC14 Pin 2", (3.2, 0.8), fontsize(8pt));

draw((1, 0.6) -- (3, 0.6), orange);
label("Guard Ring → 74HC14 Pin 2", (3.2, 0.6), fontsize(8pt));

draw((1, 0.4) -- (3, 0.4), green);
label("Ground Plane → GND", (3.2, 0.4), fontsize(8pt));

// Specifications
label("Specifications:", (8, 0.2), fontsize(10pt));
label("• Finger width: 0.5mm", (1, -0.2), fontsize(8pt));
label("• Finger spacing: 0.5mm", (1, -0.4), fontsize(8pt));
label("• Primary sensor: 12 fingers x 10mm", (1, -0.6), fontsize(8pt));
label("• Air reference: 8 fingers x 5mm", (1, -0.8), fontsize(8pt));
label("• Guard ring: 2mm wide", (1, -1.0), fontsize(8pt));
label("• Soldermask: Applied over primary sensor only", (1, -1.2), fontsize(8pt));
