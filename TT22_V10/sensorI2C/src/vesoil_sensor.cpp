// https://github.com/LilyGO/TTGO-T-Beam
// TTGO-07 Pin map http://tinymicros.com/wiki/TTGO_T-Beam
// TTGO-10 https://github.com/Xinyuan-LilyGO/TTGO-T-Beam
//
// to upload new html files use this command:
// pio device list
// pio run --target upload
// pio run --target uploadfs
// pio device monitor -p COM14 -b 115200
// 

static const char * TAG = "Sensor";

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>  
#include <Adafruit_ADS1015.h>
#include <HardwareSerial.h>
#include <LoRa.h>         // https://github.com/sandeepmistry/arduino-LoRa/blob/master/API.md
#include <ArduinoJson.h>  // https://arduinojson.org/v6/api/
#include <Preferences.h>
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>            // https://github.com/PaulStoffregen/OneWire
#include <DallasTemperature.h>  // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <DHT_U.h>
#include <DHT.h>
#include <TBeamPower.h>

#include "TinyGPS.h"
#include <vesoil.h>
#include "vesoil_sensor.h"


OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tmpsensors(&oneWire);
TinyGPSPlus gps;                            
DHT_Unified dht(DHTPIN, DHT11);
Adafruit_ADS1115 ads(ADC_ADDR);
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");
bool wifiMode=false;
struct SensorConfig config;
Preferences preferences;
TBeamPower power(PWRSDA, PWRSCL, BUSPWR, BATTERY_PIN);

void stopLoRa() {
  LoRa.sleep();
  LoRa.end();
  power.power_LoRa(false);
}

void startLoRa() {
  power.power_LoRa(true);
  Serial.printf("Starting Lora: freq:%lu enableCRC:%d coderate:%d spread:%d bandwidth:%lu txpower:%d\n", config.frequency, config.enableCRC, config.codingRate, config.spreadFactor, config.bandwidth, config.txpower);

  SPI.begin(SCK,MISO,MOSI,SS);
  LoRa.setPins(SS,RST,DI0);

  int result = LoRa.begin(config.frequency);
  if (!result) 
    Serial.printf("Starting LoRa failed: err %d\n", result);
  else
    Serial.println("Started LoRa OK");

  LoRa.setPreambleLength(config.preamble);
  LoRa.setSyncWord(config.syncword);    
  LoRa.setSignalBandwidth(config.bandwidth);
  LoRa.setSpreadingFactor(config.spreadFactor);
  LoRa.setCodingRate4(config.codingRate);
  if (config.enableCRC)
      LoRa.enableCrc();
    else 
      LoRa.disableCrc();

  LoRa.setTxPower(config.txpower);
  LoRa.idle();
  
  if (!result) {
    Serial.printf("Starting LoRa failed: err %d", result);
  }  
}

void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do
  {
    while (Serial1.available())
    {
      int c = Serial1.read();
      Serial.printf("%c", (char)c);
      gps.encode(c);
    }
  } while (millis() - start < ms);
}

void setupSerial() { 
  Serial.begin(115200);
  while (!Serial);
  Serial.println();
  Serial.printf("VESTRONG LaPoulton Sensor v%d\n", APP_VERSION);
  power.print_wakeup_reason();
}

void  deepSleep(uint64_t timetosleep) {

  Serial.printf("Preparing sleep mode for %" PRId64  " seconds\n", timetosleep);
  
  stopGPS();

  stopLoRa();

  power.deep_sleep(timetosleep);
}

void ReadGroundTemp(SensorTemp *report) {
  report->value = 0;
  if (!(config.capability & GndTemp))
    return;

  pinMode(ONE_WIRE_BUS,INPUT_PULLUP);
  tmpsensors.begin();
  tmpsensors.setResolution(12);  
  tmpsensors.setCheckForConversion(true);

  for (int i=0;i<3;i++)
  {
    tmpsensors.requestTemperatures();
    delay(100);
    float temp = tmpsensors.getTempCByIndex(0);
    if (temp!=85 && temp!=-127)
      report->value = temp;
  }
}

void ReadAirTempHumidity(SensorAirTmp *report) {
  if (!(config.capability & AirTempHum))
    return;

  // DHT-11
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delay(sensor.min_delay / 1000);

  sensors_event_t event;
  dht.temperature().getEvent(&event);
  
  if (!isnan(event.temperature)) 
  {
    report->airtemp.value = event.temperature;
  }
  if (!isnan(event.relative_humidity)) 
  {
    report->airhum.value = event.relative_humidity;
  }
}
                                                                                 
void ReadMoisture1(SensorMoisture *report)
{
  ads.setGain(GAIN_ONE);
  ads.begin();
  delay(100);

  if (config.capability & Moist1)
  {
    int m1 = ads.readADC_SingleEnded(0);
    report->value = m1;
  }
}

