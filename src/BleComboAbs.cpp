#include "BleComboAbs.h"

#if defined(CONFIG_BT_ENABLED)

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
#endif // USE_NIMBLE

#include "HIDTypes.h"
#include <Arduino.h>
#include <driver/adc.h>
#include "sdkconfig.h"

#if defined(CONFIG_ARDUHAL_ESP_LOG)
  #include "esp32-hal-log.h"
  #define LOG_TAG ""
#else
  #include "esp_log.h"
  static const char* LOG_TAG = "BLEDevice";
#endif

#define KEYBOARD_ID 0x01
#define ABS_MOUSE_ID 0x02

#define LSB(v) ((v >> 8) & 0xff)
#define MSB(v) (v & 0xff)

static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,          // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,          // USAGE (Keyboard)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  REPORT_ID(1),       KEYBOARD_ID,   //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,          //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,          //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,          //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,          //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x05,          //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,          //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,          //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,          //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,          //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,          //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,          //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x06,          //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x65,          //   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,          //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x65,          //   USAGE_MAXIMUM (0x65)
  HIDINPUT(1),        0x00,          //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                 // END_COLLECTION

  // Absolute mouse (digitizer)
  0x05, 0x0d,                    // USAGE_PAGE (Digitizer)
  0x09, 0x04,                    // USAGE (Touch Screen)
  0xa1, 0x01,                    // COLLECTION (Application)
  0x85, ABS_MOUSE_ID,            //   REPORT_ID
  0x09, 0x20,                    //   Usage (Stylus)
  0xA1, 0x00,                    //   Collection (Physical)
  0x09, 0x42,                    //     Usage (Tip Switch)
  0x09, 0x32,                    //     USAGE (In Range)
  0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
  0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x95, 0x02,                    //     REPORT_COUNT (2)
  0x81, 0x02,                    //     INPUT (Data,Var,Abs)
  0x75, 0x01,                    //     REPORT_SIZE (1)
  0x95, 0x06,                    //     REPORT_COUNT (6)
  0x81, 0x01,                    //     INPUT (Cnst,Ary,Abs)
  0x05, 0x01,                    //     Usage Page (Generic Desktop)
  0x09, 0x01,                    //     Usage (Pointer)
  0xA1, 0x00,                    //     Collection (Physical)
  0x09, 0x30,                    //        Usage (X)
  0x09, 0x31,                    //        Usage (Y)
  0x16, 0x00, 0x00,              //        Logical Minimum (0)
  0x26, 0x10, 0x27,              //        Logical Maximum (10000)
  0x36, 0x00, 0x00,              //        Physical Minimum (0)
  0x46, 0x10, 0x27,              //        Physical Maximum (10000)
  0x66, 0x00, 0x00,              //        UNIT (None)
  0x75, 0x10,                    //        Report Size (16)
  0x95, 0x02,                    //        Report Count (2)
  0x81, 0x02,                    //        Input (Data,Var,Abs)
  0xc0,                          //     END_COLLECTION
  0xc0,                          //   END_COLLECTION
  0xc0                           // END_COLLECTION
};

BleComboAbs::BleComboAbs(std::string deviceName, std::string deviceManufacturer, uint8_t batteryLevel)
    : hid(0)
    , deviceName(std::string(deviceName).substr(0, 15))
    , deviceManufacturer(std::string(deviceManufacturer).substr(0, 15))
    , batteryLevel(batteryLevel) {}

void BleComboAbs::begin(void)
{
  BLEDevice::init(deviceName);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(this);

  hid = new BLEHIDDevice(pServer);
  inputKeyboard = hid->inputReport(KEYBOARD_ID);
  outputKeyboard = hid->outputReport(KEYBOARD_ID);
  inputAbsMouse = hid->inputReport(ABS_MOUSE_ID);

  outputKeyboard->setCallbacks(this);

  hid->manufacturer()->setValue(deviceManufacturer);
  hid->pnp(0x02, 0x05ac, 0x820a, 0x0210);
  hid->hidInfo(0x00, 0x01);

#if defined(USE_NIMBLE)
  BLEDevice::setSecurityAuth(true, true, true);
#else
  BLESecurity* pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_MITM_BOND);
#endif // USE_NIMBLE

  hid->reportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();

  onStarted(pServer);

  advertising = pServer->getAdvertising();
  advertising->setAppearance(HID_KEYBOARD);
  advertising->addServiceUUID(hid->hidService()->getUUID());
  advertising->addServiceUUID(BLEUUID((uint16_t)0x180F));
  advertising->setScanResponse(false);
  advertising->start();
  hid->setBatteryLevel(batteryLevel);

  ESP_LOGD(LOG_TAG, "Advertising started!");
}

void BleComboAbs::end(void)
{
}

bool BleComboAbs::isConnected(void)
{
  return this->connected;
}

void BleComboAbs::setBatteryLevel(uint8_t level)
{
  this->batteryLevel = level;
  if (hid != 0) {
    this->hid->setBatteryLevel(this->batteryLevel);
  }
}

