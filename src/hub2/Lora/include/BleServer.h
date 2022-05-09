#ifndef __BLE_SERVER__
#define __BLE_SERVER__

#include "Arduino.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <functional>
#include "base64.hpp"

#define BT_BUFFER 512
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SENS_CHARACTERISTIC_UUID "SENS"
#define INDX_CHARACTERISTIC_UUID "INDX"
#define VOLT_CHARACTERISTIC_UUID    "VOLT"

class BleServer : BLEServerCallbacks,BLECharacteristicCallbacks
{
  public:
    void onConnect(BLEServer* pServer) {
      Serial.printf("ble Connect\n"); 
      deviceConnected = true;
      BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer* pServer) {
      Serial.printf("ble disconnect\n"); 
      deviceConnected = false;
    }

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      BLEUUID uuid = pCharacteristic->getUUID();
      int len = rxValue.length();
      Serial.printf("Received Index Change %s: %d bytes: \n",uuid.toString().c_str(), len);
      if (len > 0) {
        
        //if (_callback)
        //  _callback()
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
      }
    }
 
    void init(std::string name, std::function<void(char *, byte*, unsigned int)> callback ) 
    {
      _callback = callback;
      _read_buffer = (uint8_t *)malloc(BT_BUFFER);
      memset(_read_buffer,0,BT_BUFFER);
      BLEDevice::init(name);

      pServer = BLEDevice::createServer();
      pServer->setCallbacks(this);

      // Create the BLE Service
      BLEService *pService = pServer->createService(SERVICE_UUID);
      pSenseCharacteristic = pService->createCharacteristic(
                  SENS_CHARACTERISTIC_UUID,
                  BLECharacteristic::PROPERTY_READ   |
                  BLECharacteristic::PROPERTY_WRITE  |
                  BLECharacteristic::PROPERTY_NOTIFY
                );

      pIndexCharacteristic = pService->createCharacteristic(
                  INDX_CHARACTERISTIC_UUID,
                  BLECharacteristic::PROPERTY_READ   |
                  BLECharacteristic::PROPERTY_WRITE
                );

      pVoltsCharacteristic = pService->createCharacteristic(
                  VOLT_CHARACTERISTIC_UUID,
                  BLECharacteristic::PROPERTY_READ   |
                  BLECharacteristic::PROPERTY_WRITE  |
                  BLECharacteristic::PROPERTY_NOTIFY
                );

      // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
      // Create a BLE Descriptor
      pSenseCharacteristic->addDescriptor(new BLE2902());
      pIndexCharacteristic->addDescriptor(new BLE2902());
      pVoltsCharacteristic->addDescriptor(new BLE2902());

      pSenseCharacteristic->setCallbacks(this);
      pIndexCharacteristic->setCallbacks(this);
      pVoltsCharacteristic->setCallbacks(this);

      // Start the service
      pService->start();

      // Start advertising
      BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
      pAdvertising->addServiceUUID(SERVICE_UUID);
      pAdvertising->setScanResponse(true);
      pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
      pAdvertising->setMaxPreferred(0x12);          
      BLEDevice::startAdvertising();
    }

    void loop() 
    {
        loopServer();
    }
    
    void sendData(unsigned char cmd, unsigned char * packet, int packetSize)
    {     
        uint8_t * buffer = (uint8_t *)malloc(packetSize * 2);
        int base64_length = encode_base64(packet, packetSize, buffer);
        Serial.printf("set value %d bytes %s\n", base64_length, buffer); 
        pSenseCharacteristic->setValue(buffer, base64_length);
        free(buffer);
        pSenseCharacteristic->notify();
    }
    
    void sendVolts(float volts)
    {     
        Serial.printf("set volts %f\n", volts); 
        pVoltsCharacteristic->setValue(volts);
        pVoltsCharacteristic->notify();
    }

  private:

    void loopServer() {
      if (deviceConnected) {
        //pCharacteristic->notify();
        delay(10); // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
      }
      // disconnecting
      if (!deviceConnected && oldDeviceConnected) {
          delay(500); // give the bluetooth stack the chance to get things ready
          pServer->startAdvertising(); // restart advertising
          Serial.println("start advertising");
          oldDeviceConnected = deviceConnected;
      }
      // connecting
      if (deviceConnected && !oldDeviceConnected) {
          // do stuff here on connecting
          oldDeviceConnected = deviceConnected;
          pSenseCharacteristic->notify();
      }
    }

    uint8_t* _read_buffer;
    std::function<void(char *, byte*, unsigned int)> _callback;
    BLEServer* pServer = NULL;
    BLECharacteristic* pSenseCharacteristic = NULL;
    BLECharacteristic* pIndexCharacteristic = NULL;
    BLECharacteristic* pVoltsCharacteristic = NULL;
    bool deviceConnected = false;
    bool oldDeviceConnected = false;
    uint32_t value = 0;
};

#endif
