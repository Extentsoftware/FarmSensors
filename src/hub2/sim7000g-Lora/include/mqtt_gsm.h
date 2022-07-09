#ifndef __MQTT_GSM__
#define __MQTT_GSM__

#include <stdio.h>
#include <SoftwareSerial.h>
#include <StreamDebugger.h>
#include <PubSubClient.h>

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024 // Set RX buffer to 1Kb
#define GSM_AUTOBAUD_MIN 9600 
#define GSM_AUTOBAUD_MAX 115200
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false
#define GSM_PIN             ""
#define PWR_PIN             4
#define LED_PIN             12

#include <TinyGsm.h>
#include <TinyGsmClient.h>

enum MODEM_STATE
{
    MODEM_INIT,
    MODEM_NOT_CONNECTED,
    MODEM_CONNECTED,
    GPRS_CONNECTED,
    MQ_CONNECTED,
};

struct MqttGsmStats
{
public:
    uint16_t gsmDb=0;
    uint16_t gsmVolts=0;
    int16_t signalQuality=0;
    uint8_t chargeState=0;
    int8_t percent=0;
    bool gprsConnected = false;
    bool netConnected = false;
    bool mqttConnected = false;
};

class MqttGsmClient
{
  public:
    MqttGsmClient();
    void init(
      char *broker, 
      char *macStr, 
      char *apn,
      char *gprsUser,
      char *gprsPass,
      char *simPin,
      std::function<void(char*, byte*, unsigned int)> callback);
    bool ModemCheck();
    String getGsmStage();
    String getGsmStatus();
    bool sendMQTTBinary(uint8_t *report, int packetSize, char *subtopic);
    bool isConnected();
    void getStatus(MqttGsmStats& stats);

  private:
    bool updateStatus();
    void doModemStart();
    void waitForNetwork();
    void doGPRSConnect();
    void mqttGPRSConnect();
    void mqttGPRSPoll();
    void modemPowerOn();
    void modemPowerOff();
    void modemRestart();
    void printStatus();
    
    enum MODEM_STATE modem_state=MODEM_INIT;
    std::function<void(char*, byte*, unsigned int)> _callback_mqtt;
    String _gsmStage= "Booting";
    String _gsmStatus = "";
    char * _macStr;
    char * _apn;
    char * _gprsUser;
    char * _gprsPass;
    char * _simPin;
    StreamDebugger* debugger;
    TinyGsm* _modem;
    TinyGsmClient* _client;
    PubSubClient* _mqttGPRS;
    MqttGsmStats status;


    const char *_broker;
    const char * Msg_Quality = "Db ";
    const char * Msg_Network = "Radio";
    const char * Msg_GPRS    = "GPRS";
    const char * Msg_MQTT    = "MQTT";

    const char * Msg_Failed     = "Failed";
    const char * Msg_Connected  = "Connected";
    const char * Msg_Connecting = "Connecting";
    const char * Msg_FailedToConnect = "Failed Connection";
};

#endif