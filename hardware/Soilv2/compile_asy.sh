#!/bin/bash

echo "Compiling Asymptote files to PDF..."

# Check if Asymptote is installed
if ! command -v asy &> /dev/null; then
    echo "Error: Asymptote is not installed or not in PATH"
    echo "Please install Asymptote from https://asymptote.sourceforge.io/"
    echo "Or use the online compiler at https://asymptote.ualberta.ca/"
    exit 1
fi

# Compile circuit diagram
echo "Compiling circuit_diagram.asy..."
if asy -f pdf circuit_diagram.asy; then
    echo "✓ circuit_diagram.pdf created successfully"
else
    echo "✗ Error compiling circuit_diagram.asy"
fi

# Compile electrode design diagram
echo "Compiling electrode_design_diagram.asy..."
if asy -f pdf electrode_design_diagram.asy; then
    echo "✓ electrode_design_diagram.pdf created successfully"
else
    echo "✗ Error compiling electrode_design_diagram.asy"
fi

echo ""
echo "Compilation complete!"
echo "Generated PDF files:"
[ -f circuit_diagram.pdf ] && echo "- circuit_diagram.pdf"
[ -f electrode_design_diagram.pdf ] && echo "- electrode_design_diagram.pdf"