void BleComboAbs::setName(std::string deviceName)
{
  this->deviceName = deviceName;
}

void BleComboAbs::setDelay(uint32_t ms)
{
  this->_delay_ms = ms;
}

void BleComboAbs::setDebug(bool enabled)
{
  this->debugEnabled = enabled;
}

void BleComboAbs::sendKeyboardReport(KeyReport* keys)
{
  if (this->isConnected()) {
    ESP_LOGD(LOG_TAG, "[DEBUG] Keyboard report sent");
    this->inputKeyboard->setValue((uint8_t*)keys, sizeof(KeyReport));
    this->inputKeyboard->notify();
#if defined(USE_NIMBLE)
    this->delay_ms(_delay_ms);
#endif
  }
}

void BleComboAbs::sendAbsMouseReport(uint8_t state, int16_t x, int16_t y)
{
  if (this->isConnected()) {
    if (debugEnabled) {
      Serial.print("[DEBUG] Abs mouse report: state=");
      Serial.print(state);
      Serial.print(" x=");
      Serial.print(x);
      Serial.print(" y=");
      Serial.println(y);
    }
    uint8_t m[5];
    m[0] = state;
    m[1] = MSB(x);
    m[2] = LSB(x);
    m[3] = MSB(y);
    m[4] = LSB(y);
    this->inputAbsMouse->setValue(m, 5);
    this->inputAbsMouse->notify();
#if defined(USE_NIMBLE)
    this->delay_ms(_delay_ms);
#endif
  } else if (debugEnabled) {
    Serial.println("[DEBUG] Abs mouse report skipped (not connected)");
  }
}

extern const uint8_t _asciimap[128] PROGMEM;

#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
  0x00,             // NUL
  0x00,             // SOH
  0x00,             // STX
  0x00,             // ETX
  0x00,             // EOT
  0x00,             // ENQ
  0x00,             // ACK
  0x00,             // BEL
  0x2a,             // BS  Backspace
  0x2b,             // TAB Tab
  0x28,             // LF  Enter
  0x00,             // VT
  0x00,             // FF
  0x00,             // CR
  0x00,             // SO
  0x00,             // SI
  0x00,             // DEL
  0x00,             // DC1
  0x00,             // DC2
  0x00,             // DC3
  0x00,             // DC4
  0x00,             // NAK
  0x00,             // SYN
  0x00,             // ETB
  0x00,             // CAN
  0x00,             // EM
  0x00,             // SUB
  0x00,             // ESC
  0x00,             // FS
  0x00,             // GS
  0x00,             // RS
  0x00,             // US

  0x2c,             //  '
  0x1e|SHIFT,       // !
  0x34|SHIFT,       // "
  0x20|SHIFT,       // #
  0x21|SHIFT,       // $
  0x22|SHIFT,       // %
  0x24|SHIFT,       // &
  0x34,             // '
  0x26|SHIFT,       // (
  0x27|SHIFT,       // )
  0x25|SHIFT,       // *
  0x2e|SHIFT,       // +
  0x36,             // ,
  0x2d,             // -
  0x37,             // .
  0x38,             // /
  0x27,             // 0
  0x1e,             // 1
  0x1f,             // 2
  0x20,             // 3
  0x21,             // 4
  0x22,             // 5
  0x23,             // 6
  0x24,             // 7
  0x25,             // 8
  0x26,             // 9
  0x33|SHIFT,       // :
  0x33,             // ;
  0x36|SHIFT,       // <
  0x2e,             // =
  0x37|SHIFT,       // >
  0x38|SHIFT,       // ?
  0x1f|SHIFT,       // @
  0x04|SHIFT,       // A
  0x05|SHIFT,       // B
  0x06|SHIFT,       // C
  0x07|SHIFT,       // D
  0x08|SHIFT,       // E
  0x09|SHIFT,       // F
  0x0a|SHIFT,       // G
  0x0b|SHIFT,       // H
  0x0c|SHIFT,       // I
  0x0d|SHIFT,       // J
  0x0e|SHIFT,       // K
  0x0f|SHIFT,       // L
  0x10|SHIFT,       // M
  0x11|SHIFT,       // N
  0x12|SHIFT,       // O
  0x13|SHIFT,       // P
  0x14|SHIFT,       // Q
  0x15|SHIFT,       // R
  0x16|SHIFT,       // S
  0x17|SHIFT,       // T
  0x18|SHIFT,       // U
  0x19|SHIFT,       // V
  0x1a|SHIFT,       // W
  0x1b|SHIFT,       // X
  0x1c|SHIFT,       // Y
  0x1d|SHIFT,       // Z
  0x2f,             // [
  0x31,             // bslash
  0x30,             // ]
  0x23|SHIFT,       // ^
  0x2d|SHIFT,       // _
  0x35,             // `
  0x04,             // a
  0x05,             // b
  0x06,             // c
  0x07,             // d
  0x08,             // e
  0x09,             // f
  0x0a,             // g
  0x0b,             // h
  0x0c,             // i
  0x0d,             // j
  0x0e,             // k
  0x0f,             // l
  0x10,             // m
  0x11,             // n
  0x12,             // o
  0x13,             // p
  0x14,             // q
  0x15,             // r
  0x16,             // s
  0x17,             // t
  0x18,             // u
  0x19,             // v
  0x1a,             // w
  0x1b,             // x
  0x1c,             // y
  0x1d,             // z
  0x2f|SHIFT,       // {
  0x31|SHIFT,       // |
  0x30|SHIFT,       // }
  0x35|SHIFT,       // ~
  0                // DEL
};

