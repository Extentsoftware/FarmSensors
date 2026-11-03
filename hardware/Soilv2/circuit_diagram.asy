// Capacitive Soil Moisture Sensor Circuit
// Created with Asymptote

import graph;
import geometry;
import patterns;

// Set up the canvas
size(12cm, 8cm);

// Define colors
pen black = black;
pen red = red;
pen blue = blue;
pen green = green;
pen gray = gray;

// Title
label("Capacitive Soil Moisture Sensor Circuit", (6, 7.5), fontsize(12pt));

// Power supply section
label("3.3V Supply", (1, 6.5), fontsize(10pt));
draw((0.5, 6) -- (1.5, 6), black);
draw((1, 6) -- (1, 5.5), black);
label("+", (1.2, 5.7), fontsize(8pt));

// 74HC14 Schmitt Trigger
label("74HC14", (3, 6), fontsize(10pt));
draw((2.5, 5.5) -- (3.5, 5.5), black);
draw((2.5, 5.5) -- (2.5, 4.5), black);
draw((3.5, 5.5) -- (3.5, 4.5), black);
draw((2.5, 4.5) -- (3.5, 4.5), black);

// Pin labels for 74HC14
label("14", (2.3, 5.7), fontsize(8pt)); // VCC
label("1", (2.3, 5.3), fontsize(8pt));  // Input
label("2", (3.7, 5.3), fontsize(8pt));  // Output
label("7", (2.3, 4.7), fontsize(8pt));  // GND

// RC Oscillator components
label("R1", (4.5, 5.5), fontsize(8pt));
draw((4, 5.3) -- (4.2, 5.3), black);
draw((4.2, 5.3) -- (4.2, 5.1), black);
draw((4.2, 5.1) -- (4.4, 5.1), black);
draw((4.4, 5.1) -- (4.4, 5.3), black);
draw((4.4, 5.3) -- (4.6, 5.3), black);
label("10kΩ", (4.1, 4.9), fontsize(7pt));

// Soil sensor capacitor
label("Soil Sensor", (6, 5.5), fontsize(8pt));
draw((5.5, 5.3) -- (5.7, 5.3), black);
draw((5.7, 5.3) -- (5.7, 5.1), black);
draw((5.7, 5.1) -- (5.9, 5.1), black);
draw((5.9, 5.1) -- (5.9, 5.3), black);
draw((5.9, 5.3) -- (6.1, 5.3), black);
label("C1", (5.6, 4.9), fontsize(7pt));

// ATTINY85
label("ATTINY85", (8, 6), fontsize(10pt));
draw((7.5, 5.5) -- (8.5, 5.5), black);
draw((7.5, 5.5) -- (7.5, 4.5), black);
draw((8.5, 5.5) -- (8.5, 4.5), black);
draw((7.5, 4.5) -- (8.5, 4.5), black);

// ATTINY85 pin labels
label("1", (7.3, 5.7), fontsize(8pt)); // Reset
label("2", (7.3, 5.3), fontsize(8pt)); // PB3 (1-wire)
label("3", (7.3, 5.1), fontsize(8pt)); // PB4 (freq input)
label("4", (7.3, 4.9), fontsize(8pt)); // GND
label("8", (8.7, 5.7), fontsize(8pt)); // VCC

// CubeCell LoRa Module
label("CubeCell", (10, 6), fontsize(10pt));
draw((9.5, 5.5) -- (10.5, 5.5), black);
draw((9.5, 5.5) -- (9.5, 4.5), black);
draw((10.5, 5.5) -- (10.5, 4.5), black);
draw((9.5, 4.5) -- (10.5, 4.5), black);

// CubeCell pin labels
label("VCC", (9.3, 5.7), fontsize(8pt));
label("GND", (9.3, 4.7), fontsize(8pt));
label("Data", (10.7, 5.3), fontsize(8pt));

// Ground connections
label("GND", (1, 3.5), fontsize(10pt));
draw((0.5, 3) -- (1.5, 3), black);
draw((1, 3) -- (1, 2.5), black);
label("⊥", (1.2, 2.7), fontsize(8pt));

// Connections
// Power to 74HC14
draw((1, 5.5) -- (2.5, 5.5), red);
draw((1, 5.5) -- (1, 5.7), red);
draw((1, 5.7) -- (2.5, 5.7), red);
draw((2.5, 5.7) -- (2.5, 5.5), red);

// Power to ATTINY85
draw((1, 5.5) -- (7.5, 5.5), red);
draw((7.5, 5.5) -- (7.5, 5.7), red);
draw((7.5, 5.7) -- (8.5, 5.7), red);
draw((8.5, 5.7) -- (8.5, 5.5), red);

// Power to CubeCell
draw((1, 5.5) -- (9.5, 5.5), red);
draw((9.5, 5.5) -- (9.5, 5.7), red);
draw((9.5, 5.7) -- (10.5, 5.7), red);
draw((10.5, 5.7) -- (10.5, 5.5), red);

// Ground connections
draw((1, 2.5) -- (2.5, 2.5), black);
draw((2.5, 2.5) -- (2.5, 4.5), black);
draw((1, 2.5) -- (7.5, 2.5), black);
draw((7.5, 2.5) -- (7.5, 4.5), black);
draw((1, 2.5) -- (9.5, 2.5), black);
draw((9.5, 2.5) -- (9.5, 4.5), black);

// 1-wire connection
draw((8.5, 5.3) -- (9.5, 5.3), blue);
label("1-wire", (9, 5.1), fontsize(8pt));

// Frequency input
draw((3.5, 5.3) -- (7.5, 5.3), green);
label("freq", (5.5, 5.1), fontsize(8pt));

// RC oscillator connections
draw((3.5, 5.3) -- (4, 5.3), black);
draw((4.6, 5.3) -- (5.5, 5.3), black);
draw((6.1, 5.3) -- (6.5, 5.3), black);
draw((6.5, 5.3) -- (6.5, 2.5), black);
draw((6.5, 2.5) -- (1, 2.5), black);

// Decoupling capacitors
label("100nF", (1.5, 4.5), fontsize(7pt));
draw((1.2, 4.3) -- (1.4, 4.3), black);
draw((1.4, 4.3) -- (1.4, 4.1), black);
draw((1.4, 4.1) -- (1.6, 4.1), black);
draw((1.6, 4.1) -- (1.6, 4.3), black);
draw((1.6, 4.3) -- (1.8, 4.3), black);

// Legend
label("Legend:", (0.5, 2), fontsize(10pt));
draw((0.5, 1.8) -- (0.7, 1.8), red);
label("Power (3.3V)", (0.8, 1.8), fontsize(8pt));
draw((0.5, 1.6) -- (0.7, 1.6), black);
label("Ground", (0.8, 1.6), fontsize(8pt));
draw((0.5, 1.4) -- (0.7, 1.4), blue);
label("1-wire Data", (0.8, 1.4), fontsize(8pt));
draw((0.5, 1.2) -- (0.7, 1.2), green);
label("Frequency Signal", (0.8, 1.2), fontsize(8pt));
