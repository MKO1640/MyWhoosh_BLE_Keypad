#ifndef BLE_COMBO_ABS_H
#define BLE_COMBO_ABS_H

#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#if defined(USE_NIMBLE)
#include <NimBLECharacteristic.h>
#include <NimBLEHIDDevice.h>

#define BLEDevice                  NimBLEDevice
#define BLEServerCallbacks         NimBLEServerCallbacks
#define BLECharacteristicCallbacks NimBLECharacteristicCallbacks
#define BLEHIDDevice               NimBLEHIDDevice
#define BLECharacteristic          NimBLECharacteristic
#define BLEAdvertising             NimBLEAdvertising
#define BLEServer                  NimBLEServer

#else
#include <BLEHIDDevice.h>
#include <BLECharacteristic.h>
#endif // USE_NIMBLE

#include <Print.h>

const uint8_t KEY_LEFT_CTRL = 0x80;
const uint8_t KEY_LEFT_SHIFT = 0x81;
const uint8_t KEY_LEFT_ALT = 0x82;
const uint8_t KEY_LEFT_GUI = 0x83;
const uint8_t KEY_RIGHT_CTRL = 0x84;
const uint8_t KEY_RIGHT_SHIFT = 0x85;
const uint8_t KEY_RIGHT_ALT = 0x86;
const uint8_t KEY_RIGHT_GUI = 0x87;

const uint8_t KEY_UP_ARROW = 0xDA;
const uint8_t KEY_DOWN_ARROW = 0xD9;
const uint8_t KEY_LEFT_ARROW = 0xD8;
const uint8_t KEY_RIGHT_ARROW = 0xD7;
const uint8_t KEY_BACKSPACE = 0xB2;
const uint8_t KEY_TAB = 0xB3;
const uint8_t KEY_RETURN = 0xB0;
const uint8_t KEY_ESC = 0xB1;
const uint8_t KEY_INSERT = 0xD1;
const uint8_t KEY_PRTSC = 0xCE;
const uint8_t KEY_DELETE = 0xD4;
const uint8_t KEY_PAGE_UP = 0xD3;
const uint8_t KEY_PAGE_DOWN = 0xD6;
const uint8_t KEY_HOME = 0xD2;
const uint8_t KEY_END = 0xD5;
const uint8_t KEY_CAPS_LOCK = 0xC1;
const uint8_t KEY_F1 = 0xC2;
const uint8_t KEY_F2 = 0xC3;
const uint8_t KEY_F3 = 0xC4;
const uint8_t KEY_F4 = 0xC5;
const uint8_t KEY_F5 = 0xC6;
const uint8_t KEY_F6 = 0xC7;
const uint8_t KEY_F7 = 0xC8;
const uint8_t KEY_F8 = 0xC9;
const uint8_t KEY_F9 = 0xCA;
const uint8_t KEY_F10 = 0xCB;
const uint8_t KEY_F11 = 0xCC;
const uint8_t KEY_F12 = 0xCD;

const uint8_t KEY_NUM_0 = 0xEA;
const uint8_t KEY_NUM_1 = 0xE1;
const uint8_t KEY_NUM_2 = 0xE2;
const uint8_t KEY_NUM_3 = 0xE3;
const uint8_t KEY_NUM_4 = 0xE4;
const uint8_t KEY_NUM_5 = 0xE5;
const uint8_t KEY_NUM_6 = 0xE6;
const uint8_t KEY_NUM_7 = 0xE7;
const uint8_t KEY_NUM_8 = 0xE8;
const uint8_t KEY_NUM_9 = 0xE9;
const uint8_t KEY_NUM_SLASH = 0xDC;
const uint8_t KEY_NUM_ASTERISK = 0xDD;
const uint8_t KEY_NUM_MINUS = 0xDE;
const uint8_t KEY_NUM_PLUS = 0xDF;
const uint8_t KEY_NUM_ENTER = 0xE0;
const uint8_t KEY_NUM_PERIOD = 0xEB;

// Low level key report: up to 6 keys and modifiers at once
typedef struct
{
  uint8_t modifiers;
  uint8_t reserved;
  uint8_t keys[6];
} KeyReport;

class BleComboAbs : public Print, public BLEServerCallbacks, public BLECharacteristicCallbacks
{
private:
  BLEHIDDevice* hid;
  BLECharacteristic* inputKeyboard;
  BLECharacteristic* outputKeyboard;
  BLECharacteristic* inputAbsMouse;
  BLEAdvertising* advertising;
  KeyReport _keyReport;
  std::string deviceName;
  std::string deviceManufacturer;
  uint8_t batteryLevel;
  bool connected = false;
  uint32_t _delay_ms = 7;
  bool absPressed = false;
  bool debugEnabled = false;

  void delay_ms(uint64_t ms);
  void sendKeyboardReport(KeyReport* keys);
  void sendAbsMouseReport(uint8_t state, int16_t x, int16_t y);

public:
  BleComboAbs(std::string deviceName = "ESP32 Combo HID", std::string deviceManufacturer = "Espressif", uint8_t batteryLevel = 100);
  void begin(void);
  void end(void);
  size_t press(uint8_t k);
  size_t release(uint8_t k);
  size_t write(uint8_t c);
  size_t write(const uint8_t* buffer, size_t size);
  void releaseAll(void);
  bool isConnected(void);
  void setBatteryLevel(uint8_t level);
  void setName(std::string deviceName);
  void setDelay(uint32_t ms);
  void setDebug(bool enabled);

  void clickAbs(int16_t x, int16_t y);
  void moveAbs(int16_t x, int16_t y);
  void releaseAbs(void);
  bool isAbsPressed(void);

protected:
  virtual void onStarted(BLEServer* pServer) { };
  virtual void onConnect(BLEServer* pServer) override;
  virtual void onDisconnect(BLEServer* pServer) override;
  virtual void onWrite(BLECharacteristic* me) override;
};

#endif // CONFIG_BT_ENABLED
#endif // BLE_COMBO_ABS_H
