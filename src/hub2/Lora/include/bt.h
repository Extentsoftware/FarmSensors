#if false
#ifndef __BT__
#define __BT__

#include "Arduino.h"
#include "base64.hpp"
#include "BluetoothSerial.h"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define BT_STX 0x02
#define BT_ETX 0x03
#define BT_BUFFER 512

class Bt
{
  public:
    
    void init(String name, bool isClient, std::function<void(char, byte*, unsigned int)> callback ) 
    {
      _isClient = isClient;
      _callback = callback;
      _read_buffer = (uint8_t *)malloc(BT_BUFFER);
      memset(_read_buffer,0,BT_BUFFER);
      SerialBT.begin(name, isClient);
    }

    void loop() 
    {
      if (_isClient)
        loopClient();
      else
        loopServer();
    }
    
    void sendData(unsigned char cmd, unsigned char * packet, int packetSize)
    {      
      if (SerialBT.connected())
      {
        uint8_t * buffer = (uint8_t *)malloc(packetSize * 2);
        int base64_length = encode_base64(packet, packetSize, buffer);
        SerialBT.write(BT_STX);
        SerialBT.write(cmd);
        SerialBT.write(buffer, base64_length);
        SerialBT.write(BT_ETX);
        free(buffer);
      } 
    }

  private:

    void lookForData() {      

      if (SerialBT.available()) 
      {
        int data = SerialBT.read();
        if (data!=-1)
        {
          // wait until STX
          if (data==BT_STX)
          {
              int cmd = SerialBT.read();
              int packetSize = SerialBT.readBytesUntil(BT_ETX, _read_buffer, BT_BUFFER);
              if (packetSize>0 && _callback!=NULL)
              {
                uint8_t * decoded_buffer = (uint8_t *)malloc(packetSize);
                int base64_length = decode_base64(_read_buffer, packetSize-1, decoded_buffer);
                _callback((char)cmd, decoded_buffer, base64_length);
                free(decoded_buffer);
              }
          }
        }
      }
    }


    void loopServer() {
      lookForData();
    }

    void loopClient() {      
      if (getConnected())
        lookForData();
    }

    bool getConnected() {      

      if (!SerialBT.isReady())
        return false;

      if (!SerialBT.connected())
      {
        bool connected = SerialBT.connect("Lora Hub");    
        if (_bt_connected != connected)
          Serial.printf("BT connected=%s\n", connected?"Yes":"No");
        _bt_connected = connected;
      }

      return _bt_connected;
    }
    

    BluetoothSerial SerialBT;
    bool _bt_connected = false;
    bool _isClient;
    uint8_t* _read_buffer;
    std::function<void(char, byte*, unsigned int)> _callback;
};

#endif
#endif