#include "BleConnectionStatus.h"

BleConnectionStatus::BleConnectionStatus(void)
{
}

void BleConnectionStatus::onConnect(BLEServer* pServer)
{
  this->connected = true;
#if !defined(USE_NIMBLE)
  BLE2902* desc = (BLE2902*)this->inputAbsMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  if (desc) {
    desc->setNotifications(true);
  }
#endif
}

void BleConnectionStatus::onDisconnect(BLEServer* pServer)
{
  this->connected = false;
#if !defined(USE_NIMBLE)
  BLE2902* desc = (BLE2902*)this->inputAbsMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  if (desc) {
    desc->setNotifications(false);
  }
#endif
}
