# MyWhoosh ESP32 BLE Keypad für MYWhoosh Rollentrainer

Dieses Projekt implementiert ein BLE-Keypad auf Basis eines ESP32/ESP32-C3, um den Rollentrainer in der MYWhoosh-App komfortabel zu steuern. Es können pro Taste drei verschiedene Aktionen (Normalklick, Doppelklick, Langklick) ausgeführt werden. Die Tastenbelegung und Klickzeiten sind flexibel über eine Konfigurationsdatei anpassbar.

## Features

- **BLE-Tastatur-Emulation**: Funktioniert als drahtloses Keypad für MYWhoosh
- **BLE-Abs-Mouse (absolut)**: Mausaktionen mit absoluten Koordinaten (im gleichen BLE-Geraet)
- **Bluetooth-LED (BLE LED)**: Statusanzeige für Bluetooth-Verbindung (Pin und Logik konfigurierbar)
- **Mehrfachbelegung pro Taste**: Normalklick, Doppelklick, Langklick je GPIO
- **Konfigurierbar**: Tasten, Modi, Zeiten und BLE LED über `config.json` einstellbar
- **Kompatibel mit ESP32 und ESP32-C3**
- **Einfache Anpassung**: Keine Programmierkenntnisse für Tastenbelegung nötig

## Hardware
- ESP32 oder ESP32-C3 Devboard
- Taster (beliebige Anzahl, empfohlen: 4–12)
- Optional: Gehäuse, Pullup/Pulldown-Widerstände

## Installation & Inbetriebnahme

1. **Repository klonen**
   ```sh
   git clone https://github.com/dein-github/mywhoosh-keypad.git
   cd mywhoosh-keypad
   ```
2. **PlatformIO installieren** (VS Code empfohlen)
3. **Board auswählen**
   - In `platformio.ini` das passende Board setzen (`esp32dev` oder `esp32-c3-devkitm-1`)
4. **Firmware & Dateisystem hochladen**
   ```sh
   pio run --target uploadfs
   pio run --target upload
   ```
5. **Konfiguration im Web-Browser**
   - Nach dem ersten Start (oder wenn keine WLAN-Verbindung möglich ist) öffnet das Gerät ein WLAN namens `Keypad-Config` (Captive Portal).
   - Mit Smartphone/PC verbinden und im Browser `http://192.168.4.1` aufrufen.
   - Die Konfiguration (WLAN, BLE-Name, Buttons, **Bluetooth-LED**) kann komfortabel im Web-Formular angepasst werden.
   - Nach Speichern und Neustart verbindet sich das Gerät mit dem eingestellten WLAN. Die Konfiguration ist dann auch über die zugewiesene IP im Heimnetz erreichbar (siehe Serieller Monitor).
   - **Tipp:** Die aktuelle IP und Debug-Infos werden im seriellen Monitor ausgegeben.
   - **Bluetooth-LED:** Die LED zeigt den Status der Bluetooth-Verbindung an. Pin und Logik (invertiert/nicht invertiert) sind in der Konfiguration einstellbar. Die LED blinkt langsam bei Wartezustand, schnell nach Verbindungsabbruch und leuchtet dauerhaft bei aktiver Verbindung.
6. **Mit MYWhoosh koppeln**
   - Das Keypad erscheint als BLE-Tastatur mit integrierter Abs-Mouse (Name konfigurierbar)


## Web-Konfiguration & config.json

Die Konfiguration erfolgt komfortabel über das Web-Interface (siehe oben). Alternativ kann die Datei `data/config.json` direkt bearbeitet werden.

Beispiel für `config.json`:
```json
{
   "ble_name": "Mywhoosh_Keypad",
   "wifi_ssid": "DEIN_WLAN",
   "wifi_pass": "DEIN_PASSWORT",
   "doubleClickTime": 400,
   "longPressTime": 800,
   "battery_enabled": false,
   "battery_pin": -1,
   "debug_ble": false,
   "mouse_actions": [
      { "name": "MouseMove1", "x": 2000, "y": 5000 },
      { "name": "MouseMove2", "x": 5000, "y": 2000 }
   ],
   "buttons": [
      { "pin": 4, "key_normal": "I", "key_double": "1", "key_long": "5", "mode": "pullup", "debounce": 100 },
      { "pin": 5, "key_normal": "K", "key_double": "2", "key_long": "6", "mode": "pullup", "debounce": 100 }
   ]
}
```
- **ble_name**: Anzeigename im BLE
- **wifi_ssid**: WLAN-Name für automatische Verbindung
- **wifi_pass**: WLAN-Passwort
- **doubleClickTime**: Zeitfenster für Doppelklick (ms, global)
- **longPressTime**: Zeit für Langklick (ms, global)
- **battery_enabled**: Battery-Monitoring aktivieren (true/false)
- **battery_pin**: ADC-Pin fuer Batteriespannung (-1 deaktiviert)
- **debug_ble**: Debug-Ausgabe im seriellen Monitor aktivieren (true/false)
- **buttons**: Liste der Tasten (GPIO, Keycodes, Modus, Entprellzeit)
- **mouse_actions**: Aktionen fuer die BLE-Abs-Mouse (absolute Koordinaten 0..10000)

