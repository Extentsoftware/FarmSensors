#include "mqtt_gsm.h"

#define TINY_GSM_DEBUG SerialMon
#define AT_RX        26
#define AT_TX        27

MqttGsmClient::MqttGsmClient() 
{
    Serial1.begin(9600, SERIAL_8N1, AT_RX, AT_TX);
    while (!Serial1);

    debugger = new StreamDebugger(Serial1, Serial);
    _modem = new TinyGsm(*debugger);
    _client = new TinyGsmClient(*_modem);
    _mqttGPRS = new PubSubClient(*_client);

    // Set LED OFF
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);    
}

void MqttGsmClient::init(
  char *broker, 
  char *macStr, 
  char *apn,
  char *gprsUser,
  char *gprsPass,
  std::function<void(char*, byte*, unsigned int)> callback)
{
  _broker = broker;
  _macStr = macStr;
  _callback = callback;
  _apn=apn;
  _gprsUser=gprsUser;
  _gprsPass=gprsPass;  

  Serial.printf("GSM Broker %s apn %s\n", broker, apn);
  Serial.printf("Set MQTT Broker\n");    
  _mqttGPRS->setServer(_broker, 1883);
  Serial.printf("Set MQTT callback\n");    
  _mqttGPRS->setCallback(_callback);  

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

void MqttGsmClient::doModemStart()
{  
  _gsmStage = "Modem";
  _gsmStatus = "Starting";

  modemRestart();

  Serial.println("Restarting modem...");
  if (!_modem->restart()) {
    Serial.println("Failed to restart modem");
    return;
  }
  
  Serial.println("Initializing modem...");
  if (!_modem->init()) {
        Serial.println("Failed to restart modem, attempting to continue without restarting");
  }

  String name = _modem->getModemName();
  Serial.printf("Modem Name:%s\n", name);

  String modemInfo = _modem->getModemInfo();
  Serial.printf("Modem Info:%s\n", modemInfo);

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
    Serial.printf("Connected to GSM network\n");
  }
}

void MqttGsmClient::doGPRSConnect()
{
  Serial.printf("Attempt GPRS Connect APN %s user %s pwd %s\n", _apn, _gprsUser, _gprsPass);

  _gsmStage = Msg_GPRS;
  _gsmStatus = Msg_Connecting;
    
  bool connected = _modem->gprsConnect(_apn, _gprsUser, _gprsPass);    

  _gsmStatus = (char*)(connected ? Msg_Connected : Msg_Failed);

  if (connected) 
  { 
    Serial.println("GPRS Connected");
    modem_state=GPRS_CONNECTED;
  }
//  updateStatus();
}

void MqttGsmClient::mqttGPRSConnect()
{
  char topic[32];

//  updateStatus();
  if (!status.mqttConnected)
  {
    _gsmStage = Msg_MQTT;
    _gsmStatus = Msg_Connecting;
    boolean mqttConnected = _mqttGPRS->connect(_macStr);
    if (!mqttConnected)
    {
      _gsmStatus = Msg_FailedToConnect;
    }
    else
    {
      sprintf(topic, "bongo/%s/hub", _macStr);
      _mqttGPRS->subscribe(topic);  
      _mqttGPRS->publish(topic, "{\"state\":\"connected\"}", true);
      Serial.println("MQTT connected via GPRS");
      modem_state=MQ_CONNECTED;
    }
//    updateStatus();
  }
}  

void MqttGsmClient::mqttGPRSPoll()
{
  if (!status.gprsConnected || !status.mqttConnected)
  {
    Serial.printf("GPRS Disconnected\n");
    modem_state=MODEM_NOT_CONNECTED;
  }
  Serial.printf("Calling loop\n");
  bool result = _mqttGPRS->loop();
  Serial.printf("Called loop\n");

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
    
    MqttGsmStats newStatus;
    Serial.printf("Get battery status\n");
    _modem->getBattStats(newStatus.chargeState, newStatus.percent, newStatus.gsmVolts);
    Serial.printf("Get signal quality\n");
    newStatus.signalQuality = _modem->getSignalQuality();
    Serial.printf("Is Network connected?\n");
    newStatus.netConnected = _modem->isNetworkConnected();
    newStatus.gprsConnected = false;
    newStatus.mqttConnected = false;
    if (newStatus.netConnected)
    {
      Serial.printf("Is GPRS connected?\n");
      newStatus.gprsConnected = _modem->isGprsConnected();
      if (newStatus.gprsConnected)
      {
        Serial.printf("Is MQTT connected?\n");
        newStatus.mqttConnected = _mqttGPRS->connected();
        Serial.printf("MQTT connected =%s\n",newStatus.mqttConnected?"Yes":"No");
      }
    }

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
    return changed;
}

bool MqttGsmClient::ModemCheck()
{
  static int counter = 0;
  if ( (counter % 50) == 0)
  {
    updateStatus();    
  }

  counter += 1;

  Serial.printf("State is %d\n",modem_state);
  
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

void MqttGsmClient::modemPowerOn()
{
    Serial.printf("GSM Power On\n");  
    pinMode(PWR_PIN, OUTPUT);
    digitalWrite(PWR_PIN, LOW);
    delay(1000);    //Datasheet Ton mintues = 1S
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