#include "mqtt_gsm.h"

#define SerialMon Serial

MqttGsmClient::MqttGsmClient() 
  : Seri alAT(AT_RX, AT_TX, false)
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
  TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,9600);
  delay(1000); 

  Serial.println("modem restarting");
  _modem.restart();  
  Serial.println("modem restarted");
  delay(1000);  
  
  _gsmStage = "Radio";
  modem_state=MODEM_NOT_CONNECTED;
}

void MqttGsmClient::waitForNetwork() 
{
  static int counter = 0;
  counter += 1;
  if ( (counter % 1000) == 0)
  {
    uint8_t chargeState;
    int8_t percent;
    uint16_t milliVolts;
    _modem.getBattStats(chargeState, percent, milliVolts);
    Serial.printf("Signal Quality %s Chrg %d Percent %d Volts %d\n", String(_modem.getSignalQuality(), DEC).c_str(), chargeState, percent, milliVolts);
    if (_modem.isNetworkConnected()) 
    { 
      _gsmStage = Msg_Network;
      _gsmStatus = Msg_Connected;
      _displayUpdate();
      modem_state=MODEM_CONNECTED;
      Serial.printf("Connected to GSM network\n");
    }
  }
}

void MqttGsmClient::doGPRSConnect()
{
  Serial.println("GPRS Connect");

  _gsmStage = Msg_GPRS;
  _gsmStatus = Msg_Connecting;
  _displayUpdate();
    
  bool connected = _modem.gprsConnect(_apn, _gprsUser, _gprsPass);    

  _gsmStatus = (char*)(connected ? Msg_Connected : Msg_Failed);
  _displayUpdate();

  if (connected) 
  { 
    Serial.println("GPRS Connected");
    modem_state=GPRS_CONNECTED;
  }
}

void MqttGsmClient::mqttGPRSConnect()
{
  char topic[32];

  if (!_mqttGPRS.connected())
  {
    _gsmStage = Msg_MQTT;
    _gsmStatus = Msg_Connecting;
    _displayUpdate();
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
  }
}  

void MqttGsmClient::mqttGPRSPoll()
{
  static int counter = 0;
  counter += 1;
  if ( (counter % 1000) == 0)
  {
    //String dt = modem.getGSMDateTime( DATE_FULL );
    Serial.printf("Poll..");
    bool gprsConnected = _modem.isGprsConnected();
    bool netConnected = _modem.isNetworkConnected();
    bool clientConnected = _client.connected();
    Serial.printf("Signal Quality %s Connections network=%d GPRS=%d client=%d\n", String(_modem.getSignalQuality(), DEC).c_str(), netConnected, gprsConnected, clientConnected);
    if (!gprsConnected || !clientConnected)
    {
      Serial.printf("GPRS Disconnected\n");
      modem_state=MODEM_NOT_CONNECTED;
    }
  }
  _mqttGPRS.loop();
}

void MqttGsmClient::ModemCheck()
{
  static int counter = 0;
  counter += 1;
  if ( (counter % 1000) == 0)
  {
    Serial.printf("S=%d ",modem_state);
  }

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
}