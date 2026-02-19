// Timeout-Logik für Webserver
unsigned long webserverStartTime = 0;
unsigned long lastWebRequestTime = 0;
bool webserverActive = false;
const unsigned long WEBSERVER_TIMEOUT = 600000; // 10 Minuten

#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "BleComboAbs.h"
WebServer server(80);
WiFiManager wm;
bool debugOutput = false;
bool batteryEnabled = false;
int batteryPin = -1;
float batteryScale = 2.0f;
uint32_t lastBatteryMv = 0;
unsigned long batteryLastRead = 0;
int lastBatteryPercent = -1;
const unsigned long BATTERY_READ_INTERVAL = 60000;
BleComboAbs bleCombo;

template <typename T>
void debugPrint(const T& value) {
  if (debugOutput) {
    Serial.print(value);
  }
}

template <typename T>
void debugPrintln(const T& value) {
  if (debugOutput) {
    Serial.println(value);
  }
}

float readBatteryVoltage() {
  if (batteryPin < 0) {
    return -1.0f;
  }
  const int samples = 16;
  (void)analogReadMilliVolts(batteryPin); // dummy read to charge ADC sampling cap
  delay(2);
  uint32_t mv = 0;
  for (int i = 0; i < samples; i++) {
    mv += analogReadMilliVolts(batteryPin);
    delay(2);
  }
  lastBatteryMv = (uint32_t)(mv / samples);
  float v = (lastBatteryMv / 1000.0f);
  return v * batteryScale;
}

int batteryPercentFromVoltage(float v) {
  struct Point { float v; int p; };
  const Point points[] = {
    {2.5f, 0},
    {3.0f, 5},
    {3.2f, 8},
    {3.3f, 15},
    {3.4f, 25},
    {3.5f, 35},
    {3.6f, 45},
    {3.7f, 55},
    {3.8f, 70},
    {3.9f, 80},
    {4.0f, 90},
    {4.1f, 95},
    {4.2f, 100}
  };
  if (v <= points[0].v) return points[0].p;
  const int count = sizeof(points) / sizeof(points[0]);
  if (v >= points[count - 1].v) return points[count - 1].p;
  for (int i = 0; i < count - 1; i++) {
    if (v >= points[i].v && v <= points[i + 1].v) {
      float t = (v - points[i].v) / (points[i + 1].v - points[i].v);
      return (int)(points[i].p + t * (points[i + 1].p - points[i].p));
    }
  }
  return 0;
}

void updateBatteryLevel(bool force = false) {
  if (!batteryEnabled || batteryPin < 0) {
    return;
  }
  float v = readBatteryVoltage();
  if (v <= 0.0f) {
    return;
  }
  int percent = batteryPercentFromVoltage(v);
  if (debugOutput) {
    Serial.print("[DEBUG] Battery raw=");
    Serial.print(lastBatteryMv);
    Serial.print("mV, V=");
    Serial.print(v, 3);
    Serial.print(" -> ");
    Serial.print(percent);
    Serial.println("%");
  }
  if (force || percent != lastBatteryPercent) {
    bleCombo.setBatteryLevel((uint8_t)percent);
    lastBatteryPercent = percent;
  }
}

// Hilfsfunktion: config.json als String laden
String loadConfigString() {
  if (!LittleFS.begin(true)) return "{}";
  File file = LittleFS.open("/config.json", "r");
  if (!file) return "{}";
  String content = file.readString();
  return content;
}

// Hilfsfunktion: config.json speichern
bool saveConfigString(const String& json) {
  if (!LittleFS.begin(true)) return false;
  File file = LittleFS.open("/config.json", "w");
  if (!file) return false;
  file.print(json);
  file.close();
  return true;
}

