@echo off
echo Compiling Asymptote files to PDF...

REM Check if Asymptote is installed
where asy >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: Asymptote is not installed or not in PATH
    echo Please install Asymptote from https://asymptote.sourceforge.io/
    echo Or use the online compiler at https://asymptote.ualberta.ca/
    pause
    exit /b 1
)

REM Compile circuit diagram
echo Compiling circuit_diagram.asy...
asy -f pdf circuit_diagram.asy
if %ERRORLEVEL% EQU 0 (
    echo ✓ circuit_diagram.pdf created successfully
) else (
    echo ✗ Error compiling circuit_diagram.asy
)

REM Compile electrode design diagram
echo Compiling electrode_design_diagram.asy...
asy -f pdf electrode_design_diagram.asy
if %ERRORLEVEL% EQU 0 (
    echo ✓ electrode_design_diagram.pdf created successfully
) else (
    echo ✗ Error compiling electrode_design_diagram.asy
)

echo.
echo Compilation complete!
echo Generated PDF files:
if exist circuit_diagram.pdf echo - circuit_diagram.pdf
if exist electrode_design_diagram.pdf echo - electrode_design_diagram.pdf

pause