uint8_t USBPutChar(uint8_t c);

size_t BleComboAbs::press(uint8_t k)
{
  uint8_t i;
  if (k >= 136) {
    k = k - 136;
  } else if (k >= 128) {
    _keyReport.modifiers |= (1 << (k - 128));
    k = 0;
  } else {
    k = pgm_read_byte(_asciimap + k);
    if (!k) {
      setWriteError();
      return 0;
    }
    if (k & SHIFT) {
      _keyReport.modifiers |= 0x02;
      k &= 0x7F;
    }
  }

  if (_keyReport.keys[0] != k && _keyReport.keys[1] != k &&
      _keyReport.keys[2] != k && _keyReport.keys[3] != k &&
      _keyReport.keys[4] != k && _keyReport.keys[5] != k) {

    for (i = 0; i < 6; i++) {
      if (_keyReport.keys[i] == 0x00) {
        _keyReport.keys[i] = k;
        break;
      }
    }
    if (i == 6) {
      setWriteError();
      return 0;
    }
  }
  sendKeyboardReport(&_keyReport);
  return 1;
}

size_t BleComboAbs::release(uint8_t k)
{
  uint8_t i;
  if (k >= 136) {
    k = k - 136;
  } else if (k >= 128) {
    _keyReport.modifiers &= ~(1 << (k - 128));
    k = 0;
  } else {
    k = pgm_read_byte(_asciimap + k);
    if (!k) {
      return 0;
    }
    if (k & SHIFT) {
      _keyReport.modifiers &= ~(0x02);
      k &= 0x7F;
    }
  }

  for (i = 0; i < 6; i++) {
    if (0 != k && _keyReport.keys[i] == k) {
      _keyReport.keys[i] = 0x00;
    }
  }

  sendKeyboardReport(&_keyReport);
  return 1;
}

void BleComboAbs::releaseAll(void)
{
  _keyReport.keys[0] = 0;
  _keyReport.keys[1] = 0;
  _keyReport.keys[2] = 0;
  _keyReport.keys[3] = 0;
  _keyReport.keys[4] = 0;
  _keyReport.keys[5] = 0;
  _keyReport.modifiers = 0;
  sendKeyboardReport(&_keyReport);
}

size_t BleComboAbs::write(uint8_t c)
{
  uint8_t p = press(c);
  release(c);
  return p;
}

size_t BleComboAbs::write(const uint8_t* buffer, size_t size)
{
  size_t n = 0;
  while (size--) {
    if (*buffer != '\r') {
      if (write(*buffer)) {
        n++;
      } else {
        break;
      }
    }
    buffer++;
  }
  return n;
}

void BleComboAbs::clickAbs(int16_t x, int16_t y)
{
  moveAbs(x, y);
  releaseAbs();
}

void BleComboAbs::moveAbs(int16_t x, int16_t y)
{
  sendAbsMouseReport(3, x, y);
  absPressed = true;
}

void BleComboAbs::releaseAbs(void)
{
  sendAbsMouseReport(0, 0, 0);
  absPressed = false;
}

bool BleComboAbs::isAbsPressed(void)
{
  return absPressed;
}

void BleComboAbs::onConnect(BLEServer* pServer)
{
  this->connected = true;

  if (hid != 0) {
    hid->setBatteryLevel(batteryLevel);
  }

#if !defined(USE_NIMBLE)
  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
  desc = (BLE2902*)this->inputAbsMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(true);
#endif
}

void BleComboAbs::onDisconnect(BLEServer* pServer)
{
  this->connected = false;

#if !defined(USE_NIMBLE)
  BLE2902* desc = (BLE2902*)this->inputKeyboard->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  desc = (BLE2902*)this->inputAbsMouse->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
  desc->setNotifications(false);
  advertising->start();
#endif
}

void BleComboAbs::onWrite(BLECharacteristic* me)
{
  uint8_t* value = (uint8_t*)(me->getValue().c_str());
  (void)value;
  ESP_LOGI(LOG_TAG, "keyboard LED update: %d", *value);
}

void BleComboAbs::delay_ms(uint64_t ms)
{
  uint64_t m = esp_timer_get_time();
  if (ms) {
    uint64_t e = (m + (ms * 1000));
    if (m > e) {
      while (esp_timer_get_time() > e) { }
    }
    while (esp_timer_get_time() < e) { }
  }
}

#endif // CONFIG_BT_ENABLED
