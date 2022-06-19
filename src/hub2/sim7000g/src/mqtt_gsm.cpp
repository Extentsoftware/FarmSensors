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
  std::function<void(char*, byte*, unsigned int)> callback)
{
  _broker = broker;
  _macStr = macStr;
  _callback = callback;
  _apn=apn;
  _gprsUser=gprsUser;
  _gprsPass=gprsPass;  
  _simPin = simPin;

  Serial.printf("init:: GSM Broker %s apn %s\n", broker, apn);
  Serial.printf("init:: Set MQTT Broker\n");    
  _mqttGPRS->setServer(_broker, 1883);
  Serial.printf("init:: Set MQTT callback\n");    
  _mqttGPRS->setCallback(_callback);  

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
      Serial.printf("mqttGPRSConnect:: publishing ..%s\n", topic);
      _mqttGPRS->publish(topic, "{\"state\":\"connected\"}", true);
      modem_state=MQ_CONNECTED;
      Serial.printf("mqttGPRSConnect:: Published..%s\n", topic);
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

bool MqttGsmClient::sendMQTTBinary(uint8_t *report, int packetSize)
{
  Serial.printf("sendMQTTBinary::entered\n");
  char topic[32];
  sprintf(topic, "bongo/%s/sensor", _macStr);

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
    newStatus.signalQuality = _modem->getSignalQuality();
    newStatus.gprsConnected = false;
    newStatus.mqttConnected = false;

    //_modem->sendAT("+CSQ");
    //_modem->sendAT("+COPS?");
    //_modem->sendAT("+COPS=?");
    //delay(30000);
    //return false;

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
  Serial.printf("updateStatus:: state=%d network=%d GPRS=%d MQTT=%d Signal Quality %d Chrg %d Percent %d Volts %d\n"
    , modem_state
    , status.netConnected
    , status.gprsConnected
    , status.mqttConnected
    , status.signalQuality
    , status.chargeState
    , status.percent
    , status.gsmVolts
  );
}

bool MqttGsmClient::ModemCheck()
{
  static int counter = 0;
  bool changed = false;
  counter += 1;
  if ( (counter % 5000) == 0)
  {
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
        break;      
    }
  }

  if ( changed || (counter % 50000) == 0)
  {
    printStatus();
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