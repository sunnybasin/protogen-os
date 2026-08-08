// LED Matrix Firmware for the Huidu WF2 (ESP32-S3) HUB75 Control Card.
//
// This file just wires everything together and holds setup()/loop() -
// the actual logic lives in these files (all in src/ alongside this one):
//   config.h          - board pins, WiFi/AP settings, timing constants
//   gifs_data.h        - every embedded gif and the lists that reference them
//   colors.h           - tint color palette and the rainbow color-wheel helper
//   display_state.h    - shared runtime state (current gif/color/brightness/etc.)
//   gif_playback.h      - gif drawing/tinting logic and the apply*() helpers
//   web_interface.h     - the embedded control web page and its HTTP API
//
// Behavior summary:
//   - Click the button: cycle tint color (white -> red -> green -> blue ->
//     orange -> purple -> ... -> rainbow -> back to white)
//   - Double-click quickly: reset to the GIF's normal, untinted colors
//   - Hold the button (500ms+): cycle to the next GIF in gifList[]
//   - Web page, via your home WiFi: visit the IP printed at boot, or
//     http://protocontroller.local
//   - Web page, via the board's own WiFi: connect to AP_SSID (see
//     config.h) and visit http://192.168.4.1 - always works, home WiFi
//     or not. Most phones will auto-prompt to open it (captive portal).
//
// On boot: initializing gif -> pairing/connecting gif -> startup gif
// (in real colors) -> hands off to the normal button-controlled gifs.
//
// To add more GIFs, see the instructions at the top of gifs_data.h.

#if defined(WF1)
  #include "hd-wf1-esp32s2-config.h"
#elif defined(WF2)
  #include "hd-wf2-esp32s3-config.h"
#else
  #error "Please define either WF1 or WF2"
#endif

#include "debug.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include <Bounce2.h>
#include "esp_partition.h"

#include "config.h"
#include "gifs_data.h"
#include "colors.h"
#include "display_state.h"
#include "gif_playback.h"
#include "web_interface.h"

void setup() {
  Serial.begin(115200);
  delay(300);

  dumpPartitionTable();

  /*-------------------- START THE HUB75E DISPLAY --------------------*/
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN,
    _pins_x1
  );
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  mxconfig.latch_blanking = 8;
  mxconfig.clkphase = false;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8((uint8_t)currentBrightness);
  dma_display->clearScreen();

  /*-------------------- Button --------------------*/
  button.attach(BUTTON_PIN, INPUT); // USE EXTERNAL PULL-UP
  button.interval(5);
  button.setPressedState(LOW);

  /*-------------------- Boot-sequence gifs --------------------*/
  gif.begin(GIF_PALETTE_RGB565_BE);
  playBootGif(initGif, INIT_GIF_MAX_MS);
  playBootGif(pairingGif, PAIRING_GIF_MAX_MS);

  /*-------------------- WiFi (own AP + optional home network) + Web server --------------------*/
  prefs.begin("ledmatrix", false);
  String savedSsid = prefs.getString("ssid", "");
  String savedPass = prefs.getString("pass", "");
  String ssidToUse = savedSsid.length() > 0 ? savedSsid : String(default_wifi_ssid);
  String passToUse = savedSsid.length() > 0 ? savedPass : String(default_wifi_pass);

  WiFi.mode(WIFI_AP_STA); // own AP always on, AND try to join a home network too

  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print("Own WiFi network '" AP_SSID "' started. Connect and visit http://");
  Serial.println(WiFi.softAPIP());

  // Captive portal: redirect every DNS lookup made by a device connected to
  // our AP straight to our own IP. Combined with onNotFound() below serving
  // the control page for literally any path, this makes most phones/laptops
  // automatically pop up the control page (like the "Sign in to network"
  // prompt you see joining public WiFi) right after connecting.
  dnsServer.start(53, "*", WiFi.softAPIP());

  bool wifiConfigured = ssidToUse.length() > 0 && ssidToUse != "YOUR_WIFI_NAME_HERE";
  if (wifiConfigured) {
    connectToWifi(ssidToUse, passToUse);
  } else {
    Serial.println("No home WiFi configured - relying on the board's own AP network only.");
    Serial.println("(Set one any time from the web page's WiFi Settings section.)");
  }

  webServer.on("/", handleRoot);
  webServer.on("/api/gifs", handleApiGifs);
  webServer.on("/api/colors", handleApiColors);
  webServer.on("/api/status", handleApiStatus);
  webServer.on("/api/setGif", handleApiSetGif);
  webServer.on("/api/setColor", handleApiSetColor);
  webServer.on("/api/setNormal", handleApiSetNormal);
  webServer.on("/api/setBrightness", handleApiSetBrightness);
  webServer.on("/api/setPower", handleApiSetPower);
  webServer.on("/api/setWifi", handleApiSetWifi);
  webServer.onNotFound(handleRoot); // any unrecognized path (captive portal probes, typos, etc.) just shows the control page
  webServer.begin();
  Serial.println("Web server started.");

  /*-------------------- Startup gif + normal gif playback --------------------*/
  dma_display->clearScreen();
  playBootGif(startupGif, STARTUP_GIF_MAX_MS);
  openCurrentGif();
}

void loop() {
  dnsServer.processNextRequest(); // captive portal DNS redirect
  webServer.handleClient(); // AP is always up, so always service requests

  button.update();

  if (button.pressed()) {
    buttonPressStartTime = millis();
    buttonHoldHandled = false;
  }

  if (button.isPressed() && !buttonHoldHandled) {
    if (millis() - buttonPressStartTime >= BUTTON_HOLD_MS) {
      currentGifIndex = (currentGifIndex + 1) % NUM_GIFS;
      Serial.printf("Gif -> %d\n", currentGifIndex);
      openCurrentGif();
      buttonHoldHandled = true;
    }
  }

  if (button.released() && !buttonHoldHandled) {
    unsigned long now = millis();
    bool isDoubleClick = lastClickReleaseTime != 0 &&
                          (now - lastClickReleaseTime) < DOUBLE_CLICK_WINDOW_MS;

    if (isDoubleClick) {
      normalColorMode = true;
      lastClickReleaseTime = 0;
      Serial.println("Double-click -> normal colors");
    } else {
      normalColorMode = false;
      currentTintIndex = (currentTintIndex + 1) % NUM_TINT_MODES;
      lastClickReleaseTime = now;
      Serial.printf("Color mode -> %d\n", currentTintIndex);
    }
  }

  if (displayPoweredOn) {
    if (gifOpenOk) {
      if (millis() >= nextFrameTime) {
        int delayMs = 0;
        if (!gif.playFrame(false, &delayMs)) {
          gif.reset();
          delayMs = 0;
        }
        if (delayMs < 0 || delayMs > MAX_FRAME_DELAY_MS) {
          delayMs = MAX_FRAME_DELAY_MS;
        }
        nextFrameTime = millis() + delayMs;
      }
    } else {
      delay(200);
      openCurrentGif();
    }
  }
}