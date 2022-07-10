#include "mqtt_gsm.h"

#define TINY_GSM_DEBUG SerialMon
#define AT_RX        26
#define AT_TX        27

MqttGsmClient::MqttGsmClient() 
{
    Serial1.begin(9600, SERIAL_8N1, AT_RX, AT_TX);
    while (!Serial1);

    debugger = new StreamDebugger(Serial1, Serial);
    //_modem = new TinyGsm(*debugger);
    _modem = new TinyGsm(Serial1);

    _client = new TinyGsmClient(*_modem);
    _mqttGPRS = new PubSubClient(*_client);

    memset(&status, 0, sizeof(status));
}

void MqttGsmClient::init(
  char *broker, 
  char *macStr, 
  char *apn,
  char *gprsUser,
  char *gprsPass,
  char *simPin,
  std::function<void(char*, byte*, unsigned int)> callback_mqtt,
  std::function<void(MqttGsmStats *)> callback_status
)
{
  _broker = broker;
  _macStr = macStr;
  _callback_mqtt = callback_mqtt;
  _callback_status = callback_status;
  _apn=apn;
  _gprsUser=gprsUser;
  _gprsPass=gprsPass;  
  _simPin = simPin;

  Serial.printf("init:: GSM Broker %s apn %s\n", broker, apn);
  Serial.printf("init:: Set MQTT Broker\n");    
  _mqttGPRS->setServer(_broker, 1883);
  Serial.printf("init:: Set MQTT callback\n");    
  _mqttGPRS->setCallback(_callback_mqtt);  

}