**Konfiguration der BLE-Abs-Mouse**
Die Abs-Mouse Aktionen werden ueber `mouse_actions` definiert und in den Buttons mit `key_long`, `key_double` oder `key_normal` referenziert. Der Eintrag `name` muss exakt mit dem Button-Wert uebereinstimmen. 

Die Koordinaten `x` und `y` sind absolut und gehen von 0 bis 10000 (0,0 oben links; 10000,10000 unten rechts). Beispiel: `"key_long": "MouseMove1"` triggert die Aktion `{ "name": "MouseMove1", "x": 2000, "y": 5000 }`.

Um exakte Werte fuer `x` und `y` zu berechnen, kannst du die Bildschirmmasse in mm verwenden:

X = 10000 / Bildschirmbreite_mm * Punkt_Abstand_links_mm

Y = 10000 / Bildschirmhoehe_mm * Punkt_Abstand_oben_mm

Die Werte danach auf ganze Zahlen von 0 bis 10000 begrenzen.

## Tastenbelegung für MYWhoosh
- Die Keycodes entsprechen den Tastaturbefehlen, die MYWhoosh akzeptiert (z.B. I, K, D, A, ...)
- Jede Taste kann drei Aktionen auslösen (z.B. "key_normal": "I", "key_double": "J", "key_long": "L")

# Alle bekanten Tasten 
Hier ist eine zusammengefasste Liste aller aktuell in MyWhoosh funktionierenden Tastenkombinationen:

Kategorie |	Tasten | Funktion                        
Steuerung | ← → or A D |links / rechts auf Strecke steuern (Abbiegen derzeit nicht möglich)     
Emotes | 1 - 7 | Peace, Wave, Fist bump, Dap, Elbow flick, Toast, Thumbs up          
UI Toggle | U | Minimal UI Mode                  
Virtual Schalten |I / K | Gang hoch / runter                    

leider gibt es derzeit keine, mir bekannten Tasten, um Abzubiegen oder um einen U-Turn auszuführen

für die Cursortasten müssen folgende bezeichnungen anstatt eines Buchstabens oder einer Zahl verwendet werden: 
Pfeil nach oben: "KEY_UP_ARROW"
Pfeil nach unten: "KEY_DOWN_ARROW"
Pfeil nach links: "KEY_LEFT_ARROW"
Pfeil nach rechts: "KEY_RIGHT_ARROW"
*** alle weiteren Sondertasten entnehmen sie bitte der "BleKeybords.h" zwischen Zeile 36 und 120 aus de ESP32 BLE Keybord Library

## WLAN & Captive Portal

- Das Gerät verbindet sich nach Neustart automatisch mit dem in der Konfiguration hinterlegten WLAN.
- Ist keine Verbindung möglich, wird ein Access Point `Keypad-Config` geöffnet. Über das Web-Portal (http://192.168.4.1) kann die Konfiguration geändert werden.
- Nach erfolgreicher WLAN-Verbindung ist das Web-Portal unter der zugewiesenen IP im Heimnetz erreichbar (siehe Serieller Monitor).
- alternativ kann auch die config.json editiert und ins Filesystem hochgeladen werden. Eine verbindung ins heimische Wlan ist auch kein muss 

## Debugging

- Alle wichtigen Status- und Fehlerausgaben (WLAN, Webserver, HTTP-Requests) werden im seriellen Monitor (115200 Baud) ausgegeben.
- Bei Problemen bitte die Ausgaben dort prüfen.

## Lizenz
MIT License

---

**Hinweis:**
Dieses Projekt ist ein Community-Projekt und steht in keiner Verbindung zu MYWhoosh oder deren Entwicklern. Für Schäden oder Fehlfunktionen wird keine Haftung übernommen.

