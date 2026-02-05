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
#include <BleKeyboard.h>
WebServer server(80);
WiFiManager wm;

// Hilfsfunktion: config.json als String laden
String loadConfigString() {
  if (!LittleFS.begin(true)) return "{}";
  File file = LittleFS.open("/config.json", "r");
  if (!file) return "{}";
  String content = file.readString();
  file.close();
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
  <label>BLE LED Pin: <input id='ble_led_pin' name='ble_led_pin' type='number'></label>
  <label>BLE LED invertieren: <input id='ble_led_invert' name='ble_led_invert' type='checkbox'></label>

  <h3>Buttons</h3>
  <div id='button-list' class='button-list'></div>
  <button type='button' class='add-btn' onclick='addButton()'>Button hinzufügen</button>
  <br><br>
  <button type='submit'>Speichern</button>
</form>
<div id='msg'></div>
<script>
let config = {};
let buttonList = document.getElementById('button-list');

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
function fillForm() {
  document.getElementById('ble_name').value = config.ble_name||'';
  document.getElementById('wifi_ssid').value = config.wifi_ssid||'';
  document.getElementById('wifi_pass').value = config.wifi_pass||'';
  document.getElementById('doubleClickTime').value = config.doubleClickTime||400;
  document.getElementById('longPressTime').value = config.longPressTime||800;
  renderButtons();
}
function saveCfg() {
  config.ble_name = document.getElementById('ble_name').value;
  config.wifi_ssid = document.getElementById('wifi_ssid').value;
  config.wifi_pass = document.getElementById('wifi_pass').value;
  config.doubleClickTime = parseInt(document.getElementById('doubleClickTime').value)||400;
  config.longPressTime = parseInt(document.getElementById('longPressTime').value)||800;
  config.ble_led_pin = parseInt(document.getElementById('ble_led_pin').value)||-1;
  config.ble_led_invert = document.getElementById('ble_led_invert').checked;
  fetch('/save', {method:'POST', body:JSON.stringify(config)}).then(r=>r.text()).then(t=>msg.innerText=t);
}
fetch('/config.json').then(r=>r.json()).then(j=>{config=j;if(!config.buttons)config.buttons=[];fillForm();});
</script>
</body></html>
)rawliteral";


