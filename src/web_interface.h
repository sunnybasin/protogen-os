// web_interface.h - the embedded control web page (HTML/CSS/JS all in one
// string) plus the HTTP API handlers and WiFi connection logic behind it.
#pragma once

const char PAGE_HTML[] PROGMEM = R"HTMLPAGE(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Proto Controller</title>
<style>
 * { box-sizing: border-box; }
  body {
    font-family: -apple-system, "Segoe UI", sans-serif;
    background: #0c0d10;
    color: #eee;
    margin: 0;
    padding: 24px;
  }
  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 28px;
    flex-wrap: wrap;
    gap: 16px;
  }
  .brand { display: flex; align-items: center; gap: 14px; }
  .brand img { width: 44px; height: 44px; border-radius: 10px; }
  .brand h1 { font-size: 20px; margin: 0; font-weight: 700; letter-spacing: 0.5px; }
  .brand .sub { font-size: 12px; color: #7a7f8a; margin-top: 2px; letter-spacing: 0.5px; }

  .header-actions { display: flex; align-items: center; gap: 10px; }
  .icon-btn {
    width: 44px; height: 44px; border-radius: 12px; border: none;
    background: #16181d; color: #eee; font-size: 18px; cursor: pointer;
    display: flex; align-items: center; justify-content: center;
    transition: background-color 0.15s ease, color 0.15s ease, filter 0.15s ease;
  }
  .icon-btn:hover { filter: brightness(1.3); }
  .icon-btn:active { background: #fff; color: #111; transition: none; }
  .icon-btn.power-on { background: #2fbf71; color: #06210f; }
  .icon-btn.power-on:hover { filter: brightness(1.1); }

  .dotgrid { display: grid; grid-template-columns: repeat(12, 6px); grid-auto-rows: 6px; gap: 6px; }
  .dotgrid span { width: 6px; height: 6px; border-radius: 50%; background: #3a4a6b; }
  .dotgrid span.dim { background: #23283a; }

  .section-title {
    display: flex; align-items: center; gap: 10px;
    font-size: 15px; font-weight: 600; color: #fff;
    margin: 26px 0 14px 0;
  }
  .badge {
    background: #e6395c; color: #fff; font-size: 12px; font-weight: 700;
    border-radius: 20px; padding: 2px 9px;
  }

  .cards { display: flex; gap: 12px; margin-bottom: 18px; flex-wrap: wrap; }
  .card {
    flex: 1; min-width: 160px;
    background: #16181d; border-radius: 14px; padding: 14px 16px;
    cursor: pointer; border: 2px solid transparent;
    transition: background-color 0.15s ease, color 0.15s ease, filter 0.15s ease;
  }
  .card:hover { filter: brightness(1.25); }
  .card:active { background: #fff; transition: none; }
  .card:active .value, .card:active .label { color: #111; }
  .card.selected { background: #fff; }
  .card .label {
    font-size: 11px; letter-spacing: 1px; color: #8a8f99; text-transform: uppercase;
    display: flex; align-items: center; gap: 6px; margin-bottom: 4px;
  }
  .card.selected .label { color: #6b7280; }
  .card .value { font-size: 17px; font-weight: 700; color: #fff; }
  .card.selected .value { color: #111; }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
    gap: 10px;
  }
  .tile {
    aspect-ratio: 1;
    border-radius: 16px;
    border: none;
    cursor: pointer;
    background: #16181d;
    color: #fff;
    font-size: 14px;
    font-weight: 600;
    display: flex;
    align-items: center;
    justify-content: center;
    text-align: center;
    padding: 10px;
    transition: background-color 0.15s ease, color 0.15s ease, filter 0.15s ease;
  }
  .tile:hover { filter: brightness(0.8); }
  .tile.selected:hover { filter: brightness(0.92); }
  .tile:active { background: #fff !important; color: #111 !important; transition: none; }
  .tile.selected { background: #fff; color: #111; }

  h2.legacy { margin-top: 30px; margin-bottom: 10px; font-size: 13px; color: #7a7f8a; text-transform: uppercase; letter-spacing: 1px; }
  input[type=range] { width: 100%; }
  input[type=text], input[type=password] {
    width: 100%; box-sizing: border-box; padding: 12px; border-radius: 10px;
    border: none; margin-bottom: 8px; background: #16181d; color: #eee;
  }
  .row { display: flex; align-items: center; gap: 12px; }
  #wifiInfo { font-size: 13px; color: #7a7f8a; margin-bottom: 10px; line-height: 1.6; }
  .actionbtn {
    border: none; border-radius: 10px; padding: 12px 18px; font-size: 14px;
    font-weight: 600; cursor: pointer; color: #fff; background: #2d6cdf;
    transition: filter 0.15s ease;
  }
  .actionbtn:hover { filter: brightness(1.15); }
  .actionbtn:active { filter: brightness(0.9); }

  .backbtn {
    border: none; background: none; color: #8a8f99; font-size: 14px;
    cursor: pointer; margin-bottom: 16px; padding: 0;
    display: flex; align-items: center; gap: 6px;
  }
  .backbtn:hover { color: #fff; }

  #wifiScreen { display: none; }
  #mainScreen.dimmed { opacity: 0.35; pointer-events: none; }
</style>
</head>
<body>

  <div class="header">
    <div class="brand">
      <img src="data:image/png;base64,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" alt="logo">
      <div>
        <h1>PROTO CONTROLLER</h1>
        <div class="sub">Control panel &middot; v<span id="versionText">-</span></div>
      </div>
    </div>
    <div class="header-actions">
      <button class="icon-btn" id="powerBtn" onclick="togglePower()" title="Power">&#9211;</button>
      <button class="icon-btn" onclick="showWifiScreen()" title="WiFi Settings">&#9737;</button>
      <div class="dotgrid" id="dotgrid"></div>
    </div>
  </div>

  <div id="mainScreen">
    <div class="section-title">
      Expression <span class="badge" id="totalBadge">0</span>
    </div>

    <div class="cards">
      <div class="card selected" id="gifCard" onclick="selectCategory('gif')">
        <div class="label">GIF</div>
        <div class="value" id="gifCardValue">-</div>
      </div>
      <div class="card" id="colorCard" onclick="selectCategory('color')">
        <div class="label">COLOR</div>
        <div class="value" id="colorCardValue">-</div>
      </div>
    </div>

    <div class="grid" id="optionsGrid"></div>

    <h2 class="legacy">Brightness</h2>
    <div class="row">
      <input type="range" min="0" max="255" id="brightness" oninput="setBrightness(this.value)">
      <span id="brightnessVal">--</span>
    </div>
  </div>

  <div id="wifiScreen">
    <button class="backbtn" onclick="showMainScreen()">&#8592; Back</button>
    <div class="section-title">WiFi Settings</div>
    <div id="wifiInfo">Loading...</div>
    <input type="text" id="wifiSsid" placeholder="Home WiFi name">
    <input type="password" id="wifiPass" placeholder="Home WiFi password">
    <button class="actionbtn" onclick="saveWifi()">Save &amp; Connect</button>
  </div>

<script>
let state = {};
let gifNames = [];
let colorList = [];
let activeCategory = 'gif';

function buildDotGrid() {
  const el = document.getElementById('dotgrid');
  for (let row = 0; row < 3; row++) {
    for (let col = 0; col < 12; col++) {
      const d = document.createElement('span');
      if (row === 0 || row === 2) d.className = 'dim';
      el.appendChild(d);
    }
  }
}

function showWifiScreen() {
  document.getElementById('mainScreen').style.display = 'none';
  document.getElementById('wifiScreen').style.display = 'block';
}
function showMainScreen() {
  document.getElementById('wifiScreen').style.display = 'none';
  document.getElementById('mainScreen').style.display = 'block';
}

async function refreshStatus() {
  const res = await fetch('/api/status');
  state = await res.json();
  document.getElementById('brightness').value = state.brightness;
  document.getElementById('brightnessVal').innerText = state.brightness;
  document.getElementById('versionText').innerText = state.version;
  const homeStatus = state.staConnected ? ('connected, IP ' + state.staIp) : 'not connected';
  document.getElementById('wifiInfo').innerHTML =
    'Home WiFi: ' + homeStatus + '<br>Board\'s own network: ' + state.apSsid + ' (' + state.apIp + ')';

  document.getElementById('gifCardValue').innerText = gifNames[state.gif] || '-';
  document.getElementById('colorCardValue').innerText = state.normal ? 'Normal' : (colorList[state.color] ? colorList[state.color].name : '-');

  const powerBtn = document.getElementById('powerBtn');
  powerBtn.classList.toggle('power-on', state.powered);
  document.getElementById('mainScreen').classList.toggle('dimmed', !state.powered);

  renderGrid();
}

async function togglePower() {
  const newState = !state.powered;
  await fetch('/api/setPower?on=' + (newState ? 1 : 0));
  refreshStatus();
}

function selectCategory(cat) {
  activeCategory = cat;
  document.getElementById('gifCard').classList.toggle('selected', cat === 'gif');
  document.getElementById('colorCard').classList.toggle('selected', cat === 'color');
  renderGrid();
}

function renderGrid() {
  const grid = document.getElementById('optionsGrid');
  grid.innerHTML = '';

  if (activeCategory === 'gif') {
    gifNames.forEach((name, i) => {
      const b = document.createElement('button');
      b.className = 'tile' + (i === state.gif ? ' selected' : '');
      b.innerText = name;
      b.onclick = () => setGif(i);
      grid.appendChild(b);
    });
  } else {
    colorList.forEach((c, i) => {
      const b = document.createElement('button');
      const isSelected = !state.normal && i === state.color;
      b.className = 'tile' + (isSelected ? ' selected' : '');
      if (c.hex) {
        b.style.background = c.hex;
        const r = parseInt(c.hex.substr(1,2),16), g = parseInt(c.hex.substr(3,2),16), bl = parseInt(c.hex.substr(5,2),16);
        b.style.color = (r*299+g*587+bl*114)/1000 > 150 ? '#000' : '#fff';
      }
      b.innerText = c.name;
      b.onclick = () => setColor(i);
      grid.appendChild(b);
    });
    const b = document.createElement('button');
    b.className = 'tile' + (state.normal ? ' selected' : '');
    b.innerText = "Normal (gif's own)";
    b.onclick = () => setNormal();
    grid.appendChild(b);
  }
}

async function loadLists() {
  gifNames = await (await fetch('/api/gifs')).json();
  colorList = await (await fetch('/api/colors')).json();
  document.getElementById('totalBadge').innerText = gifNames.length + colorList.length;
}

async function setGif(i) { await fetch('/api/setGif?index=' + i); refreshStatus(); }
async function setColor(i) { await fetch('/api/setColor?index=' + i); refreshStatus(); }
async function setNormal() { await fetch('/api/setNormal'); refreshStatus(); }
async function setBrightness(v) {
  document.getElementById('brightnessVal').innerText = v;
  await fetch('/api/setBrightness?value=' + v);
}
async function saveWifi() {
  const ssid = document.getElementById('wifiSsid').value;
  const pass = document.getElementById('wifiPass').value;
  if (!ssid) { alert('Enter a WiFi name first'); return; }
  document.getElementById('wifiInfo').innerText = 'Saving and connecting... (this page may briefly disconnect)';
  await fetch('/api/setWifi?ssid=' + encodeURIComponent(ssid) + '&pass=' + encodeURIComponent(pass));
  setTimeout(refreshStatus, 4000);
}

buildDotGrid();
loadLists().then(refreshStatus);
setInterval(refreshStatus, 3000);
</script>
</body>
</html>
)HTMLPAGE";

void handleRoot() {
  webServer.send(200, "text/html", PAGE_HTML);
}

void handleApiGifs() {
  String json = "[";
  for (int i = 0; i < NUM_GIFS; i++) {
    if (i > 0) json += ",";
    json += "\"" + String(gifList[i].name) + "\"";
  }
  json += "]";
  webServer.send(200, "application/json", json);
}

void handleApiColors() {
  String json = "[";
  for (int i = 0; i < NUM_STATIC_COLORS; i++) {
    if (i > 0) json += ",";
    char hex[8];
    snprintf(hex, sizeof(hex), "#%02x%02x%02x", tintColors[i].r, tintColors[i].g, tintColors[i].b);
    json += "{\"name\":\"" + String(tintNames[i]) + "\",\"hex\":\"" + String(hex) + "\"}";
  }
  json += ",{\"name\":\"Rainbow\"}";
  json += ",{\"name\":\"Gradient Rainbow\"}";
  json += ",{\"name\":\"Pastel Gradient\"}";
  json += "]";
  webServer.send(200, "application/json", json);
}

void handleApiStatus() {
  String json = "{";
  json += "\"gif\":" + String(currentGifIndex) + ",";
  json += "\"color\":" + String(currentTintIndex) + ",";
  json += "\"normal\":" + String(normalColorMode ? "true" : "false") + ",";
  json += "\"brightness\":" + String(currentBrightness) + ",";
  json += "\"powered\":" + String(displayPoweredOn ? "true" : "false") + ",";
  json += "\"staConnected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
  json += "\"staIp\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : String("-")) + "\",";
  json += "\"apSsid\":\"" AP_SSID "\",";
  json += "\"apIp\":\"" + WiFi.softAPIP().toString() + "\",";
  json += "\"version\":\"" FIRMWARE_VERSION "\"";
  json += "}";
  webServer.send(200, "application/json", json);
}

void handleApiSetGif() {
  if (webServer.hasArg("index")) {
    applyGif(webServer.arg("index").toInt());
  }
  webServer.send(200, "text/plain", "OK");
}

void handleApiSetColor() {
  if (webServer.hasArg("index")) {
    applyColor(webServer.arg("index").toInt());
  }
  webServer.send(200, "text/plain", "OK");
}

void handleApiSetNormal() {
  applyNormal();
  webServer.send(200, "text/plain", "OK");
}

void handleApiSetBrightness() {
  if (webServer.hasArg("value")) {
    applyBrightness(webServer.arg("value").toInt());
  }
  webServer.send(200, "text/plain", "OK");
}

void handleApiSetPower() {
  if (webServer.hasArg("on")) {
    applyPower(webServer.arg("on").toInt() != 0);
  }
  webServer.send(200, "text/plain", "OK");
}

// ------------------------------------------------------------------
// WiFi connect helper - joins your home network if configured. The
// board's own AP_SSID network (set up separately in setup()) stays up
// regardless of whether this succeeds, so the web page is always
// reachable at least one way or the other.
// ------------------------------------------------------------------
void connectToWifi(const String &ssid, const String &pass) {
  Serial.printf("Connecting to home WiFi '%s'", ssid.c_str());
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_CONNECT_TIMEOUT_MS) {
    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Home WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
    if (MDNS.begin("protocontroller")) {
      Serial.println("mDNS responder started: http://protocontroller.local");
    }
  } else {
    Serial.println();
    Serial.println("Home WiFi connect failed/timed out - the board's own AP network still works.");
  }
}

void handleApiSetWifi() {
  if (!webServer.hasArg("ssid")) {
    webServer.send(400, "text/plain", "Missing ssid");
    return;
  }
  String newSsid = webServer.arg("ssid");
  String newPass = webServer.hasArg("pass") ? webServer.arg("pass") : "";
  prefs.putString("ssid", newSsid);
  prefs.putString("pass", newPass);
  webServer.send(200, "text/plain", "OK");
  connectToWifi(newSsid, newPass);
}