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
  html, body { height: 100%; }
  body {
    font-family: -apple-system, "Segoe UI", sans-serif;
    background: #0a0b0e radial-gradient(ellipse 900px 500px at 15% -10%, rgba(99,102,241,0.16), transparent),
                        radial-gradient(ellipse 700px 500px at 100% 10%, rgba(139,92,246,0.10), transparent);
    background-attachment: fixed;
    color: #eee;
    margin: 0;
    padding: 28px;
    position: relative;
    overflow-x: hidden;
    -webkit-font-smoothing: antialiased;
  }
  @media (max-width: 380px) {
    body { padding: 16px; }
  }

  /* Subtle wireframe grid that glows near the cursor/touch point */
  .bg-grid {
    position: fixed;
    inset: 0;
    pointer-events: none;
    z-index: 0;
    background-image:
      linear-gradient(rgba(140, 160, 255, 0.16) 1px, transparent 1px),
      linear-gradient(90deg, rgba(140, 160, 255, 0.16) 1px, transparent 1px);
    background-size: 26px 26px;
    -webkit-mask-image: radial-gradient(circle 240px at var(--mx, 50%) var(--my, 30%), black 0%, transparent 100%);
    mask-image: radial-gradient(circle 240px at var(--mx, 50%) var(--my, 30%), black 0%, transparent 100%);
    transition: mask-position 0.05s linear;
  }

  .container {
    max-width: 1400px;
    margin: 0 auto;
    position: relative;
    z-index: 1;
  }

  .header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 32px;
    flex-wrap: wrap;
    gap: 16px;
  }
  .brand { display: flex; align-items: center; gap: 14px; }
  .brand img { width: 46px; height: 46px; border-radius: 13px; box-shadow: 0 4px 18px rgba(99,102,241,0.35); }
  .brand h1 {
    font-size: 21px; margin: 0; font-weight: 800; letter-spacing: 0.3px;
    background: linear-gradient(135deg, #fff 20%, #a5b4fc 100%);
    -webkit-background-clip: text; background-clip: text; color: transparent;
  }
  .brand .sub { font-size: 12px; color: #7a7f8a; margin-top: 3px; letter-spacing: 0.4px; }

  .header-actions { display: flex; align-items: center; gap: 10px; }
  .icon-btn {
    width: 46px; height: 46px; border-radius: 14px; border: 1px solid rgba(255,255,255,0.07);
    background: rgba(22,24,29,0.75); backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px);
    color: #eee; font-size: 18px; cursor: pointer;
    display: flex; align-items: center; justify-content: center;
    transition: transform 0.15s ease, box-shadow 0.15s ease, background-color 0.15s ease, color 0.15s ease;
  }
  .icon-btn:hover { transform: translateY(-2px); box-shadow: 0 6px 16px rgba(0,0,0,0.35); }
  .icon-btn:active { background: #fff; color: #111; transition: none; transform: translateY(0); }
  .icon-btn.power-on {
    background: linear-gradient(135deg, #34d399, #10b981); color: #06210f; border-color: transparent;
    box-shadow: 0 4px 16px rgba(16,185,129,0.4);
  }
  .icon-btn.power-on:hover { filter: brightness(1.08); }

  .dotgrid { display: grid; grid-template-columns: repeat(12, 6px); grid-auto-rows: 6px; gap: 6px; }
  .dotgrid span { width: 6px; height: 6px; border-radius: 50%; background: #4c5a8a; }
  .dotgrid span.dim { background: #23283a; }

  .section-title {
    display: flex; align-items: center; gap: 10px;
    font-size: 16px; font-weight: 700; color: #fff;
    margin: 30px 0 16px 0; letter-spacing: 0.2px;
  }
  .badge {
    background: linear-gradient(135deg, #6366f1, #8b5cf6); color: #fff; font-size: 12px; font-weight: 700;
    border-radius: 20px; padding: 3px 10px; box-shadow: 0 2px 10px rgba(99,102,241,0.45);
  }

  .cards { display: flex; gap: 12px; margin-bottom: 20px; flex-wrap: wrap; }
  .card {
    flex: 1; min-width: 160px; max-width: 320px;
    background: rgba(22,24,29,0.75); backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px);
    border-radius: 18px; padding: 16px 18px;
    cursor: pointer; border: 1px solid rgba(255,255,255,0.07);
    transition: transform 0.15s ease, box-shadow 0.15s ease, background-color 0.15s ease, border-color 0.15s ease;
  }
  .card:hover { transform: translateY(-2px); box-shadow: 0 8px 22px rgba(0,0,0,0.35); }
  .card:active { background: #fff; transition: none; transform: translateY(0); }
  .card:active .value, .card:active .label { color: #111; }
  .card.selected {
    background: linear-gradient(160deg, rgba(99,102,241,0.18), rgba(139,92,246,0.10));
    border-color: rgba(139,92,246,0.55);
    box-shadow: 0 0 0 1px rgba(139,92,246,0.25), 0 8px 24px rgba(99,102,241,0.2);
  }
  .card .label {
    font-size: 11px; letter-spacing: 1px; color: #8a8f99; text-transform: uppercase;
    display: flex; align-items: center; gap: 6px; margin-bottom: 5px;
  }
  .card.selected .label { color: #b3b8ff; }
  .card .value { font-size: 18px; font-weight: 700; color: #fff; }
  .card.selected .value { color: #fff; }

  .grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(120px, 1fr));
    gap: 12px;
  }
  .tile {
    aspect-ratio: 1;
    border-radius: 18px;
    border: 1px solid rgba(255,255,255,0.06);
    cursor: pointer;
    background: rgba(22,24,29,0.75);
    backdrop-filter: blur(10px); -webkit-backdrop-filter: blur(10px);
    color: #fff;
    font-size: 14px;
    font-weight: 600;
    display: flex;
    align-items: center;
    justify-content: center;
    text-align: center;
    padding: 10px;
    transition: transform 0.15s ease, box-shadow 0.15s ease, background-color 0.15s ease, color 0.15s ease, border-color 0.15s ease;
  }
  .tile:hover { transform: translateY(-3px); box-shadow: 0 10px 24px rgba(0,0,0,0.4); border-color: rgba(139,92,246,0.4); }
  .tile:active { background: #fff !important; color: #111 !important; transition: none; transform: translateY(0); }
  .tile.selected {
    background: linear-gradient(160deg, #fff, #e8e9ff);
    color: #111;
    border-color: transparent;
    box-shadow: 0 0 0 2px rgba(139,92,246,0.6), 0 10px 26px rgba(99,102,241,0.35);
  }
  .tile.selected:hover { transform: translateY(-3px); filter: brightness(0.97); }

  h2.legacy { margin-top: 34px; margin-bottom: 12px; font-size: 13px; color: #7a7f8a; text-transform: uppercase; letter-spacing: 1.2px; font-weight: 700; }
  input[type=range] { width: 100%; accent-color: #8b5cf6; }
  input[type=text], input[type=password] {
    width: 100%; box-sizing: border-box; padding: 13px 14px; border-radius: 12px;
    border: 1px solid rgba(255,255,255,0.08); margin-bottom: 10px;
    background: rgba(22,24,29,0.75); color: #eee; font-size: 14px;
    transition: border-color 0.15s ease;
  }
  input[type=text]:focus, input[type=password]:focus { outline: none; border-color: rgba(139,92,246,0.6); }
  .row { display: flex; align-items: center; gap: 12px; }
  #wifiInfo { font-size: 13px; color: #9298a3; margin-bottom: 14px; line-height: 1.7; }
  .actionbtn {
    border: none; border-radius: 12px; padding: 13px 20px; font-size: 14px;
    font-weight: 700; cursor: pointer; color: #fff;
    background: linear-gradient(135deg, #6366f1, #8b5cf6);
    box-shadow: 0 6px 18px rgba(99,102,241,0.35);
    transition: transform 0.15s ease, filter 0.15s ease;
  }
  .actionbtn:hover { transform: translateY(-2px); filter: brightness(1.08); }
  .actionbtn:active { transform: translateY(0); filter: brightness(0.92); }

  .backbtn {
    border: none; background: none; color: #8a8f99; font-size: 14px;
    cursor: pointer; margin-bottom: 18px; padding: 0;
    display: flex; align-items: center; gap: 6px;
    transition: color 0.15s ease;
  }
  .backbtn:hover { color: #fff; }

  #wifiScreen { display: none; }
  #mainScreen.dimmed { opacity: 0.35; pointer-events: none; }
</style>
</head>
<body>

  <div class="bg-grid" id="bgGrid"></div>

  <div class="container">
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
  </div>

<script>
function updateSpotlight(x, y) {
  document.documentElement.style.setProperty('--mx', x + 'px');
  document.documentElement.style.setProperty('--my', y + 'px');
}
document.addEventListener('mousemove', (e) => updateSpotlight(e.clientX, e.clientY));
document.addEventListener('touchmove', (e) => {
  if (e.touches.length > 0) updateSpotlight(e.touches[0].clientX, e.touches[0].clientY);
}, { passive: true });

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

  bool prevNormalColorMode = normalColorMode;
  normalColorMode = true; // show the pairing gif in its own real colors, not tinted
  bool pairingGifOk = gif.open((uint8_t *)pairingGif.data, pairingGif.len, GifDraw);

  unsigned long wifiStart = millis();
  int delayMs = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < WIFI_CONNECT_TIMEOUT_MS) {
    if (pairingGifOk) {
      // Keep the pairing animation looping for as long as we're waiting -
      // this is what makes the screen "stay on connecting" until we
      // actually succeed or time out, instead of just playing once.
      if (!gif.playFrame(false, &delayMs)) {
        gif.reset();
        delayMs = 0;
      }
      if (delayMs < 0 || delayMs > MAX_FRAME_DELAY_MS) delayMs = MAX_FRAME_DELAY_MS;
      if (delayMs > 0) delay(delayMs);
    } else {
      delay(300);
    }
    Serial.print(".");
  }

  if (pairingGifOk) {
    gif.close();
  }
  normalColorMode = prevNormalColorMode;

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