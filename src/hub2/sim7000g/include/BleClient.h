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

static bool ble_notified = false;

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

        Serial.printf(" - MTU %d", pClient->getMTU());

        // Connect to the remove BLE Server.
        pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
        Serial.println(" - Connected to server");

        

        // Obtain a reference to the service we are after in the remote BLE server.
        BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
        if (pRemoteService == nullptr) {
          pClient->disconnect();
          return false;
        }
        Serial.println(" - Found our service");

        // Obtain a reference to the characteristic in the service of the remote BLE server.
        pRemoteCharacteristic = pRemoteService->getCharacteristic(SENS_CHARACTERISTIC_UUID);
        if (pRemoteCharacteristic == nullptr) {
          Serial.print("Failed to find our characteristic UUID: ");
          Serial.println(SENS_CHARACTERISTIC_UUID);
          pClient->disconnect();
          return false;
        }
        Serial.println(" - Found our characteristic");

        // Read the value of the characteristic.
        if(pRemoteCharacteristic->canRead()) {
          std::string value = pRemoteCharacteristic->readValue();
          Serial.print("The characteristic value was: ");
          Serial.println(value.c_str());
        }

        if(pRemoteCharacteristic->canNotify())
          pRemoteCharacteristic->registerForNotify(notifyCallback);

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

        ble_notified = true;
    }

    void init() 
    {
        BLEDevice::init("");        
        BLEDevice::setMTU(500);
        BLEScan* pBLEScan = BLEDevice::getScan();
        pBLEScan->setAdvertisedDeviceCallbacks(this);
        pBLEScan->setActiveScan(true);
        pBLEScan->start(5, false);
    }

    void loop() 
    {
        if (ble_notified)
        {
          ble_notified = false;
          if(pRemoteCharacteristic->canRead()) {
            std::string value = pRemoteCharacteristic->readValue();
            Serial.print("In loop: The characteristic value was: ");
            Serial.println(value.c_str());
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
    BLERemoteCharacteristic* pRemoteCharacteristic;
    BLEAdvertisedDevice* myDevice;

};

#endif