// HTML Editor Seite mit Formular und dynamischer Button-Liste
const char* configEditorHTML = R"rawliteral(
<!DOCTYPE html>
<html lang='de'>
<head>
  <meta charset='UTF-8'>
  <title>Keypad Config Editor</title>
  <style>
    body { font-family: sans-serif; max-width: 800px; margin: 2em auto; background: #f8f8f8; }
    h2 { color: #333; }
    label { display: block; margin-top: 1em; }
    input, select { margin: 0.2em 0 0.5em 0; padding: 0.2em; }
    .button-list { margin: 1em 0; }
    .button-entry { background: #fff; border: 1px solid #ccc; padding: 1em; margin-bottom: 0.5em; border-radius: 6px; }
    .remove-btn { background: #e74c3c; color: #fff; border: none; padding: 0.3em 0.8em; border-radius: 4px; cursor: pointer; float: right; }
    .add-btn { background: #27ae60; color: #fff; border: none; padding: 0.5em 1em; border-radius: 4px; cursor: pointer; margin-top: 1em; }
    #msg { margin-top: 1em; color: #2980b9; }
  </style>
</head>
<body>
<h2>Keypad Konfiguration</h2>
<form id='cfgform' onsubmit='event.preventDefault(); saveCfg();'>
  <label>BLE Name: <input id='ble_name' name='ble_name'></label>
  <label>WLAN SSID: <input id='wifi_ssid' name='wifi_ssid'></label>
  <label>WLAN Passwort: <input id='wifi_pass' name='wifi_pass' type='password'></label>
  <label>Doppelklick-Zeit (ms): <input id='doubleClickTime' name='doubleClickTime' type='number'></label>
  <label>Langklick-Zeit (ms): <input id='longPressTime' name='longPressTime' type='number'></label>
  <label>Battery aktiv: <input id='battery_enabled' name='battery_enabled' type='checkbox'></label>
  <label>Battery Pin: <input id='battery_pin' name='battery_pin' type='number'></label>
  <label>BLE LED Pin: <input id='ble_led_pin' name='ble_led_pin' type='number'></label>
  <label>BLE LED invertieren: <input id='ble_led_invert' name='ble_led_invert' type='checkbox'></label>
  <label>Debug Ausgabe: <input id='debug_ble' name='debug_ble' type='checkbox'></label>

  <h3>Buttons</h3>
  <div id='button-list' class='button-list'></div>
  <button type='button' class='add-btn' onclick='addButton()'>Button hinzufügen</button>

  <h3>Mouse Actions</h3>
  <div id='mouse-action-list' class='button-list'></div>
  <button type='button' class='add-btn' onclick='addMouseAction()'>Mouse Action hinzufügen</button>
  <br><br>
  <button type='submit'>Speichern</button>
</form>
<div id='msg'></div>
<script>
let config = {};
let buttonList = document.getElementById('button-list');
let mouseActionList = document.getElementById('mouse-action-list');

function renderButtons() {
  buttonList.innerHTML = '';
  config.buttons.forEach((btn, idx) => {
    let div = document.createElement('div');
    div.className = 'button-entry';
    div.innerHTML = `
      <button type='button' class='remove-btn' onclick='removeButton(${idx})'>Entfernen</button>
      <b>Button ${idx+1}</b><br>
      Pin: <input type='number' value='${btn.pin}' onchange='updateButton(${idx},"pin",this.value)'>
      Key normal: <input maxlength='1' value='${btn.key_normal||""}' onchange='updateButton(${idx},"key_normal",this.value)'>
      Key double: <input maxlength='1' value='${btn.key_double||""}' onchange='updateButton(${idx},"key_double",this.value)'>
      Key long: <input maxlength='1' value='${btn.key_long||""}' onchange='updateButton(${idx},"key_long",this.value)'>
      Mode: <select onchange='updateButton(${idx},"mode",this.value)'>
        <option value='pullup' ${btn.mode=="pullup"?"selected":""}>pullup</option>
        <option value='pulldown' ${btn.mode=="pulldown"?"selected":""}>pulldown</option>
        <option value='input' ${btn.mode=="input"?"selected":""}>input</option>
      </select>
      Debounce: <input type='number' value='${btn.debounce||100}' onchange='updateButton(${idx},"debounce",this.value)'>
    `;
    buttonList.appendChild(div);
  });
}

function renderMouseActions() {
  mouseActionList.innerHTML = '';
  config.mouse_actions.forEach((action, idx) => {
    let div = document.createElement('div');
    div.className = 'button-entry';
    div.innerHTML = `
      <button type='button' class='remove-btn' onclick='removeMouseAction(${idx})'>Entfernen</button>
      <b>Mouse Action ${idx+1}</b><br>
      Name: <input value='${action.name||""}' onchange='updateMouseAction(${idx},"name",this.value)'>
      X: <input type='number' value='${action.x||0}' onchange='updateMouseAction(${idx},"x",this.value)'>
      Y: <input type='number' value='${action.y||0}' onchange='updateMouseAction(${idx},"y",this.value)'>
    `;
    mouseActionList.appendChild(div);
  });
}

function updateButton(idx, key, value) {
  if(key=="pin"||key=="debounce") value = parseInt(value)||0;
  config.buttons[idx][key] = value;
}
function addButton() {
  config.buttons.push({pin:0,key_normal:"",key_double:"",key_long:"",mode:"pullup",debounce:100});
  document.getElementById('ble_led_pin').value = config.ble_led_pin||'';
  document.getElementById('ble_led_invert').checked = !!config.ble_led_invert;
  renderButtons();
}
function removeButton(idx) {
  config.buttons.splice(idx,1);
  renderButtons();
}
function updateMouseAction(idx, key, value) {
  if (key=="x"||key=="y") value = parseInt(value)||0;
  config.mouse_actions[idx][key] = value;
}
function addMouseAction() {
  config.mouse_actions.push({name:"",x:0,y:0});
  renderMouseActions();
}
function removeMouseAction(idx) {
  config.mouse_actions.splice(idx,1);
  renderMouseActions();
}
function fillForm() {
  document.getElementById('ble_name').value = config.ble_name||'';
  document.getElementById('wifi_ssid').value = config.wifi_ssid||'';
  document.getElementById('wifi_pass').value = config.wifi_pass||'';
  document.getElementById('doubleClickTime').value = config.doubleClickTime||400;
  document.getElementById('longPressTime').value = config.longPressTime||800;
  document.getElementById('battery_enabled').checked = !!config.battery_enabled;
  document.getElementById('battery_pin').value = config.battery_pin||'';
  document.getElementById('ble_led_pin').value = config.ble_led_pin||'';
  document.getElementById('ble_led_invert').checked = !!config.ble_led_invert;
  document.getElementById('debug_ble').checked = !!config.debug_ble;
  renderButtons();
  renderMouseActions();
}
function saveCfg() {
  config.ble_name = document.getElementById('ble_name').value;
  config.wifi_ssid = document.getElementById('wifi_ssid').value;
  config.wifi_pass = document.getElementById('wifi_pass').value;
  config.doubleClickTime = parseInt(document.getElementById('doubleClickTime').value)||400;
  config.longPressTime = parseInt(document.getElementById('longPressTime').value)||800;
  config.battery_enabled = document.getElementById('battery_enabled').checked;
  config.battery_pin = parseInt(document.getElementById('battery_pin').value)||-1;
  config.ble_led_pin = parseInt(document.getElementById('ble_led_pin').value)||-1;
  config.ble_led_invert = document.getElementById('ble_led_invert').checked;
  config.debug_ble = document.getElementById('debug_ble').checked;
  fetch('/save', {method:'POST', body:JSON.stringify(config)}).then(r=>r.text()).then(t=>{
    msg.innerText = t;
    setTimeout(() => {
      if (confirm('Konfiguration gespeichert!\nSoll das Gerät jetzt neu gestartet werden?')) {
        fetch('/restart', {method:'POST'});
      }
    }, 500);
  });
}
fetch('/config.json').then(r=>r.json()).then(j=>{
  config=j;
  if(!config.buttons)config.buttons=[];
  if(!config.mouse_actions)config.mouse_actions=[];
  fillForm();
});
</script>
</body></html>
)rawliteral";


enum ButtonState { BTN_IDLE, BTN_DEBOUNCE, BTN_PRESSED, BTN_WAIT_DOUBLE, BTN_LONG, BTN_RELEASED };
struct ButtonConfig {
  int pin;
  String key_normal;
  String key_double;
  String key_long;
  String mode;
  int debounce;
  int lastState;
  unsigned long lastChange;
  unsigned long pressStart;
  unsigned long lastRelease;
  ButtonState state;
  bool doubleClickPending;
};
ButtonConfig buttons[12];
int buttonCount = 0;
String bleName = "ESP32 Keyboard";
String wifiSSID = "";
String wifiPASS = "";

struct MouseAction {
  String name;
  int x;
  int y;
};
MouseAction mouseActions[8];
int mouseActionCount = 0;

// Globale Zeiten für Doppelklick und Langklick
unsigned long doubleClickTime = 400; // ms
unsigned long longPressTime = 800; // ms

int bleLedPin = -1;
bool bleLedInvert = false;
void executeMouseAction(const String& actionName);
void loadConfig() {
    debugPrint("[DEBUG] WLAN SSID: ");
    debugPrintln(wifiSSID);
    debugPrint("[DEBUG] WLAN PASS: ");
    debugPrintln(wifiPASS.length() > 0 ? "(gesetzt)" : "(leer)");
  if (!LittleFS.begin(true)) {
    debugPrintln("[DEBUG] LittleFS konnte nicht initialisiert werden!");
    return;
  }
  File file = LittleFS.open("/config.json");
  if (!file) {
    debugPrintln("[DEBUG] /config.json konnte nicht geöffnet werden!");
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, file);
  if (err) {
    debugPrint("[DEBUG] Fehler beim Parsen von config.json: ");
    debugPrintln(err.c_str());
    return;
  }
  if (doc.containsKey("ble_name")) {
    bleName = doc["ble_name"].as<String>();
  }
  if (doc.containsKey("doubleClickTime")) {
    doubleClickTime = doc["doubleClickTime"].as<unsigned long>();
  }
  if (doc.containsKey("longPressTime")) {
    longPressTime = doc["longPressTime"].as<unsigned long>();
  }
  if (doc.containsKey("battery_enabled")) {
    batteryEnabled = doc["battery_enabled"].as<bool>();
  } else {
    batteryEnabled = false;
  }
  if (doc.containsKey("battery_pin")) {
    batteryPin = doc["battery_pin"].as<int>();
  } else {
    batteryPin = -1;
  }
  if (doc.containsKey("battery_scale")) {
    float scale = doc["battery_scale"].as<float>();
    if (scale > 0.0f) {
      batteryScale = scale;
    }
  }
  if (doc.containsKey("wifi_ssid")) {
    wifiSSID = doc["wifi_ssid"].as<String>();
  }
  if (doc.containsKey("wifi_pass")) {
    wifiPASS = doc["wifi_pass"].as<String>();
  }
  if (doc.containsKey("ble_led_pin")) {
    bleLedPin = doc["ble_led_pin"].as<int>();
  } else {
    bleLedPin = -1;
  }
  if (doc.containsKey("ble_led_invert")) {
    bleLedInvert = doc["ble_led_invert"].as<bool>();
  } else {
    bleLedInvert = false;
  }
  if (doc.containsKey("debug_ble")) {
    debugOutput = doc["debug_ble"].as<bool>();
  } else {
    debugOutput = false;
  }
  buttonCount = doc["buttons"].size();
  for (int i = 0; i < buttonCount; i++) {
    buttons[i].pin = doc["buttons"][i]["pin"].as<int>();
    if (doc["buttons"][i].containsKey("key_normal"))
      buttons[i].key_normal = doc["buttons"][i]["key_normal"].as<String>();
    else if (doc["buttons"][i].containsKey("key"))
      buttons[i].key_normal = doc["buttons"][i]["key"].as<String>();
    else
      buttons[i].key_normal = "A";
    if (doc["buttons"][i].containsKey("key_double"))
      buttons[i].key_double = doc["buttons"][i]["key_double"].as<String>();
    else
      buttons[i].key_double = buttons[i].key_normal;
    if (doc["buttons"][i].containsKey("key_long"))
      buttons[i].key_long = doc["buttons"][i]["key_long"].as<String>();
    else
      buttons[i].key_long = buttons[i].key_normal;
    if (doc["buttons"][i].containsKey("mode")) {
      buttons[i].mode = doc["buttons"][i]["mode"].as<String>();
    } else {
      buttons[i].mode = "pullup";
    }
    if (doc["buttons"][i].containsKey("debounce")) {
      buttons[i].debounce = doc["buttons"][i]["debounce"].as<int>();
    } else {
      buttons[i].debounce = 100;
    }
    buttons[i].lastState = HIGH;
    buttons[i].lastChange = 0;
    buttons[i].pressStart = 0;
    buttons[i].lastRelease = 0;
    buttons[i].state = BTN_IDLE;
    buttons[i].doubleClickPending = false;
  }

  // Mausaktionen laden
  mouseActionCount = 0;
  if (doc.containsKey("mouse_actions")) {
    JsonArray arr = doc["mouse_actions"].as<JsonArray>();
    for (JsonObject obj : arr) {
      if (mouseActionCount < 8) {
        mouseActions[mouseActionCount].name = obj["name"].as<String>();
        mouseActions[mouseActionCount].x = obj["x"].as<int>();
        mouseActions[mouseActionCount].y = obj["y"].as<int>();
        mouseActionCount++;
      }
    }
  }

  file.close();
  debugPrintln("[DEBUG] Geladene Konfiguration:");
  debugPrint("[DEBUG] BLE-Name: ");
  debugPrintln(bleName);
  debugPrint("[DEBUG] ButtonCount: ");
  debugPrintln(buttonCount);
  for (int i = 0; i < buttonCount; i++) {
    debugPrint("[DEBUG] Button ");
    debugPrint(i);
    debugPrint(": Pin ");
    debugPrint(buttons[i].pin);
    debugPrint(", Key_normal '");
    debugPrint(buttons[i].key_normal);
    debugPrint("', Key_double '");
    debugPrint(buttons[i].key_double);
    debugPrint("', Key_long '");
    debugPrint(buttons[i].key_long);
    debugPrint("', Mode: ");
    debugPrintln(buttons[i].mode);
  }
}

// Hilfsfunktion: Mausaktion ausführen
void executeMouseAction(const String& actionName) {
  for (int i = 0; i < mouseActionCount; i++) {
    if (mouseActions[i].name == actionName) {
      int mx = mouseActions[i].x;
      int my = mouseActions[i].y;
      if (!bleCombo.isConnected()) {
        debugPrintln("[DEBUG] BLE nicht verbunden, Aktion ignoriert");
        return;
      }
      if (mx < 0) mx = 0;
      if (my < 0) my = 0;
      if (mx > 10000) mx = 10000;
      if (my > 10000) my = 10000;
      debugPrint("[DEBUG] Abs Mouse action: ");
      debugPrint(actionName);
      debugPrint(" x=");
      debugPrint(mx);
      debugPrint(" y=");
      debugPrintln(my);
      bleCombo.clickAbs(mx, my);
      return;
    }
  }
}

void setup() {
    Serial.begin(115200);
    delay(5000); // Warte auf Serial-Port Initialisierung
    debugPrintln("[DEBUG] setup() gestartet");
    debugPrintln("[DEBUG] Serial initialisiert");
    // Captive Portal starten, falls kein WLAN konfiguriert
    loadConfig();
    bool wifiConnected = false;
    if (wifiSSID.length() > 0) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
      debugPrint("[DEBUG] Verbinde mit WLAN: ");
      debugPrintln(wifiSSID);
      unsigned long startAttempt = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
        delay(500);
        Serial.print(".");
      }
      Serial.println();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("WLAN-Verbindung erfolgreich! IP: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("WLAN-Verbindung fehlgeschlagen!");
      }
      wifiConnected = (WiFi.status() == WL_CONNECTED);
    }
    if (!wifiConnected) {
      Serial.print("Keine WLAN-Verbindung, Captive Portal aktiv!");
      WiFi.mode(WIFI_AP);
      bool ap = WiFi.softAP("Keypad-Config");
      Serial.print("Access Point gestartet: ");
      Serial.println(ap ? "OK" : "Fehler");
      Serial.print("AP-IP: ");
      Serial.println(WiFi.softAPIP());
      // Webserver Endpunkte
      server.on("/", []() {
        lastWebRequestTime = millis();
        debugPrintln("[DEBUG] HTTP GET /");
        server.send(200, "text/html", configEditorHTML);
      });
      server.on("/config.json", []() {
        lastWebRequestTime = millis();
        debugPrintln("[DEBUG] HTTP GET /config.json");
        server.send(200, "application/json", loadConfigString());
      });
      server.on("/save", HTTP_POST, []() {
        lastWebRequestTime = millis();
        debugPrintln("[DEBUG] HTTP POST /save");
        String body = server.arg("plain");
        if (saveConfigString(body)) {
          debugPrintln("[DEBUG] config.json gespeichert!");
          server.send(200, "text/plain", "Gespeichert!");
        } else {
          debugPrintln("[DEBUG] Fehler beim Speichern von config.json!");
          server.send(500, "text/plain", "Fehler beim Speichern!");
        }
      });
      server.begin();
      webserverStartTime = millis();
      lastWebRequestTime = millis();
      webserverActive = true;
      Serial.println("Webserver gestartet (Port 80)");
    } else {
      Serial.print("WLAN verbunden: ");
      Serial.println(WiFi.localIP());
      // Webserver für lokale Bearbeitung (optional)
      server.on("/", []() {
        lastWebRequestTime = millis();
        debugPrintln("[DEBUG] HTTP GET /");
        server.send(200, "text/html", configEditorHTML);
      });
      server.on("/config.json", []() {
        lastWebRequestTime = millis();
        debugPrintln("[DEBUG] HTTP GET /config.json");
        server.send(200, "application/json", loadConfigString());
      });
      server.on("/save", HTTP_POST, []() {
        lastWebRequestTime = millis();
        debugPrintln("[DEBUG] HTTP POST /save");
        String body = server.arg("plain");
        if (saveConfigString(body)) {
          debugPrintln("[DEBUG] config.json gespeichert!");
          server.send(200, "text/plain", "Gespeichert!");
        } else {
          debugPrintln("[DEBUG] Fehler beim Speichern von config.json!");
          server.send(500, "text/plain", "Fehler beim Speichern!");
        }
      });
      server.begin();
      webserverStartTime = millis();
      lastWebRequestTime = millis();
      webserverActive = true;
      debugPrintln("[DEBUG] Webserver gestartet (Port 80)");
    }
  //pinMode(8, OUTPUT);
  Serial.begin(115200);
  delay(5000); // Warte auf Serial-Port Initialisierung
  debugPrintln("[DEBUG] setup() gestartet");
  debugPrintln("[DEBUG] Serial initialisiert");
  loadConfig();
  debugPrintln("[DEBUG] Konfiguration geladen");
  for (int i = 0; i < buttonCount; i++) {
    debugPrint("Init Button ");
    debugPrint(i);
    debugPrint(": Pin ");
    debugPrint(buttons[i].pin);
    debugPrint(", Key_normal '");
    debugPrint(buttons[i].key_normal);
    debugPrint("', Key_double '");
    debugPrint(buttons[i].key_double);
    debugPrint("', Key_long '");
    debugPrint(buttons[i].key_long);
    debugPrint("', Mode: ");
    debugPrint(buttons[i].mode);
    debugPrint(", Debounce: ");
    debugPrintln(buttons[i].debounce);
    if (buttons[i].pin < 0 || buttons[i].pin > 39) {
      debugPrint("Warnung: Ungültiger GPIO: ");
      debugPrintln(buttons[i].pin);
      continue;
    }
    if (buttons[i].mode == "pullup") {
      pinMode(buttons[i].pin, INPUT_PULLUP);
      debugPrintln("[DEBUG] pinMode INPUT_PULLUP gesetzt");
    } else if (buttons[i].mode == "pulldown") {
      pinMode(buttons[i].pin, INPUT_PULLDOWN);
      debugPrintln("[DEBUG] pinMode INPUT_PULLDOWN gesetzt");
    } else {
      pinMode(buttons[i].pin, INPUT);
      debugPrintln("[DEBUG] pinMode INPUT gesetzt");
    }
  }
  debugPrintln("[DEBUG] Alle Pins initialisiert");
  if (bleLedPin >= 0 && bleLedPin <= 39) {
    pinMode(bleLedPin, OUTPUT);
    digitalWrite(bleLedPin, bleLedInvert ? HIGH : LOW);
    debugPrint("[DEBUG] BLE LED Pin initialisiert: ");
    debugPrintln(bleLedPin);
    debugPrint("[DEBUG] BLE LED invertiert: ");
    debugPrintln(bleLedInvert ? "true" : "false");
  }
  if (batteryEnabled && batteryPin >= 0) {
    pinMode(batteryPin, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(batteryPin, ADC_11db);
    adcAttachPin(batteryPin);
    debugPrint("[DEBUG] Battery Pin initialisiert: ");
    debugPrintln(batteryPin);
  }
  bleCombo.setName(bleName.c_str());
  bleCombo.setDebug(debugOutput);
  debugPrintln("[DEBUG] BLE-Name gesetzt");
  bleCombo.begin();
  updateBatteryLevel(true);
  debugPrintln("[DEBUG] BLE Keyboard und Abs Mouse gestartet");
  Serial.print("Tastatur-Emulator gestartet (BLE-Modus, Name: ");
  Serial.print(bleName);
  Serial.println(")");
}

unsigned long bleLedLastToggle = 0;
bool bleLedState = false;
bool bleWasConnected = false;
unsigned long bleDisconnectTime = 0;
bool bleFastBlinkActive = false;
void loop() {
    if (webserverActive) {
      server.handleClient();
      // Timeout prüfen
      if ((millis() - lastWebRequestTime > WEBSERVER_TIMEOUT) && (millis() - webserverStartTime > WEBSERVER_TIMEOUT)) {
        debugPrintln("[DEBUG] Webserver Timeout, stoppe Webserver und Access Point!");
        server.stop();
        WiFi.softAPdisconnect(true);
        webserverActive = false;
      }
    }

  unsigned long now = millis();
  // Bluetooth LED Status blinken
  bool bleConnected = bleCombo.isConnected();
  if (bleLedPin >= 0 && bleLedPin <= 39) {
    auto ledWrite = [&](bool on) {
      digitalWrite(bleLedPin, bleLedInvert ? !on : on);
    };
    if (bleConnected) {
      ledWrite(true); // LED dauerhaft an bei Verbindung
      bleWasConnected = true;
      bleFastBlinkActive = false;
    } else {
      if (bleWasConnected) {
        bleDisconnectTime = now;
        bleFastBlinkActive = true;
        bleWasConnected = false;
      }
      // blinke für 5 secunden nach einem Disconnect schnell, danach wieder langsam
      if (bleFastBlinkActive && (now - bleDisconnectTime < 5000)) {
        if (now - bleLedLastToggle > 100) { // schnell blinken (10Hz)
          bleLedState = !bleLedState;
          ledWrite(bleLedState);
          bleLedLastToggle = now;
        }
      } else {
        bleFastBlinkActive = false;
        if (now - bleLedLastToggle > 500) { // langsam blinken (1Hz) wenn nicht verbunden 
          bleLedState = !bleLedState;
          ledWrite(bleLedState);
          bleLedLastToggle = now;
        }
      }
    }
  }
  for (int i = 0; i < buttonCount; i++) {
    int pinState = digitalRead(buttons[i].pin);
    switch (buttons[i].state) {
        case BTN_IDLE:
          if (pinState == LOW) {
            buttons[i].state = BTN_DEBOUNCE;
            buttons[i].lastChange = now;
          }
          break;
        case BTN_DEBOUNCE:
          if (pinState == LOW && (now - buttons[i].lastChange > buttons[i].debounce)) {
            buttons[i].state = BTN_PRESSED;
            buttons[i].pressStart = now;
          } else if (pinState == HIGH) {
            buttons[i].state = BTN_IDLE;
          }
          break;
        case BTN_PRESSED:
          if (pinState == HIGH) {
            // Button wurde kurz gedrückt
            if (buttons[i].doubleClickPending && (now - buttons[i].lastRelease < doubleClickTime)) {
              // Doppelklick erkannt
              debugPrint("-> Doppelklick: ");
              debugPrintln(buttons[i].key_double);
              String doubleName = buttons[i].key_double;
              bool mouseDone = false;
              for (int m = 0; m < mouseActionCount; m++) {
                if (mouseActions[m].name == doubleName) {
                  executeMouseAction(doubleName);
                  mouseDone = true;
                  break;
                }
              }
              if (!mouseDone && bleConnected) {
                Serial.print("Keyboard double key: ");
                Serial.println(buttons[i].key_double);
                bleCombo.press((uint8_t)buttons[i].key_double[0]);
                delay(100);
                bleCombo.release((uint8_t)buttons[i].key_double[0]);
              }
              buttons[i].doubleClickPending = false;
              buttons[i].state = BTN_IDLE;
            } else {
              // Warte auf zweiten Klick
              buttons[i].doubleClickPending = true;
              buttons[i].lastRelease = now;
              buttons[i].state = BTN_WAIT_DOUBLE;
            }
          } else if (now - buttons[i].pressStart > longPressTime) {
            // Langklick erkannt
            Serial.print("-> Langklick: ");
            Serial.println(buttons[i].key_long);
            String longName = buttons[i].key_long;
            bool mouseDone = false;
            for (int m = 0; m < mouseActionCount; m++) {
              if (mouseActions[m].name == longName) {
                executeMouseAction(longName);
                mouseDone = true;
                break;
              }
            }
            if (!mouseDone && bleConnected) {
              Serial.print("[DEBUG] Keyboard long key: ");
              Serial.println(buttons[i].key_long);
              bleCombo.press((uint8_t)buttons[i].key_long[0]);
              delay(100);
              bleCombo.release((uint8_t)buttons[i].key_long[0]);
            }
            buttons[i].doubleClickPending = false;
            buttons[i].state = BTN_LONG;
          }
          break;
        case BTN_WAIT_DOUBLE:
          if (pinState == LOW) {
            buttons[i].state = BTN_DEBOUNCE;
            buttons[i].lastChange = now;
          } else if (now - buttons[i].lastRelease > doubleClickTime) {
            // Zeit abgelaufen, Normalklick
            Serial.print("-> Normalklick: ");
            Serial.println(buttons[i].key_normal);
            String normalName = buttons[i].key_normal;
            bool mouseDone = false;
            for (int m = 0; m < mouseActionCount; m++) {
              if (mouseActions[m].name == normalName) {
                executeMouseAction(normalName);
                mouseDone = true;
                break;
              }
            }
            if (!mouseDone && bleConnected) {
              Serial.print("Keyboard normal key: ");
              Serial.println(buttons[i].key_normal);
              bleCombo.press((uint8_t)buttons[i].key_normal[0]);
              delay(100);
              bleCombo.release((uint8_t)buttons[i].key_normal[0]);
            }
            buttons[i].doubleClickPending = false;
            buttons[i].state = BTN_IDLE;
          }
          break;
        case BTN_LONG:
          if (pinState == HIGH) {
            buttons[i].state = BTN_IDLE;
          }
          break;
        default:
          buttons[i].state = BTN_IDLE;
          break;
      }
    buttons[i].lastState = pinState;
  }

  if (batteryEnabled && batteryPin >= 0) {
    if (now - batteryLastRead > BATTERY_READ_INTERVAL) {
      batteryLastRead = now;
      updateBatteryLevel(false);
    }
  }
}
  