void ReadMoisture2(SensorMoisture *report)
{
  ads.setGain(GAIN_ONE);
  ads.begin();
  delay(100);

  if (config.capability & Moist2)
  {
    int m1 = ads.readADC_SingleEnded(0);
    report->value = m1;
  }
}

void ReadGPSData(SensorGps *report)
{
  if (!(config.capability & GPS))
      return;

  struct tm curtime;
  curtime.tm_sec = gps.time.second();
  curtime.tm_min=gps.time.minute();
  curtime.tm_hour= gps.time.hour();
  curtime.tm_mday= gps.date.day();
  curtime.tm_mon= gps.date.month()-1;
  curtime.tm_year= gps.date.year()-1900;
  curtime.tm_isdst=false;

  report->time = mktime(&curtime);
  report->lat = gps.location.lat();
  report->lng = gps.location.lng();
  report->alt = gps.altitude.meters();
  report->sats = gps.satellites.value();
  report->hdop = gps.hdop.value();
}

void ReadDistance(SensorDistance *report) {
  report->value = 0;
  if (!(config.capability & Distance))
    return;

    // Define inputs and outputs
  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  digitalWrite(TRIGPIN, LOW);   // Clear the TRIGPIN by setting it LOW:
  delayMicroseconds(2); 
  digitalWrite(TRIGPIN, HIGH);  // Trigger the sensor by setting the TRIGPIN high for 10 microseconds:
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  
  // Read the ECHOPIN. pulseIn() returns the duration (length of the pulse) in microseconds:
  long duration = pulseIn(ECHOPIN, HIGH);
  
  report->value = (float)(duration / 58.0);
}

void ReadVolts(SensorVoltage * report) {
  report->value = power.get_battery_voltage();
}

void getSample(SensorReport *report) {
  ReadGPSData(&report->gps);
  ReadVolts(&report->volts);
  ReadGroundTemp( &report->gndTemp );
  ReadDistance( &report->distance );
  ReadAirTempHumidity( &report->airTempHumidity);
  esp_efuse_mac_get_default(report->id.id);
  ReadMoisture1(&report->moist1);
  ReadMoisture2(&report->moist2);
}


void startGPS() {
  if (!(config.capability & GPS))
    return;

  power.power_GPS(true);
  Serial1.begin(GPSBAUD, SERIAL_8N1, GPSRX, GPSTX);
  Serial.println("Wake GPS");
  int data = -1;
  do {
    for(int i = 0; i < 20; i++){ //send random to trigger respose
        Serial1.write(0xFF);
      }
    data = Serial1.read();
  } while(data == -1);
  Serial.println("GPS is awake");
}

void stopGPS() {
  if (!(config.capability & GPS))
    return;

  const byte CFG_RST[12] = {0xb5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x01,0x00, 0x0F, 0x66};//Controlled Software reset
  const byte RXM_PMREQ[16] = {0xb5, 0x62, 0x02, 0x41, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x4d, 0x3b};   //power off until wakeup call
  Serial1.write(CFG_RST, sizeof(CFG_RST));
  delay(600); //give some time to restart //TODO wait for ack
  Serial1.write(RXM_PMREQ, sizeof(RXM_PMREQ));
  power.power_GPS(false);
}

GPSLOCK getGpsLock() {
  if (!(config.capability & GPS))
    return LOCK_DISABLED;

  for (int i=0; i<config.gps_timeout; i++)
  {
    
    Serial.printf("waiting for GPS try: %d  Age:%u  valid: %d   %d\n", i, gps.location.age(), gps.location.isValid(), gps.time.isValid());

    // check whethe we have  gps sig
    if (gps.location.lat()!=0 && gps.location.isValid() )
    {
      // in the report window?
      if (gps.time.hour() >=config.fromHour && gps.time.hour() < config.toHour)
        return LOCK_OK;          
      else
        return LOCK_WINDOW;
    }

    power.flashlight(1);    

    smartDelay(1000);
  }
  return LOCK_FAIL;
}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

String processor(const String& var)
{
  if(var == "vbatt")
    return String(power.get_battery_voltage());
  if(var == "SSID")
    return config.ssid;
  if(var == "failedGPSsleep")
    return String(config.failedGPSsleep);
  if(var == "frequency")
    return String(config.frequency);
  if(var == "fromHour")
    return String(config.fromHour);
  if(var == "gps_timeout")
    return String(config.gps_timeout);
  if(var == "lowvoltsleep")
    return String(config.lowvoltsleep);
  if(var == "password")
    return config.password;
  if(var == "reportEvery")
    return String(config.reportEvery);
  if(var == "spreadFactor")
    return String(config.spreadFactor);
  if(var == "toHour")
    return String(config.toHour);
  if(var == "txpower")
    return String(config.txpower);
  if(var == "txvolts")
    return String(config.txvolts);
  if(var == "enableCRC")
    return String(config.enableCRC?"checked":"");
  if(var == "codingRate")
    return String(config.codingRate);
  if(var == "bandwidth")
    return String(config.bandwidth);
    
  return String();
}

