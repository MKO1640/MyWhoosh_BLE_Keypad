#ifndef ESP32_BLE_CONNECTION_STATUS_H
#define ESP32_BLE_CONNECTION_STATUS_H
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)
#include "NimBLEServer.h"
#include "NimBLECharacteristic.h"
#define BLEServerCallbacks NimBLEServerCallbacks
#define BLEServer NimBLEServer
#define BLECharacteristic NimBLECharacteristic
#else
#include <BLEServer.h>
#include <BLEUtils.h>
#include "BLE2902.h"
#include "BLECharacteristic.h"
#endif

class BleConnectionStatus : public BLEServerCallbacks
{
public:
  BleConnectionStatus(void);
  bool connected = false;
  void onConnect(BLEServer* pServer);
  void onDisconnect(BLEServer* pServer);
  BLECharacteristic* inputAbsMouse;
};

#endif // CONFIG_BT_ENABLED
#endif // ESP32_BLE_CONNECTION_STATUS_H
