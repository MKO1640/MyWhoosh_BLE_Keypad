#ifndef WEBUI_H
#define WEBUI_H

// HTML Editor Seite mit Formular und dynamischer Button-Liste
const char* configEditorHTML = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Keypad Config Editor</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    
    body { 
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
      max-width: 900px; 
      margin: 0 auto;
      padding: 2em 1em;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
    }
    
    .container {
      background: #fff;
      border-radius: 12px;
      box-shadow: 0 10px 40px rgba(0,0,0,0.2);
      padding: 2em;
    }
    
    h2 { 
      color: #333;
      margin-bottom: 1.5em;
      font-size: 2em;
    }
    
    .form-group {
      margin-bottom: 1.5em;
    }
    
    label { 
      display: block; 
      margin-bottom: 0.5em;
      color: #555;
      font-weight: 600;
      font-size: 0.95em;
    }
    
    input[type="text"],
    input[type="password"],
    input[type="number"],
    select { 
      width: 100%;
      padding: 0.75em;
      border: 2px solid #e0e0e0;
      border-radius: 6px;
      font-size: 1em;
      transition: border-color 0.3s ease;
    }
    
    input[type="text"]:focus,
    input[type="password"]:focus,
    input[type="number"]:focus,
    select:focus { 
      outline: none;
      border-color: #667eea;
      box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
    }
    
    .checkbox-group {
      display: flex;
      align-items: center;
      gap: 0.75em;
    }
    
    input[type="checkbox"] {
      width: 1.2em;
      height: 1.2em;
      cursor: pointer;
    }
    
    .button-list { 
      margin: 2em 0;
      display: grid;
      gap: 1em;
    }
    
    .button-entry { 
      background: #f9f9f9;
      border: 2px solid #e0e0e0;
      padding: 1.5em;
      border-radius: 8px;
      position: relative;
      transition: border-color 0.3s ease, box-shadow 0.3s ease;
    }
    
    .button-entry:hover {
      border-color: #667eea;
      box-shadow: 0 4px 12px rgba(102, 126, 234, 0.1);
    }
    
    .button-entry-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 1em;
      padding-bottom: 1em;
      border-bottom: 1px solid #e0e0e0;
    }
    
    .button-entry-title {
      font-weight: 700;
      color: #333;
      font-size: 1.1em;
    }
    
    .button-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
      gap: 1em;
    }
    
    .remove-btn { 
      background: #e74c3c;
      color: #fff;
      border: none;
      padding: 0.5em 1em;
      border-radius: 4px;
      cursor: pointer;
      font-weight: 600;
      transition: background-color 0.3s ease;
    }
    
    .remove-btn:hover {
      background: #c0392b;
    }
    
    .btn-group {
      display: flex;
      gap: 1em;
      margin-top: 2em;
    }
    
    .add-btn { 
      background: #27ae60;
      color: #fff;
      border: none;
      padding: 0.75em 1.5em;
      border-radius: 6px;
      cursor: pointer;
      font-weight: 600;
      font-size: 1em;
      transition: background-color 0.3s ease;
    }
    
    .add-btn:hover {
      background: #229954;
    }
    
    .submit-btn {
      background: #667eea;
      color: #fff;
      border: none;
      padding: 0.75em 2em;
      border-radius: 6px;
      cursor: pointer;
      font-weight: 600;
      font-size: 1em;
      transition: background-color 0.3s ease;
    }
    
    .submit-btn:hover {
      background: #5568d3;
    }
    
    #msg { 
      margin-top: 1.5em;
      padding: 1em;
      border-radius: 6px;
      display: none;
    }
    
    #msg.success {
      background-color: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
      display: block;
    }
    
    #msg.error {
      background-color: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
      display: block;
    }
    
    h3 {
      color: #333;
      margin: 2em 0 1em 0;
      font-size: 1.3em;
    }
  </style>
</head>
<body>
<div class="container">
  <h2>⚙️ Keypad Konfiguration</h2>
  <form id="cfgform" onsubmit="event.preventDefault(); saveCfg();">
    <div class="form-group">
      <label for="ble_name">BLE Name</label>
      <input id="ble_name" name="ble_name" type="text">
    </div>
    
    <div class="form-group">
      <label for="wifi_ssid">WLAN SSID</label>
      <input id="wifi_ssid" name="wifi_ssid" type="text">
    </div>
    
    <div class="form-group">
      <label for="wifi_pass">WLAN Passwort</label>
      <input id="wifi_pass" name="wifi_pass" type="password">
    </div>
    
    <div class="form-group">
      <label for="doubleClickTime">Doppelklick-Zeit (ms)</label>
      <input id="doubleClickTime" name="doubleClickTime" type="number" min="100" max="2000">
    </div>
    
    <div class="form-group">
      <label for="longPressTime">Langklick-Zeit (ms)</label>
      <input id="longPressTime" name="longPressTime" type="number" min="100" max="5000">
    </div>
    
    <div class="form-group">
      <label for="ble_led_pin">BLE LED Pin</label>
      <input id="ble_led_pin" name="ble_led_pin" type="number">
    </div>
    
    <div class="form-group">
      <div class="checkbox-group">
        <input id="ble_led_invert" name="ble_led_invert" type="checkbox">
        <label for="ble_led_invert" style="margin-bottom: 0;">BLE LED invertieren</label>
      </div>
    </div>

    <h3>🔘 Buttons</h3>
    <div id="button-list" class="button-list"></div>
    
    <div class="btn-group">
      <button type="button" class="add-btn" onclick="addButton()">+ Button hinzufügen</button>
    </div>
    
    <br>
    <button type="submit" class="submit-btn">💾 Speichern</button>
  </form>
  <div id="msg"></div>