void setConfigParam(const String& var, const char *value) {
  Serial.printf("param %s %s\n", var.c_str(), value);

  if(var == "SSID")
    memcpy(config.ssid, value, strlen(value));
  if(var == "password")
    memcpy(config.password, value, strlen(value));
  if(var == "failedGPSsleep")
    config.failedGPSsleep = atoi(value);
  if(var == "frequency")
    config.frequency = atol(value);
  if(var == "fromHour")
    config.fromHour = atoi(value);
  if(var == "gps_timeout")
    config.gps_timeout = atoi(value);
  if(var == "lowvoltsleep")
    config.lowvoltsleep = atoi(value);
  if(var == "reportEvery")
    config.reportEvery = atoi(value);
  if(var == "spreadFactor")
    config.spreadFactor = atoi(value);
  if(var == "toHour")
    config.toHour = atoi(value);
  if(var == "txpower")
    config.txpower = atoi(value);
  if(var == "txvolts")
    config.txvolts = atof(value);
  if(var == "enableCRC")
    config.enableCRC = strcmp(value, "on")==0;
  if(var == "codingRate")
    config.codingRate = atoi(value);
  if(var == "bandwidth")
    config.bandwidth = atol(value);
}

void setupWifi() {
    if (!SPIFFS.begin())
    {
      Serial.println("Failed to initialise SPIFFS");
    }

    if (strlen(config.password)==0)
      WiFi.softAP(config.ssid);
    else
      WiFi.softAP(config.ssid, config.password);

    Serial.println("IP Address:");
    Serial.println(WiFi.softAPIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        Serial.printf("Web request %s\n", request->url().c_str());
        request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Send a GET request to <IP>/get?message=<message>
    server.on("/api", HTTP_GET, [] (AsyncWebServerRequest *request) {
        
        String reply = "{ \"config\": [";
        
        char *msg = (char *)malloc(512);
        sprintf(msg, "{ \"ssid\": \"%s\", \"gpstimeout\": %d, \"gpssleep\": %d, \"fromHour\": %d, \"toHour\": %d, \"reportfreq\":%d, \"frequency\":%lu, \"txpower\":%d, \"txvolts\":%f, \"volts\":%f }\n", 
                 config.ssid, config.gps_timeout, config.failedGPSsleep, config.fromHour, config.toHour, config.reportEvery,config.frequency,config.txpower,config.txvolts, power.get_battery_voltage() );
        reply += msg;
        reply += " ] }\n";
        free(msg);
        request->send(200, "text/plain",  reply);
    });

    // post form
    server.on("/setconfig", HTTP_POST, [] (AsyncWebServerRequest *request) {
      Serial.printf("root post %s", request->url().c_str());
      config.enableCRC=false;

      for (int i=0; i< request->params(); i++)
      {
        AsyncWebParameter *p = request->getParam(i);
        setConfigParam(p->name(), p->value().c_str());
      }
      
      preferences.putBytes("config", &config, sizeof(SensorConfig));
      stopLoRa();
      startLoRa();
      request->send(SPIFFS, "/index.html", String(), false, processor);
    });

    server.onNotFound(notFound);

    server.begin();
}

STARTUPMODE getStartupMode() {
  const int interval = 100;
  STARTUPMODE startup_mode = NORMAL;

  // get startup by detecting how many seconds the button is held
  int btndown = 0;
  while (digitalRead(BTN1)==0)
  {
    delay(interval);
    btndown += interval;
  }
  
  if (btndown==0)
    startup_mode = NORMAL;   // sensor mode
  else if (btndown<2000)
    startup_mode = WIFI;   // wifi
  else 
  {
    startup_mode = RESET;   // reset config
    power.flashlight(ERR_LOWPOWER);
  }

  Serial.printf("button held for %d mode is %d\n", btndown, startup_mode );

  return startup_mode;
}

void getConfig(STARTUPMODE startup_mode) {
  preferences.begin(TAG, false);

  // if we have a stored config and we're not resetting, then load the config
  if (preferences.getBool("configinit") && startup_mode != RESET)
  {
    preferences.getBytes("config", &config, sizeof(SensorConfig));
  }
  else
  {
    // we're resetting the config or there isn't one
    preferences.putBytes("config", &default_config, sizeof(SensorConfig));
    preferences.putBool("configinit", true);
    memcpy( &config, &default_config, sizeof(SensorConfig));
  }
  Serial.printf("Capability is %d\n", config.capability);
}

void getSampleAndSend() {
  // get GPS and then gather/send a sample if within the time window
  // best not to send at night as we drain the battery
  SensorReport report;
  memset( &report, 0, sizeof(report));
  report.capability = config.capability;
  
  power.power_sensors(true);   // turn on power to the sensor bus
  startLoRa();
  delay(10);  
  getSample(&report);
  power.power_sensors(false);   // turn off power to the sensor bus

  char *stime = asctime(gmtime(&report.gps.time));
  stime[24]='\0';
  Serial.printf("%02X%02X%02X%02X%02X%02X,", (report.id).id[0],report.id.id[1],report.id.id[2],report.id.id[3],report.id.id[4],report.id.id[5]);

  Serial.printf("%s %f/%f alt=%f sats=%d hdop=%d gt=%f at=%f ah=%f m1=%d m2=%d v=%f d=%f\n",
  stime, report.gps.lat, report.gps.lng ,report.gps.alt, report.gps.sats , report.gps.hdop,
  report.gndTemp.value,report.airTempHumidity.airtemp.value,report.airTempHumidity.airhum.value ,
  report.moist1.value ,report.moist2.value, report.volts.value, report.distance.value );
  
  // send packet
  power.led_onoff(true);

  LoRa.beginPacket();
  Serial.println("LoRa begin");
  LoRa.write( (const uint8_t *)&report, sizeof(SensorReport));
  Serial.println("LoRa Write");
  LoRa.endPacket();
  Serial.println("LoRa End");

  power.led_onoff(false);

  stopLoRa();
}

void setup() {

  setupSerial();  

  digitalWrite(BUSPWR, LOW); // turn off power to the sensor bus

  power.begin();
  power.power_sensors(false);
  power.power_peripherals(false);

  pinMode(BTN1,INPUT);         // Button 1

  STARTUPMODE startup_mode = getStartupMode();

  // check we have enough juice
  float currentVoltage = power.get_battery_voltage();
  if (currentVoltage!=0 && currentVoltage<config.txvolts)
  {
    Serial.printf("Battery Voltage too low: %f\n", currentVoltage);
    power.flashlight(ERR_LOWPOWER);
    deepSleep(config.lowvoltsleep);
  } 
  else
    Serial.printf("Battery Voltage OK: %f > %f\n", currentVoltage, config.txvolts);
    
  getConfig(startup_mode);

  startGPS();

  if (startup_mode==WIFI)
  {
    power.flashlight(INFO_WIFI);
    Serial.printf("Entering WiFi mode\n");
    wifiMode=true;
    setupWifi();
  }
  else
    power.flashlight(INFO_SENSOR);
  
  Serial.printf("{ \"ssid\": \"%s\", \"gpstimeout\": %d, \"gpssleep\": %d, \"fromHour\": %d, \"toHour\": %d, \"reportfreq\":%d, \"frequency\":%lu, \"txpower\":%d, \"txvolts\":%f, \"volts\":%f }\n", 
          config.ssid, config.gps_timeout, config.failedGPSsleep, config.fromHour, config.toHour, config.reportEvery,config.frequency,config.txpower,config.txvolts, currentVoltage );

  Serial.printf("End of setup - sensor packet size is %u\n", sizeof(SensorReport));
}

void loopWifiMode() {
  GPSLOCK lock = getGpsLock();

  Serial.printf("GPS Lock is  %d", lock);

  // mode button held down
  if (digitalRead(BTN1)!=0)
  {
    if (lock==LOCK_OK || lock==LOCK_DISABLED)
      getSampleAndSend();
    else
      power.flashlight(INFO_NOGPS);

    smartDelay(2);
    return;
  }
  else
  {
    power.flashlight(INFO_WIFI);
    smartDelay(2);
    return;
  }
}

void loopSensorMode() {
  GPSLOCK lock = getGpsLock();
  Serial.printf("GPS Lock is  %d @ hour %d\n", lock, gps.time.hour());

  switch (lock)
  {
    case LOCK_DISABLED:
    case LOCK_OK:
      getSampleAndSend();
      deepSleep((uint64_t)config.reportEvery);

    case LOCK_FAIL:
      // GPS failed - try again in the future
      deepSleep((uint64_t)config.failedGPSsleep);

    case LOCK_WINDOW:
      // not in report window - calc sleep time
      long timeToSleepSecs=0;
      uint curtime = gps.time.hour();
      
      if (curtime>=config.toHour)   // before midnight
      {
        timeToSleepSecs = ((24-gps.time.hour()) + config.fromHour) * 60 * 60;
      }
      else  // after minight
      {
        timeToSleepSecs = (config.fromHour - gps.time.hour()) * 60 * 60;
      }

      deepSleep( timeToSleepSecs );
  }
}

void loop() {

  // no sampling during wifi mode if btn not held down
  if (wifiMode==true)
    loopWifiMode();
  else
    loopSensorMode();
}
