// config.h - board pins, WiFi/AP settings, panel geometry, and all the
// timing/behavior constants used across the rest of the firmware.
#pragma once

// Bump this whenever you reflash with meaningful changes - shown on the
// physical panel at boot and in the web page header, so you can always
// tell which build is actually running on the board.
#define FIRMWARE_VERSION "1.0.0"

/*----------------------------- WiFi Configuration -------------------------------*/
// These are just the FIRST-TIME defaults for joining your home network.
// Change them any time from the web page's WiFi Settings section instead -
// that saves to flash and reconnects immediately, no reflashing needed.
const char *default_wifi_ssid = "YOUR_WIFI_NAME_HERE";
const char *default_wifi_pass = "YOUR_WIFI_PASSWORD_HERE";
#define WIFI_CONNECT_TIMEOUT_MS 10000 // give up joining home WiFi after this long

// The board's own WiFi network - always broadcast, regardless of whether
// it also joins a home network. Connect to this directly and visit
// http://192.168.4.1 any time there's no home WiFi around.
#define AP_SSID "PROTO CONTROLLER"
#define AP_PASSWORD "protogen1234" // WPA2 requires at least 8 characters

Preferences prefs;
WebServer webServer(80);
DNSServer dnsServer;

void dumpPartitionTable() {
  Serial.println("---- Partition table on this chip ----");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  while (it != NULL) {
    const esp_partition_t *p = esp_partition_get(it);
    Serial.printf("  %-10s type=%d subtype=%d offset=0x%06x size=0x%06x (%u KB)\n",
                  p->label, p->type, p->subtype, p->address, p->size, p->size / 1024);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
  Serial.println("---------------------------------------");
}

/*-------------------------- HUB75E DMA Setup -----------------------------*/
#define PANEL_RES_X 64      // Number of pixels wide of each INDIVIDUAL panel module.
#define PANEL_RES_Y 32      // Number of pixels tall of each INDIVIDUAL panel module.
#define PANEL_CHAIN 2       // 2x 64x32 modules chained

#if defined(WF1)
HUB75_I2S_CFG::i2s_pins _pins_x1 = {WF1_R1_PIN, WF1_G1_PIN, WF1_B1_PIN, WF1_R2_PIN, WF1_G2_PIN, WF1_B2_PIN, WF1_A_PIN, WF1_B_PIN, WF1_C_PIN, WF1_D_PIN, WF1_E_PIN, WF1_LAT_PIN, WF1_OE_PIN, WF1_CLK_PIN};
#else
HUB75_I2S_CFG::i2s_pins _pins_x1 = {WF2_X1_R1_PIN, WF2_X1_G1_PIN, WF2_X1_B1_PIN, WF2_X1_R2_PIN, WF2_X1_G2_PIN, WF2_X1_B2_PIN, WF2_A_PIN, WF2_B_PIN, WF2_C_PIN, WF2_D_PIN, WF2_X1_E_PIN, WF2_LAT_PIN, WF2_OE_PIN, WF2_CLK_PIN};
#endif

// WF2's built-in push button
#define BUTTON_PIN 17
#define BUTTON_HOLD_MS 500 // hold longer than this to cycle gifs instead of color

// Two clicks whose *releases* land within this window of each other count
// as a "double-click", which resets the display to the GIF's normal,
// untinted colors instead of advancing to the next tint.
#define DOUBLE_CLICK_WINDOW_MS 400

// Upper bound on how long we'll ever wait between GIF frames. Some GIF
// files carry a per-frame delay value that is set very high (either a
// bad export, or a "hold on last frame" trick meant for viewers that
// loop differently than we do here). Capping it means a single bad frame
// can never stall playback (and button input) for multiple seconds.
#define MAX_FRAME_DELAY_MS 100

// Safety caps on the boot-sequence gifs (startup/init/pairing) - if any of
// them are a looping animation rather than a one-shot, we don't want them
// playing forever, so we bail out after this many milliseconds regardless
// (each one still finishes its current frame first, no mid-frame cutoff).
#define STARTUP_GIF_MAX_MS 4000
#define INIT_GIF_MAX_MS 1500
#define PAIRING_GIF_MAX_MS 2500

// How fast the gradient/rainbow color effects drift over time. Lower = faster.
#define RAINBOW_SPEED_DIVISOR 20
// How much to blend gradient colors toward white for the pastel color mode.
// 0.0 = no blending (full saturation), 1.0 = pure white.
#define PASTEL_BLEND_FACTOR 0.55f