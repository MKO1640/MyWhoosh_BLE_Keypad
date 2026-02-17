#include "BleAbsMouse.h"

#if defined(USE_NIMBLE)
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <NimBLEHIDDevice.h>
#else
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#endif

#include "HIDTypes.h"
#include "HIDKeyboardTypes.h"
#include <driver/adc.h>
#include "sdkconfig.h"

#include "BleConnectionStatus.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

#define REPORTID_TOUCH 0x01

#define LSB(v) ((v >> 8) & 0xff)
#define MSB(v) (v & 0xff)

static const uint8_t _hidReportDescriptor[] = {
  0x05, 0x0d,                    /* USAGE_PAGE (Digitizer) */
  0x09, 0x04,                    /* USAGE (Touch Screen) */
  0xa1, 0x01,                    /* COLLECTION (Application) */
  0x85, REPORTID_TOUCH,          /* REPORT_ID */

  0x09, 0x20,                    /* Usage (Stylus) */
  0xA1, 0x00,                    /* Collection (Physical) */

  0x09, 0x42,                    /* Usage (Tip Switch) */
  0x09, 0x32,                    /* USAGE (In Range) */
  0x15, 0x00,                    /* LOGICAL_MINIMUM (0) */
  0x25, 0x01,                    /* LOGICAL_MAXIMUM (1) */
  0x75, 0x01,                    /* REPORT_SIZE (1) */
  0x95, 0x02,                    /* REPORT_COUNT (2) */
  0x81, 0x02,                    /* INPUT (Data,Var,Abs) */

  0x75, 0x01,                    /* REPORT_SIZE (1) */
  0x95, 0x06,                    /* REPORT_COUNT (6) */
  0x81, 0x01,                    /* INPUT (Cnst,Ary,Abs) */

  0x05, 0x01,                    /* Usage Page (Generic Desktop) */
  0x09, 0x01,                    /* Usage (Pointer) */
  0xA1, 0x00,                    /* Collection (Physical) */
  0x09, 0x30,                    /* Usage (X) */
  0x09, 0x31,                    /* Usage (Y) */
  0x16, 0x00, 0x00,              /* Logical Minimum (0) */
  0x26, 0x10, 0x27,              /* Logical Maximum (10000) */
  0x36, 0x00, 0x00,              /* Physical Minimum (0) */
  0x46, 0x10, 0x27,              /* Physical Maximum (10000) */
  0x66, 0x00, 0x00,              /* UNIT (None) */
  0x75, 0x10,                    /* Report Size (16), */
  0x95, 0x02,                    /* Report Count (2), */
  0x81, 0x02,                    /* Input (Data,Var,Abs) */
  0xc0,                          /* END_COLLECTION */
  0xc0,                          /* END_COLLECTION */
  0xc0                           /* END_COLLECTION */
};

BleAbsMouse::BleAbsMouse(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel) :
    hid(0)
{
  this->deviceName = deviceName;
  this->deviceManufacturer = deviceManufacturer;
  this->batteryLevel = batteryLevel;
  this->connectionStatus = new BleConnectionStatus();
  this->isPressed = false;
}

void BleAbsMouse::begin(void)
{
  xTaskCreate(this->taskServer, "server", 20000, (void *)this, 5, NULL);
}

void BleAbsMouse::end(void)
{
}

void BleAbsMouse::click(int16_t x, int16_t y)
{
  this->move(x, y);
  this->release();
}

void BleAbsMouse::move(int16_t x, int16_t y)
{
  this->send(3, x, y);
  this->isPressed = true;
}

void BleAbsMouse::release()
{
  this->send(0, 0, 0);
  this->isPressed = false;
}

void BleAbsMouse::send(int state, int16_t x, int16_t y)
{
  if (this->isConnected())
  {
    uint8_t m[5];
    m[0] = state;
    m[1] = MSB(x);
    m[2] = LSB(x);
    m[3] = MSB(y);
    m[4] = LSB(y);
    this->inputAbsMouse->setValue(m, 5);
    this->inputAbsMouse->notify();
  }
}

bool BleAbsMouse::isConnected(void) {
  return this->connectionStatus->connected;
}

void BleAbsMouse::setBatteryLevel(uint8_t level) {
  this->batteryLevel = level;
  if (hid != 0) {
    this->hid->setBatteryLevel(this->batteryLevel);
  }
}

void BleAbsMouse::taskServer(void* pvParameter) {
  BleAbsMouse* bleMouseInstance = (BleAbsMouse *)pvParameter;

  BLEDevice::init(bleMouseInstance->deviceName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(bleMouseInstance->connectionStatus);

  bleMouseInstance->hid = new BLEHIDDevice(pServer);
  bleMouseInstance->inputAbsMouse = bleMouseInstance->hid->inputReport(REPORTID_TOUCH);
  bleMouseInstance->connectionStatus->inputAbsMouse = bleMouseInstance->inputAbsMouse;

  bleMouseInstance->hid->manufacturer()->setValue(bleMouseInstance->deviceManufacturer);
  bleMouseInstance->hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  bleMouseInstance->hid->hidInfo(0x00, 0x02);

#if defined(USE_NIMBLE)
  BLEDevice::setSecurityAuth(true, true, true);
#else
  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
#endif

  bleMouseInstance->hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  bleMouseInstance->hid->startServices();

  bleMouseInstance->onStarted(pServer);

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(bleMouseInstance->hid->hidService()->getUUID());
  pAdvertising->start();
  bleMouseInstance->hid->setBatteryLevel(bleMouseInstance->batteryLevel);

  ESP_LOGD(LOG_TAG, "Advertising started!");
  vTaskDelay(portMAX_DELAY);
}
