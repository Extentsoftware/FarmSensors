#include "mqtt_gsm.h"

MqttGsmClient::MqttGsmClient(int rx, int tx) 
  : SerialAT(rx, tx, false)
  , debugger(SerialAT, Serial)
  , _modem(debugger)
  , _client(_modem)
  , _mqttGPRS(_client)
{
  
}

void MqttGsmClient::init(
  char *broker, 
  char *macStr, 
  char *apn,
  char *gprsUser,
  char *gprsPass,
  std::function<void(char*, byte*, unsigned int)> callback, 
  std::function<void()> displayUpdate)
{
  _broker = broker;
  _macStr = macStr;
  _callback = callback;
  _apn=apn;
  _gprsUser=gprsUser;
  _gprsPass=gprsPass;
  _displayUpdate = displayUpdate;
  _mqttGPRS.setServer(_broker, 1883);
  _mqttGPRS.setCallback(_callback);  
  Serial.printf("GSM Broker %s apn %s\n", broker, apn);
}

String MqttGsmClient::getGsmStage()
{
  return _gsmStage;
}

String MqttGsmClient::getGsmStatus()
{
  return _gsmStatus;
}

bool MqttGsmClient::sendMQTTBinary(uint8_t *report, int packetSize)
{
  char topic[32];
  sprintf(topic, "bongo/%s/sensor", _macStr);

  if (_mqttGPRS.connected())
  {
      if (!_mqttGPRS.publish(topic, report, packetSize))
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

void MqttGsmClient::doModemStart()
{  
  _gsmStage = "Modem";
  _gsmStatus = "Starting";
  _displayUpdate();
  uint32_t rate = TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,19200);

  Serial.printf("modem baud rate %d\n", rate);
  if (rate==0)
    return;

  bool restarted = _modem.restart();  
  Serial.printf("modem restarted %d\n", restarted?1:0);
  if (!restarted)
    return;
  
  _gsmStage = "Radio";
  modem_state=MODEM_NOT_CONNECTED;
}

void MqttGsmClient::waitForNetwork() 
{  
  if (status.netConnected) 
  { 
    _gsmStage = Msg_Network;
    _gsmStatus = Msg_Connected;
    _displayUpdate();
    modem_state=MODEM_CONNECTED;
    Serial.printf("Connected to GSM network\n");
  }
}

void MqttGsmClient::doGPRSConnect()
{
  Serial.printf("GPRS Connect APN %s user %s pwd %s\n", _apn, _gprsUser, _gprsPass);

  _gsmStage = Msg_GPRS;
  _gsmStatus = Msg_Connecting;
  _displayUpdate();
    
  bool connected = _modem.gprsConnect(_apn, _gprsUser, _gprsPass);    

  _gsmStatus = (char*)(connected ? Msg_Connected : Msg_Failed);

  if (connected) 
  { 
    Serial.println("GPRS Connected");
    modem_state=GPRS_CONNECTED;
  }
  updateStatus();
}

void MqttGsmClient::mqttGPRSConnect()
{
  char topic[32];

  updateStatus();
  if (!status.mqttConnected)
  {
    _gsmStage = Msg_MQTT;
    _gsmStatus = Msg_Connecting;
    boolean mqttConnected = _mqttGPRS.connect(_macStr);
    if (!mqttConnected)
    {
      _gsmStatus = Msg_FailedToConnect;
    }
    else
    {
      sprintf(topic, "bongo/%s/hub", _macStr);
      _mqttGPRS.subscribe(topic);  
      _mqttGPRS.publish(topic, "{\"state\":\"connected\"}", true);
      Serial.println("MQTT connected via GPRS");
      modem_state=MQ_CONNECTED;
    }
    updateStatus();
  }
}  

void MqttGsmClient::mqttGPRSPoll()
{
  if (!status.gprsConnected || !status.mqttConnected)
  {
    Serial.printf("GPRS Disconnected\n");
    modem_state=MODEM_NOT_CONNECTED;
  }
  bool result = _mqttGPRS.loop();
  if (!result)
  {
    modem_state = MODEM_INIT;
  }
}

void MqttGsmClient::getStats(MqttGsmStats& stats) 
{
    memcpy(&stats, &status, sizeof(status));
}

bool MqttGsmClient::updateStatus()
{
    Serial.printf("gsm status check\n");
    MqttGsmStats newStatus;
    _modem.getBattStats(newStatus.chargeState, newStatus.percent, newStatus.gsmVolts);
    newStatus.signalQuality = _modem.getSignalQuality();
    newStatus.gprsConnected = _modem.isGprsConnected();
    newStatus.netConnected = _modem.isNetworkConnected();
    newStatus.mqttConnected = _mqttGPRS.connected();

    // status changed?
    bool changed = memcmp(&newStatus, &status, sizeof(newStatus))!=0;
    if (changed)
    {
      memcpy(&status, &newStatus, sizeof(status));
      Serial.printf("network=%d GPRS=%d client=%d Signal Quality %d Chrg %d Percent %d Volts %d\n"
          , status.signalQuality
          , status.gprsConnected
          , status.mqttConnected
          , status.signalQuality
          , status.chargeState
          , status.percent
          , status.gsmVolts
      );
    }
    if (changed)
      _displayUpdate();
    return changed;
}

bool MqttGsmClient::ModemCheck()
{
  static int counter = 0;
  if ( (counter % 5) == 0)
  {
    updateStatus();    
  }

  counter += 1;
  
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
      break;      
  }

  return isConnected();
}

bool MqttGsmClient::isConnected()
{
  return (modem_state == MQ_CONNECTED);
}