void MqttGsmClient::doModemStart()
{  
  Serial.printf("doModemStart::entered\n");
  _gsmStage = "Modem";
  _gsmStatus = "Starting";

  modemPowerOn();

  Serial.println("doModemStart:: Initializing modem...");
  if (!_modem->init()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  if (_simPin != NULL && strlen(_simPin)>0)
  {
    Serial.printf("Unlocking with pin: %s\n",_simPin);
    _modem->simUnlock(_simPin);
  }

  uint8_t network[] = {
      2,  /*Automatic*/
      13, /*GSM only*/
      38, /*LTE only*/
      51  /*GSM and LTE only*/
  };  
  for (int i = 0; i <= 4; i++) {

      Serial.printf("Try method '%d' \n", network[i]);
      _modem->setNetworkMode(network[i]);
      delay(3000);
      bool isConnected = false;
      int tryCount = 60;
      while (tryCount--) {
          int16_t signal =  _modem->getSignalQuality();
          Serial.print("Signal: ");
          Serial.print(signal);
          Serial.print(" ");
          Serial.print("isNetworkConnected: ");
          isConnected = _modem->isNetworkConnected();
          Serial.println( isConnected ? "CONNECT" : "NO CONNECT");
          if (isConnected) {
              break;
          }
          delay(1000);
      }
      if (isConnected) {
          break;
      }
  }

  _modem->setPreferredMode(3); // cat-m & nb-iot
  delay(500);

  _gsmStage = "Radio";
  modem_state=MODEM_NOT_CONNECTED;
}

void MqttGsmClient::waitForNetwork() 
{  
  if (status.netConnected) 
  { 
    _gsmStage = Msg_Network;
    _gsmStatus = Msg_Connected;
    modem_state=MODEM_CONNECTED;
  }
}

void MqttGsmClient::doGPRSConnect()
{
  Serial.printf("doGPRSConnect::entered\n");

  _gsmStage = Msg_GPRS;
  _gsmStatus = Msg_Connecting;
  
  Serial.printf("doGPRSConnect:: Attempt GPRS Connect APN %s user %s pwd %s\n", _apn, _gprsUser, _gprsPass);  
  bool connected = _modem->gprsConnect(_apn, _gprsUser, _gprsPass);    
  Serial.printf("doGPRSConnect:: GPRS Connected..%s\n", connected?"yes":"no");

  _gsmStatus = (char*)(connected ? Msg_Connected : Msg_Failed);

  if (connected) 
  { 
    modem_state=GPRS_CONNECTED;
  }
}

void MqttGsmClient::mqttGPRSConnect()
{
  Serial.printf("mqttGPRSConnect::entered\n");
  char topic[64];
  if (!status.mqttConnected)
  {
    _gsmStage = Msg_MQTT;
    _gsmStatus = Msg_Connecting;
    Serial.printf("mqttGPRSConnect:: MQTT Connecting\n");
    boolean mqttConnected = _mqttGPRS->connect(_macStr);
    Serial.printf("mqttGPRSConnect:: MQTT Connected..%s\n", mqttConnected?"yes":"no");

    if (!mqttConnected)
    {
      _gsmStatus = Msg_FailedToConnect;
    }
    else
    {
      sprintf(topic, "bongo/%s/hub", _macStr);
      Serial.printf("mqttGPRSConnect:: Subscribing..%s\n", topic);
      _mqttGPRS->subscribe(topic);  
      modem_state=MQ_CONNECTED;
      _callback_status( &status );
    }
  }
}  

String MqttGsmClient::getGsmStage()
{
  return _gsmStage;
}

String MqttGsmClient::getGsmStatus()
{
  return _gsmStatus;
}

bool MqttGsmClient::sendMQTTBinary(uint8_t *report, int packetSize, char *subtopic)
{
  Serial.printf("sendMQTTBinary::entered\n");
  char topic[32];
  sprintf(topic, "bongo/%s/%s", _macStr,subtopic);

  if (_mqttGPRS->connected())
  {
      if (!_mqttGPRS->publish(topic, report, packetSize))
      {
        Serial.printf("MQTT GPRS Fail\n");
      } 
      else
      {
        Serial.printf("MQTT Sent GPRS %d bytes to %s\n", packetSize, topic);
      }
      return true;
  }
  return false;
}

void MqttGsmClient::mqttGPRSPoll()
{
  if (!status.gprsConnected || !status.mqttConnected)
  {
    Serial.printf("GPRS Disconnected\n");
    modem_state=MODEM_NOT_CONNECTED;
  }
  
  bool result = _mqttGPRS->loop();
  
  if (!result)
  {
    modem_state = MODEM_INIT;
  }
}

void MqttGsmClient::getStatus(MqttGsmStats& stats) 
{
    memcpy(&stats, &status, sizeof(status));
}

bool MqttGsmClient::updateStatus()
{
    MqttGsmStats newStatus;
    
    newStatus.gpsOn = status.gpsOn;
    newStatus.lat = status.lat;
    newStatus.lon = status.lon;
    newStatus.speed = status.speed;
    newStatus.alt = status.alt;
    newStatus.gpsfix = status.gpsfix;
    newStatus.vs = status.vs;
    newStatus.us = status.us;

    newStatus.signalQuality = _modem->getSignalQuality();
    newStatus.gprsConnected = false;
    newStatus.mqttConnected = false;
    _modem->getBattStats(newStatus.chargeState, newStatus.percent, newStatus.gsmVolts);
        
    newStatus.netConnected = _modem->isNetworkConnected();

    if (newStatus.netConnected)
    {
      newStatus.gprsConnected = _modem->isGprsConnected();
      if (newStatus.gprsConnected)
      {
        newStatus.mqttConnected = _mqttGPRS->connected();
      }
    }
    memcpy(&status, &newStatus, sizeof(status));

    // status changed?
    bool changed = (newStatus.netConnected!=status.netConnected || newStatus.gprsConnected!=status.gprsConnected ||newStatus.mqttConnected!=status.mqttConnected);
    return changed;
}

void MqttGsmClient::printStatus()
{
  Serial.printf("updateStatus:: state=%d network=%d GPRS=%d MQTT=%d Signal Quality %d Chrg %d Percent %d Volts %d gps: fix=%s lat/lon %f/%f speed %f alt %f vs %d us %d\n"
    , modem_state
    , status.netConnected
    , status.gprsConnected
    , status.mqttConnected
    , status.signalQuality
    , status.chargeState
    , status.percent
    , status.gsmVolts
    , status.gpsfix?"Y":"N"
    , status.lat
    , status.lon
    , status.speed
    , status.alt
    , status.vs
    , status.us
  );
}

void MqttGsmClient::enableGPS()
{
    // Set SIM7000G GPIO4 LOW ,turn on GPS power
    // CMD:AT+SGPIO=0,4,1,1
    // Only in version 20200415 is there a function to control GPS power
    _modem->sendAT("+SGPIO=0,4,1,1");
    if (_modem->waitResponse(10000L) != 1) {
        Serial.printf("enableGps power failed\n");
        DBG(" SGPIO=0,4,1,1 false ");
    }
    status.gpsOn = _modem->enableGPS();
    Serial.printf("gps: enabled success=%s\n", status.gpsOn?"Y":"N");
} 

void MqttGsmClient::disableGPS()
{
    // Set SIM7000G GPIO4 LOW ,turn off GPS power
    // CMD:AT+SGPIO=0,4,1,0
    // Only in version 20200415 is there a function to control GPS power
    _modem->sendAT("+SGPIO=0,4,1,0");
    if (_modem->waitResponse(10000L) != 1) {
        DBG(" SGPIO=0,4,1,0 false ");
    }
    _modem->disableGPS();
    Serial.printf("gps: disabled\n");
    status.gpsOn = false;
}


bool MqttGsmClient::GetGps()
{
  if (!status.gpsOn)
    enableGPS();
  status.gpsfix = _modem->getGPS(&status.lat, &status.lon, &status.speed, &status.alt, &status.vs, &status.us);
  return status.gpsfix;
}

bool MqttGsmClient::ModemCheck()
{
  static int counter = 0;
  static int gps_counter = 9999;
  static int gps_fail_count =0;
  bool changed = false;
  counter += 1;
  if ( (counter % check_poll_rate) == 0)
  {
    printStatus();
    if (modem_state!=MODEM_INIT)
      changed = updateStatus();    
    
    switch(modem_state)
    {
      case MODEM_INIT:
        doModemStart();
        break;
      case MODEM_NOT_CONNECTED:
        waitForNetwork();
        break;
      case MODEM_CONNECTED:
        doGPRSConnect();
        break;
      case GPRS_CONNECTED:
        mqttGPRSConnect();
        break;      
      case MQ_CONNECTED:
        _gsmStatus = Msg_Connected;
        mqttGPRSPoll();
        ++gps_counter;
        if (gps_counter > get_gps_rate)
        {
          Serial.printf("gps_counter %d\n",gps_counter);
          gps_counter=0;
          gps_fail_count=0;
          modem_state = START_GPS;
        }
        break;
      case START_GPS:
        mqttGPRSPoll();
        enableGPS();
        if (status.gpsOn)
        {
          gps_fail_count=0;
          modem_state = GET_GPS;
        }
        else
        {
          ++ gps_fail_count;
          if (gps_fail_count > start_gps_fail_max)
            modem_state = MQ_CONNECTED;
        }
        break;
      case GET_GPS:
        mqttGPRSPoll();
        bool success = GetGps();
        if (success)
        {
          _callback_status( &status );
          disableGPS();
          modem_state = MQ_CONNECTED;
        }
        else
        {
          ++ gps_fail_count;
          if (gps_fail_count > get_gps_fail_max)
            modem_state = MQ_CONNECTED;
        }
        break;      
    }
  }

  return isConnected();
}

bool MqttGsmClient::isConnected()
{
  return (modem_state == MQ_CONNECTED);
}

void MqttGsmClient::modemPowerOn()
{
    Serial.printf("modemPowerOn:: GSM Power On\n");  
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(3000);    //Datasheet Ton mintues = 1S
    digitalWrite(PWR_PIN, HIGH);
}

void MqttGsmClient::modemPowerOff()
{
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1500);    //Datasheet Ton mintues = 1.2S
    digitalWrite(PWR_PIN, HIGH);
}

void MqttGsmClient::modemRestart()
{
    modemPowerOff();
    delay(1000);
    modemPowerOn();
}  