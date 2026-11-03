# Installing Asymptote on Windows

## Method 1: Download from Official Website
1. Go to https://asymptote.sourceforge.io/
2. Click "Download" → "Windows"
3. Download the latest Windows installer
4. Run the installer and follow the setup wizard
5. Add Asymptote to your PATH during installation

## Method 2: Using Chocolatey (if you have it)
```powershell
choco install asymptote
```

## Method 3: Using Scoop (if you have it)
```powershell
scoop install asymptote
```

## After Installation
1. Open a new Command Prompt or PowerShell
2. Navigate to your project directory: `cd C:\Data\Bongo\Soil2`
3. Run the compilation script: `compile_asy.bat`
4. Or compile manually:
   ```bash
   asy -f pdf circuit_diagram.asy
   asy -f pdf electrode_design_diagram.asy
   ```

## Verify Installation
```bash
asy --version
```

This should show the Asymptote version if installed correctly.
