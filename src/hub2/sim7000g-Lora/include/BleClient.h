#ifndef __BLE_CLIENT__
#define __BLE_CLIENT__

#include "Arduino.h"
#include <BLEDevice.h>
#include <functional>
#include "base64.hpp"

#define BT_BUFFER                   512
#define SERVICE_UUID                "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define SENS_CHARACTERISTIC_UUID    "SENS"
#define INDX_CHARACTERISTIC_UUID    "INDX"
#define VOLT_CHARACTERISTIC_UUID    "VOLT"

static bool ble_sense_notified = false;

class BleClient : BLEClientCallbacks, BLEAdvertisedDeviceCallbacks 
{
  public:

    // Advertise callback
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice.toString().c_str());

        // We have found a device, let us now see if it contains the service we are looking for.
        if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) {

        BLEDevice::getScan()->stop();
        myDevice = new BLEAdvertisedDevice(advertisedDevice);
        doConnect = true;
        doScan = true;

        } // Found our server
    } // onResult
        
    void onConnect(BLEClient* pclient) {
        Serial.println("onConnect");
    }

    void onDisconnect(BLEClient* pclient) {
        connected = false;
        Serial.println("onDisconnect");
    }

    bool connectToServer() {
        Serial.print("Forming a connection to ");
        Serial.println(myDevice->getAddress().toString().c_str());
        
        BLEClient*  pClient  = BLEDevice::createClient();
        Serial.println(" - Created client");

        pClient->setClientCallbacks(this);

        pClient->setMTU(500);
        Serial.printf(" - MTU %d\n", pClient->getMTU());

        // Connect to the remove BLE Server.
        for (int i=0;i<3;i++)
        {
            delay(1000);
            bool connected = pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
            if (connected)
              break;
            Serial.printf("Failed to connect to server - attempt %d", i+1);
        }

        Serial.println(" - Connected to server");

        // Obtain a reference to the service we are after in the remote BLE server.
        BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
        if (pRemoteService == nullptr) {
          pClient->disconnect();
          return false;
        }
        Serial.println(" - Found our service");

        // Obtain a reference to the characteristic in the service of the remote BLE server.
        pRemoteSenseCharacteristic = pRemoteService->getCharacteristic(SENS_CHARACTERISTIC_UUID);
        if (pRemoteSenseCharacteristic == nullptr) {
          Serial.print("Failed to find our characteristic UUID: ");
          Serial.println(SENS_CHARACTERISTIC_UUID);
          pClient->disconnect();
          return false;
        }
        Serial.println(" - Found our characteristic");

        // Read the value of the characteristic.
        if(pRemoteSenseCharacteristic->canRead()) {
          std::string value = pRemoteSenseCharacteristic->readValue();
          Serial.print("The characteristic value was: ");
          Serial.println(value.c_str());
        }

        if(pRemoteSenseCharacteristic->canNotify())
          pRemoteSenseCharacteristic->registerForNotify(notifyCallback);

        connected = true;
        return connected;
    }

    static void notifyCallback(
        BLERemoteCharacteristic* pBLERemoteCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify) 
    {
        Serial.print("Notify callback for characteristic ");        
        Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
        Serial.print(" of data length ");
        Serial.println(length);
        Serial.print("data: ");
        Serial.println((char*)pData);

        if (pBLERemoteCharacteristic->getUUID().equals(BLEUUID(SENS_CHARACTERISTIC_UUID)))
          ble_sense_notified = true;
    }

    void init(std::function<void(byte*, unsigned int)> transmit_callback) 
    {
        _transmit_callback = transmit_callback;
        Serial.printf("BLEDevice::init\n");
        BLEDevice::init("SIM7000G");        
        BLEDevice::setMTU(500);
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(this);
        Serial.printf("pBLEScan->setActiveScan\n");
        pBLEScan->setActiveScan(true);
        Serial.printf("pBLEScan->start\n");
        pBLEScan->start(5, false);
        Serial.printf("BLEDevice::init end\n");
    }

    void loop() 
    {
        if (ble_sense_notified)
        {
          ble_sense_notified = false;
          if(pRemoteSenseCharacteristic->canRead()) {
            std::string value = pRemoteSenseCharacteristic->readValue();
            Serial.print("In loop: The characteristic value was: ");
            Serial.println(value.c_str());
            unsigned char * buffer = (uint8_t *)malloc(value.length());
            int base64_length = decode_base64((unsigned char *)(value.c_str()), buffer);
            if (_transmit_callback != NULL)
              _transmit_callback(buffer, base64_length);
            free(buffer);
          }
        }
        // If the flag "doConnect" is true then we have scanned for and found the desired
        // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
        // connected we set the connected flag to be true.
        if (doConnect == true) {
            if (connectToServer()) {
              Serial.println("We are now connected to the BLE Server.");
            } else {
              Serial.println("We have failed to connect to the server; there is nothin more we will do.");
            }
            doConnect = false;
        }

        // If we are connected to a peer BLE Server, update the characteristic each time we are reached
        // with the current time since boot.
        if (connected) {
            // String newValue = "Time since boot: " + String(millis()/1000);
            // Serial.println("Setting new characteristic value to \"" + newValue + "\"");
            
            // // Set the characteristic's value to be the array of bytes that is actually a string.
            // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
        }else if(doScan){
            BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
        }
    }
    
  private:
    boolean doConnect = false;
    boolean connected = false;
    boolean doScan = false;
    BLERemoteCharacteristic* pRemoteSenseCharacteristic;
    BLEAdvertisedDevice* myDevice;
    std::function<void(byte*, unsigned int)> _transmit_callback;

};

#endif