</div>

<script>
let config = {};
let buttonList = document.getElementById('button-list');

function renderButtons() {
  buttonList.innerHTML = '';
  config.buttons.forEach((btn, idx) => {
    let div = document.createElement('div');
    div.className = 'button-entry';
    div.innerHTML = `
      <div class="button-entry-header">
        <span class="button-entry-title">Button ${idx + 1}</span>
        <button type="button" class="remove-btn" onclick="removeButton(${idx})">Entfernen</button>
      </div>
      <div class="button-grid">
        <div class="form-group">
          <label>Pin:</label>
          <input type="number" value="${btn.pin}" onchange="updateButton(${idx}, 'pin', this.value)">
        </div>
        <div class="form-group">
          <label>Normal:</label>
          <input type="text" maxlength="1" value="${btn.key_normal || ''}" onchange="updateButton(${idx}, 'key_normal', this.value)">
        </div>
        <div class="form-group">
          <label>Doppel:</label>
          <input type="text" maxlength="1" value="${btn.key_double || ''}" onchange="updateButton(${idx}, 'key_double', this.value)">
        </div>
        <div class="form-group">
          <label>Lang:</label>
          <input type="text" maxlength="1" value="${btn.key_long || ''}" onchange="updateButton(${idx}, 'key_long', this.value)">
        </div>
        <div class="form-group">
          <label>Mode:</label>
          <select onchange="updateButton(${idx}, 'mode', this.value)">
            <option value="pullup" ${btn.mode == 'pullup' ? 'selected' : ''}>pullup</option>
            <option value="pulldown" ${btn.mode == 'pulldown' ? 'selected' : ''}>pulldown</option>
            <option value="input" ${btn.mode == 'input' ? 'selected' : ''}>input</option>
          </select>
        </div>
        <div class="form-group">
          <label>Debounce (ms):</label>
          <input type="number" value="${btn.debounce || 100}" onchange="updateButton(${idx}, 'debounce', this.value)">
        </div>
      </div>
    `;
    buttonList.appendChild(div);
  });
}

function updateButton(idx, key, value) {
  if (key == 'pin' || key == 'debounce') value = parseInt(value) || 0;
  config.buttons[idx][key] = value;
}

function addButton() {
  config.buttons.push({
    pin: 0,
    key_normal: '',
    key_double: '',
    key_long: '',
    mode: 'pullup',
    debounce: 100
  });
  renderButtons();
}

function removeButton(idx) {
  config.buttons.splice(idx, 1);
  renderButtons();
}

function fillForm() {
  document.getElementById('ble_name').value = config.ble_name || '';
  document.getElementById('wifi_ssid').value = config.wifi_ssid || '';
  document.getElementById('wifi_pass').value = config.wifi_pass || '';
  document.getElementById('doubleClickTime').value = config.doubleClickTime || 400;
  document.getElementById('longPressTime').value = config.longPressTime || 800;
  document.getElementById('ble_led_pin').value = config.ble_led_pin || '';
  document.getElementById('ble_led_invert').checked = !!config.ble_led_invert;
  renderButtons();
}

function saveCfg() {
  config.ble_name = document.getElementById('ble_name').value;
  config.wifi_ssid = document.getElementById('wifi_ssid').value;
  config.wifi_pass = document.getElementById('wifi_pass').value;
  config.doubleClickTime = parseInt(document.getElementById('doubleClickTime').value) || 400;
  config.longPressTime = parseInt(document.getElementById('longPressTime').value) || 800;
  config.ble_led_pin = parseInt(document.getElementById('ble_led_pin').value) || -1;
  config.ble_led_invert = document.getElementById('ble_led_invert').checked;
  
  fetch('/save', {
    method: 'POST',
    body: JSON.stringify(config)
  }).then(r => r.text()).then(t => {
    let msg = document.getElementById('msg');
    msg.innerText = t;
    msg.className = 'success';
    
    setTimeout(() => {
      if (confirm('Konfiguration gespeichert!\nSoll das Gerät jetzt neu gestartet werden?')) {
        fetch('/restart', {method: 'POST'});
      }
    }, 500);
  }).catch(err => {
    let msg = document.getElementById('msg');
    msg.innerText = 'Fehler beim Speichern: ' + err;
    msg.className = 'error';
  });
}

// Konfiguration beim Laden der Seite abrufen
fetch('/config.json')
  .then(r => r.json())
  .then(j => {
    config = j;
    if (!config.buttons) config.buttons = [];
    fillForm();
  })
  .catch(err => {
    console.error('Fehler beim Laden der Konfiguration:', err);
  });
</script>
</body>
</html>
)rawliteral";

#endif
