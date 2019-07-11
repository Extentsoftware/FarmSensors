**Vesoil Farm Monitoring**

This project is for remote soil moisture, ground temperature, air temperature and humidity based on the TTG T-Beam board.

Use VSCode and Platform.io extension to compile and upload firmware to the TTGO board.

**Vesoil Hub**
This code receives sensor measurements from Vesoil sensors and records them in volatile memory.


**Vesoil Sensor**

if you reset the device without holding down the GPIO39 button the device enters Sensor mode.

When resetting the device via the reset button, if you hold in the GPIO39 button for more that 2 seconds during the reboot then the device resets it stored configuration to factory defaults.

When resetting the device via the reset button, if you hold in the GPIO39 button for less than 1 seconds during the reboot then the device enters WiFi mode. It will be accessible on http://192.168.4.1

The device will sleep for *lowvoltsleep* (600 seonds) if the voltage is below _txvolts_, regardless of WiFi or Sensor mode.

**Sensor Mode**
In Sensor mode the device wakes up every hour (configured using _reportEvery_) and attempts to obtain a GPS signal within _gps_timout_ (60) seconds. If no fix can be obtained the device sleeps for _failedGPSsleep_ (60) seconds and reboots.

During sleep mode the device turns off power to virtually all systems on the TTGO T-Beam, drawing approximately 5 microamps.

**WiFi Mode**

In Wifi mode the sensor can be reached via http://192.168.4.1. Configuration parameters can be set in FLASH memory. Additionally, pressing the GPIO39 button will also send a LoRa sensor sample.




