# Vesoil Farm Monitoring

Plant health is, in part, dependent on good soil condition which includes correct moisture and nutriants. Our initial project is for remote sensing of soil moisture, ground temperature, air temperature and humidity, based on the TTGO T-Beam board. You need at least one sensor board and one hub for a working system. The sensor board transmits using [LoRa](https://en.wikipedia.org/wiki/LoRa) (long range radio) on 868Mhz to the hub. The frequency can be changed if desired.

The devices are designed to run on a small Macadamia plantation in Zimbabwe, and will therefore run off solar energy as sunlight is generally not a problem in this part of the world. Unfortunately, internet access is restricted on this remote farm so samples must be captured and stored until they can be retrieved. We use [farmOS](https://farmos.org) in this project to retreive the samples from the device and then sync them with the FarmOS database when internet access is availalable.

We have use VSCode and Platform.io extension to compile and upload firmware to the TTGO board. Autodesk Eagle was used to prepare the PCBs.

# Vesoil Sensor

The device is designed to measure current location, soil moisture at two depths, air temperature, air humidity and ground temperature. Sample rate can be configured using Wifi Mode. Samples are not stored, but transmitted via LoRa to the Vesoil Hub.

If you reset the device without holding down the **Mode** button the device enters Sensor mode.

If you hold in the **Mode** button for more that 2 seconds when resetting the device via the reset button the device resets it stored configuration to factory defaults.

When resetting the device via the reset button, if you hold in the **Mode** button for less than 1 seconds during the reboot then the device enters WiFi mode. It will be accessible on http://192.168.4.1

The device will sleep for *lowvoltsleep* (600 seonds) if the voltage is below _txvolts_, regardless of WiFi or Sensor mode.

### Sensor Mode
In Sensor mode the device wakes up every hour (configured using _reportEvery_) and attempts to obtain a GPS signal within _gps_timout_ (60) seconds. If no fix can be obtained the device sleeps for _failedGPSsleep_ (60) seconds and reboots.

During sleep mode the device turns off power to virtually all systems on the TTGO T-Beam, drawing approximately 5 microamps.

### WiFi Mode

In Wifi mode the sensor can be reached via http://192.168.4.1. Configuration parameters can be set in FLASH memory. Additionally, pressing the **Mode** button will also send a LoRa sensor sample.

### Settings

| **Name**        | **Description**           |  **Default** |
| ------------- |-------------| -----:|
| ssid       | Access point name | VESTRONG_S |
| gps_timout | Wait n seconds to get GPS fix |   240 |
| failedGPSsleep | Sleep this long if failed to get GPS  |    360 |
| reportEvery | Get sample every n seconds  |    360 |
| fromHour |  Between these hours | 6  |
| toHour |  Between these hours | 22 |
| frequency | LoRa transmit frequency  | 868MHz  |
| txpower | LoRa transmit power  | 17dB |
| txvolts | Power supply must be delivering this voltage in order to xmit |  2.7 |
| lowvoltsleep | Sleep this long if low on volts | 600  |
| bandwidth | Lower (narrower) bandwidth values give longer range | 125000  |
| speadFactor | Signal processing gain. higher values give greater range  |  12 |
| codingRate |  Extra info for CRC |  6 |
| enableCRC | Enable detection of corrupt packets  |  true |
||||

### Blue light code
The onboard blue LED is used to indicate software state as follows:

| **Description**        | Flashes |
| ------------- |-------------|
| Power too low  | 3 short flashes |
| WiFi Mode  | 2 long flashes |
| Sensor Mode  | 4 short flashes |
| LoRa Transmit  | 1 long flash |
|||

### Sensors
* 1 x DS18B20 for ground temperature
* 2 x [Soil Moisture Sensors](https://www.banggood.com/Capacitive-Soil-Moisture-Sensor-Not-Easy-To-Corrode-Wide-Voltage-Module-For-Arduino-p-1309033.html?rmmds=buy&cur_warehouse=UK) based on 555 chip
* 1 x DHT22 for air temperature and humidity

# Vesoil Hub
This code receives sensor measurements from Vesoil sensors and records them in volatile memory.

The device is designed to capture data packets transmitted from Vesoil sensors and record the in a ring buffer.

If you reset the device without holding down the **Mode** button the device enters Sensor mode.

If you hold in the **Mode** button for more that 2 seconds when resetting the device via the reset button the device resets it stored configuration to factory defaults.

When resetting the device via the reset button, if you hold in the **Mode** button for less than 1 seconds during the reboot then the device enters WiFi mode. It will be accessible on http://192.168.4.1

The device will sleep for *lowvoltsleep* (600 seonds) if the voltage is below _txvolts_, regardless of WiFi or Sensor mode.

### Sensor Mode
In Sensor mode the device wakes up every hour (configured using _reportEvery_) and attempts to obtain a GPS signal within _gps_timout_ (60) seconds. If no fix can be obtained the device sleeps for _failedGPSsleep_ (60) seconds and reboots.

During sleep mode the device turns off power to virtually all systems on the TTGO T-Beam, drawing approximately 5 microamps.

### WiFi Mode

In Wifi mode the sensor can be reached via http://192.168.4.1. Configuration parameters can be set in FLASH memory. Additionally, pressing the **Mode** button will also send a LoRa sensor sample.

### Settings

| **Name**        | **Description**           |  **Default** |
| ------------- |-------------| -----:|
| ssid       | Access point name | VESTRONG_S |
| gps_timout | Wait n seconds to get GPS fix |   240 |
| failedGPSsleep | Sleep this long if failed to get GPS  |    360 |
| reportEvery | Get sample every n seconds  |    360 |
| fromHour |  Between these hours | 6  |
| toHour |  Between these hours | 22 |
| frequency | LoRa transmit frequency  | 868MHz  |
| txpower | LoRa transmit power  | 17dB |
| txvolts | Power supply must be delivering this voltage in order to xmit |  2.7 |
| lowvoltsleep | Sleep this long if low on volts | 600  |
| bandwidth | Lower (narrower) bandwidth values give longer range | 125000  |
| speadFactor | Signal processing gain. higher values give greater range  |  12 |
| codingRate |  Extra info for CRC |  6 |
| enableCRC | Enable detection of corrupt packets  |  true |
||||

### Blue light code
The onboard blue LED is used to indicate software state as follows:

| **Description**        | Flashes |
| ------------- |-------------|
| Power too low  | 3 short flashes |
| WiFi Mode  | 2 long flashes |
| Sensor Mode  | 4 short flashes |
| LoRa Transmit  | 1 long flash |
|||

### Sensors

*None*

# FarmOS Mobile App
Sensor reading collected by the hub can be retrieved by a FarmOS mobile device for onward synchronisation with a farmOS database.

*To be completed*

# Useful Links
## V07 board

https://github.com/LilyGO/TTGO-T-Beam
http://tinymicros.com/wiki/TTGO_T-Beam

## V08 board

https://github.com/Xinyuan-LilyGO/TTGO-T-Beam

## Libraries

https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md

https://arduinojson.org/v6/api/

https://github.com/PaulStoffregen/OneWire

https://github.com/milesburton/Arduino-Temperature-Control-Library

https://farmos.org/development/client/

# TTGO Product Upgrade V07->V8

Lilygo release V8 of the TTGO T-Beam board in August 2019. This rework included several changes mainly targetted at reducing power consumption by using integrating the AXP192 power controller IC. A list of the changes is as follows:

1. User button Pin GPIO39 pin is replaced with GPIO38
2. Replace the charging IC (TP5400) with the power management AXP192
3. GPS TX (12), RX (34) pin replacement
4. Power on switch removed and replaced with push button
5. Reduced sleep current
6. GPS battery replacement

The AXP193 has 4 linear voltage regulators (LDO) and 3 Buck DC-DC converters.

- DCDC1 is connected to DCDC3
- DCDC2 not connected
- DCDC3 now controls the 3.3v to the board and to header 2 Pin7 & pin11 
- LDO2 controls LoRa power
- LDO3 controls GPS power
- AXP Charge Led replaces Led on GPIO

# Power Consumption

Experiments indicate that
- the board draws 5.3mA in deep sleep.
- GPS and LoRa powered off draws ~42mA.
- GPS and LoRa powered on draws ~44mA.
- With GPS searching the board draws ~110mA.
- The blue LED charge light draws 20mA.



