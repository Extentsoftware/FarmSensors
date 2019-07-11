**Vesoil Farm Monitoring**

This project is for remote soil moisture, ground temperature, air temperature and humidity based on the TTG T-Beam board.

Use VSCode and Platform.io extension to compile and upload firmware to the TTGO board.

**Vesoil Hub**
This code receives sensor measurements from Vesoil sensors and records them in volatile memory.


**Vesoil Sensor**

Wakes up every hour (or whatever is confiured) and attempts to obtain a GPS signal within _gps_timout_ (60) seconds. If this can't be acheievd then the device sleeps for _failedGPSsleep_ (60) seconds and tries again.            