enum ButtonState { BTN_IDLE, BTN_DEBOUNCE, BTN_PRESSED, BTN_WAIT_DOUBLE, BTN_LONG, BTN_RELEASED };
struct ButtonConfig {
  int pin;
  char key_normal;
  char key_double;
  char key_long;
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
BleKeyboard bleKeyboard;

// Globale Zeiten für Doppelklick und Langklick
unsigned long doubleClickTime = 400; // ms
unsigned long longPressTime = 800; // ms

int bleLedPin = -1;
bool bleLedInvert = false;
void loadConfig() {
    Serial.print("[DEBUG] WLAN SSID: ");
    Serial.println(wifiSSID);
    Serial.print("[DEBUG] WLAN PASS: ");
    Serial.println(wifiPASS.length() > 0 ? "(gesetzt)" : "(leer)");
  if (!LittleFS.begin(true)) {
    Serial.println("[DEBUG] LittleFS konnte nicht initialisiert werden!");
    return;
  }
  File file = LittleFS.open("/config.json");
  if (!file) {
    Serial.println("[DEBUG] /config.json konnte nicht geöffnet werden!");
    return;
  }
  StaticJsonDocument<1024> doc;
  DeserializationError err = deserializeJson(doc, file);
  if (err) {
    Serial.print("[DEBUG] Fehler beim Parsen von config.json: ");
    Serial.println(err.c_str());
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
  buttonCount = doc["buttons"].size();
  for (int i = 0; i < buttonCount; i++) {
    buttons[i].pin = doc["buttons"][i]["pin"].as<int>();
    // Neue Struktur: key_normal, key_double, key_long
    if (doc["buttons"][i].containsKey("key_normal"))
      buttons[i].key_normal = doc["buttons"][i]["key_normal"].as<const char*>()[0];
    else if (doc["buttons"][i].containsKey("key"))
      buttons[i].key_normal = doc["buttons"][i]["key"].as<const char*>()[0];
    else
      buttons[i].key_normal = 'A';
    if (doc["buttons"][i].containsKey("key_double"))
      buttons[i].key_double = doc["buttons"][i]["key_double"].as<const char*>()[0];
    else
      buttons[i].key_double = buttons[i].key_normal;
    if (doc["buttons"][i].containsKey("key_long"))
      buttons[i].key_long = doc["buttons"][i]["key_long"].as<const char*>()[0];
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
  file.close();
  Serial.println("[DEBUG] Geladene Konfiguration:");
  Serial.print("[DEBUG] BLE-Name: ");
  Serial.println(bleName);
  Serial.print("[DEBUG] ButtonCount: ");
  Serial.println(buttonCount);
  for (int i = 0; i < buttonCount; i++) {
    Serial.print("[DEBUG] Button ");
    Serial.print(i);
    Serial.print(": Pin ");
    Serial.print(buttons[i].pin);
    Serial.print(", Key_normal '");
    Serial.print(buttons[i].key_normal);
    Serial.print("', Key_double '");
    Serial.print(buttons[i].key_double);
    Serial.print("', Key_long '");
    Serial.print(buttons[i].key_long);
    Serial.print("', Mode: ");
    Serial.println(buttons[i].mode);
  }
}

void setup() {
    Serial.begin(115200);
    delay(5000); // Warte auf Serial-Port Initialisierung
    Serial.println("[DEBUG] setup() gestartet");
    Serial.println("[DEBUG] Serial initialisiert");
    // Captive Portal starten, falls kein WLAN konfiguriert
    loadConfig();
    bool wifiConnected = false;
    if (wifiSSID.length() > 0) {
      WiFi.mode(WIFI_STA);
      WiFi.begin(wifiSSID.c_str(), wifiPASS.c_str());
      Serial.print("[DEBUG] Verbinde mit WLAN: ");
      Serial.println(wifiSSID);
      unsigned long startAttempt = millis();
      while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
        delay(500);
        Serial.print(".");
      }
      Serial.println();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("[DEBUG] WLAN-Verbindung erfolgreich! IP: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("[DEBUG] WLAN-Verbindung fehlgeschlagen!");
      }
      wifiConnected = (WiFi.status() == WL_CONNECTED);
    }
    if (!wifiConnected) {
      Serial.println("[DEBUG] Keine WLAN-Verbindung, Captive Portal aktiv!");
      WiFi.mode(WIFI_AP);
      bool ap = WiFi.softAP("Keypad-Config");
      Serial.print("[DEBUG] Access Point gestartet: ");
      Serial.println(ap ? "OK" : "Fehler");
      Serial.print("[DEBUG] AP-IP: ");
      Serial.println(WiFi.softAPIP());
      // Webserver Endpunkte
      server.on("/", []() {
        lastWebRequestTime = millis();
        Serial.println("[DEBUG] HTTP GET /");
        server.send(200, "text/html", configEditorHTML);
      });
      server.on("/config.json", []() {
        lastWebRequestTime = millis();
        Serial.println("[DEBUG] HTTP GET /config.json");
        server.send(200, "application/json", loadConfigString());
      });
      server.on("/save", HTTP_POST, []() {
        lastWebRequestTime = millis();
        Serial.println("[DEBUG] HTTP POST /save");
        String body = server.arg("plain");
        if (saveConfigString(body)) {
          Serial.println("[DEBUG] config.json gespeichert!");
          server.send(200, "text/plain", "Gespeichert!");
        } else {
          Serial.println("[DEBUG] Fehler beim Speichern von config.json!");
          server.send(500, "text/plain", "Fehler beim Speichern!");
        }
      });
      server.begin();
      webserverStartTime = millis();
      lastWebRequestTime = millis();
      webserverActive = true;
      Serial.println("[DEBUG] Webserver gestartet (Port 80)");
    } else {
      Serial.print("[DEBUG] WLAN verbunden: ");
      Serial.println(WiFi.localIP());
      // Webserver für lokale Bearbeitung (optional)
      server.on("/", []() {
        lastWebRequestTime = millis();
        Serial.println("[DEBUG] HTTP GET /");
        server.send(200, "text/html", configEditorHTML);
      });
      server.on("/config.json", []() {
        lastWebRequestTime = millis();
        Serial.println("[DEBUG] HTTP GET /config.json");
        server.send(200, "application/json", loadConfigString());
      });
      server.on("/save", HTTP_POST, []() {
        lastWebRequestTime = millis();
        Serial.println("[DEBUG] HTTP POST /save");
        String body = server.arg("plain");
        if (saveConfigString(body)) {
          Serial.println("[DEBUG] config.json gespeichert!");
          server.send(200, "text/plain", "Gespeichert!");
        } else {
          Serial.println("[DEBUG] Fehler beim Speichern von config.json!");
          server.send(500, "text/plain", "Fehler beim Speichern!");
        }
      });
      server.begin();
      webserverStartTime = millis();
      lastWebRequestTime = millis();
      webserverActive = true;
      Serial.println("[DEBUG] Webserver gestartet (Port 80)");
    }
  //pinMode(8, OUTPUT);
  Serial.begin(115200);
  delay(5000); // Warte auf Serial-Port Initialisierung
  Serial.println("[DEBUG] setup() gestartet");
  Serial.println("[DEBUG] Serial initialisiert");
  loadConfig();
  Serial.println("[DEBUG] Konfiguration geladen");
  for (int i = 0; i < buttonCount; i++) {
    Serial.print("Init Button ");
    Serial.print(i);
    Serial.print(": Pin ");
    Serial.print(buttons[i].pin);
    Serial.print(", Key_normal '");
    Serial.print(buttons[i].key_normal);
    Serial.print("', Key_double '");
    Serial.print(buttons[i].key_double);
    Serial.print("', Key_long '");
    Serial.print(buttons[i].key_long);
    Serial.print("', Mode: ");
    Serial.print(buttons[i].mode);
    Serial.print(", Debounce: ");
    Serial.println(buttons[i].debounce);
    if (buttons[i].pin < 0 || buttons[i].pin > 39) {
      Serial.print("Warnung: Ungültiger GPIO: ");
      Serial.println(buttons[i].pin);
      continue;
    }
    if (buttons[i].mode == "pullup") {
      pinMode(buttons[i].pin, INPUT_PULLUP);
      Serial.println("[DEBUG] pinMode INPUT_PULLUP gesetzt");
    } else if (buttons[i].mode == "pulldown") {
      pinMode(buttons[i].pin, INPUT_PULLDOWN);
      Serial.println("[DEBUG] pinMode INPUT_PULLDOWN gesetzt");
    } else {
      pinMode(buttons[i].pin, INPUT);
      Serial.println("[DEBUG] pinMode INPUT gesetzt");
    }
  }
  Serial.println("[DEBUG] Alle Pins initialisiert");
  if (bleLedPin >= 0 && bleLedPin <= 39) {
    pinMode(bleLedPin, OUTPUT);
    digitalWrite(bleLedPin, bleLedInvert ? HIGH : LOW);
    Serial.print("[DEBUG] BLE LED Pin initialisiert: ");
    Serial.println(bleLedPin);
    Serial.print("[DEBUG] BLE LED invertiert: ");
    Serial.println(bleLedInvert ? "true" : "false");
  }
  bleKeyboard.setName(bleName.c_str());
  Serial.println("[DEBUG] BLE-Name gesetzt");
  bleKeyboard.begin();
  Serial.println("[DEBUG] BLE Keyboard gestartet");
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
        Serial.println("[DEBUG] Webserver Timeout, stoppe Webserver und Access Point!");
        server.stop();
        WiFi.softAPdisconnect(true);
        webserverActive = false;
      }
    }

  unsigned long now = millis();
  // Bluetooth LED Status blinken
  if (bleLedPin >= 0 && bleLedPin <= 39) {
    auto ledWrite = [&](bool on) {
      digitalWrite(bleLedPin, bleLedInvert ? !on : on);
    };
    if (bleKeyboard.isConnected()) {
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
  if (bleKeyboard.isConnected()) {
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
              Serial.print("-> Doppelklick: ");
              Serial.println(buttons[i].key_double);
              bleKeyboard.press(buttons[i].key_double);
              delay(100);
              bleKeyboard.release(buttons[i].key_double);
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
            bleKeyboard.press(buttons[i].key_long);
            delay(100);
            bleKeyboard.release(buttons[i].key_long);
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
            bleKeyboard.press(buttons[i].key_normal);
            delay(100);
            bleKeyboard.release(buttons[i].key_normal);
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
  }
}
  