// Minimal LED Matrix Firmware for the Huidu WF2 (ESP32-S3) HUB75 Control Card.
// No WiFi, no RTC/NTP, no web server, no button. Just: boot -> play
// audio-reactive GIF forever, switching between silence.gif and talking.gif
// based on a MAX4466 mic on GPIO1.

#if defined(WF1)
  #include "hd-wf1-esp32s2-config.h"
#elif defined(WF2)
  #include "hd-wf2-esp32s3-config.h"
#else
  #error "Please define either WF1 or WF2"
#endif

#include "debug.h"

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <AnimatedGIF.h>
#include "esp_partition.h"

// Generate these two files with bin2h.py (see project notes), then place
// them in src/ alongside this file:
//   python bin2h.py silence.gif silence_gif silence_gif.h
//   python bin2h.py talking.gif talking_gif talking_gif.h
#include "silence_gif.h"
#include "talking_gif.h"

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
#define PANEL_CHAIN 2       // You have 2x 64x32 modules chained

#if defined(WF1)
HUB75_I2S_CFG::i2s_pins _pins_x1 = {WF1_R1_PIN, WF1_G1_PIN, WF1_B1_PIN, WF1_R2_PIN, WF1_G2_PIN, WF1_B2_PIN, WF1_A_PIN, WF1_B_PIN, WF1_C_PIN, WF1_D_PIN, WF1_E_PIN, WF1_LAT_PIN, WF1_OE_PIN, WF1_CLK_PIN};
#else
HUB75_I2S_CFG::i2s_pins _pins_x1 = {WF2_X1_R1_PIN, WF2_X1_G1_PIN, WF2_X1_B1_PIN, WF2_X1_R2_PIN, WF2_X1_G2_PIN, WF2_X1_B2_PIN, WF2_A_PIN, WF2_B_PIN, WF2_C_PIN, WF2_D_PIN, WF2_X1_E_PIN, WF2_LAT_PIN, WF2_OE_PIN, WF2_CLK_PIN};
#endif

// Mic input. GPIO1 is the only free ADC1 pin on the WF2 - everything else
// (2-13) is claimed by the HUB75 bus. Wire MAX4466 OUT here, VCC->3V3, GND->GND.
#define MIC_PIN 1
#define AUDIOGIF_SAMPLE_WINDOW_MS 30
#define AUDIOGIF_CALIBRATION_MS   2000
#define AUDIOGIF_MARGIN_ON        60
#define AUDIOGIF_MARGIN_OFF       25
#define AUDIOGIF_STATE_DWELL_MS   400
#define AUDIOGIF_EMA_ALPHA        0.35f

MatrixPanel_I2S_DMA *dma_display = nullptr;

AnimatedGIF gif;
bool gifOpenOk = false;
int audioNoiseFloor = 0;
float audioSmoothedLevel = 0;
bool audioIsTalking = false;
bool audioCandidateState = false;
unsigned long audioCandidateSince = 0;
unsigned long lastStatusPrint = 0;

// ------------------------------------------------------------------
// GIF draw callback - called once per decoded scanline
// ------------------------------------------------------------------
void AudioGifDraw(GIFDRAW *pDraw) {
  uint8_t *s = pDraw->pPixels;
  uint16_t *usPalette = pDraw->pPalette;
  int y = pDraw->iY + pDraw->y;
  int iWidth = pDraw->iWidth;
  int matrixWidth = PANEL_RES_X * PANEL_CHAIN;
  if (iWidth > matrixWidth) iWidth = matrixWidth;

  if (pDraw->ucDisposalMethod == 2) {
    for (int x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent) s[x] = pDraw->ucBackground;
    }
    pDraw->ucHasTransparency = 0;
  }

  if (pDraw->ucHasTransparency) {
    uint8_t ucTransparent = pDraw->ucTransparent;
    for (int x = 0; x < iWidth; x++) {
      uint8_t c = s[x];
      if (c != ucTransparent) {
        dma_display->drawPixel(x, y, usPalette[c]);
      }
    }
  } else {
    for (int x = 0; x < iWidth; x++) {
      dma_display->drawPixel(x, y, usPalette[s[x]]);
    }
  }
}

// ------------------------------------------------------------------
// Audio sampling + gif switching logic
// ------------------------------------------------------------------
int audioGifSamplePeakToPeak() {
  unsigned long start = millis();
  int minVal = 4095, maxVal = 0;
  while (millis() - start < AUDIOGIF_SAMPLE_WINDOW_MS) {
    int v = analogRead(MIC_PIN);
    if (v < minVal) minVal = v;
    if (v > maxVal) maxVal = v;
  }
  return maxVal - minVal;
}

void audioGifCalibrateNoiseFloor() {
  Serial.println("Calibrating mic noise floor - stay quiet...");
  long total = 0;
  int samples = 0;
  unsigned long start = millis();
  while (millis() - start < AUDIOGIF_CALIBRATION_MS) {
    total += audioGifSamplePeakToPeak();
    samples++;
  }
  audioNoiseFloor = samples > 0 ? (total / samples) : 20;
  audioSmoothedLevel = audioNoiseFloor;
  Serial.printf("Noise floor: %d (from %d samples)\n", audioNoiseFloor, samples);
}

void audioGifOpenForState(bool talking) {
  gif.close();
  if (talking) {
    gifOpenOk = gif.open((uint8_t *)talking_gif, talking_gif_len, AudioGifDraw);
  } else {
    gifOpenOk = gif.open((uint8_t *)silence_gif, silence_gif_len, AudioGifDraw);
  }
  if (!gifOpenOk) {
    Serial.printf("Failed to open %s gif\n", talking ? "talking" : "silence");
  } else {
    Serial.printf("Playing %s gif\n", talking ? "talking" : "silence");
  }
}

void audioGifUpdateState() {
  int level = audioGifSamplePeakToPeak();
  audioSmoothedLevel = audioSmoothedLevel * (1.0f - AUDIOGIF_EMA_ALPHA) + level * AUDIOGIF_EMA_ALPHA;

  int onThresh = audioNoiseFloor + AUDIOGIF_MARGIN_ON;
  int offThresh = audioNoiseFloor + AUDIOGIF_MARGIN_OFF;

  bool rawTalking = audioIsTalking ? (audioSmoothedLevel > offThresh) : (audioSmoothedLevel > onThresh);

  if (rawTalking != audioCandidateState) {
    audioCandidateState = rawTalking;
    audioCandidateSince = millis();
  }

  if (audioCandidateState != audioIsTalking && (millis() - audioCandidateSince) >= AUDIOGIF_STATE_DWELL_MS) {
    audioIsTalking = audioCandidateState;
    Serial.printf("State change -> %s (level=%.1f)\n", audioIsTalking ? "TALKING" : "SILENCE", audioSmoothedLevel);
    audioGifOpenForState(audioIsTalking);
  }
}

void updateAudioReactiveGif() {
  audioGifUpdateState();

  if (gifOpenOk) {
    if (!gif.playFrame(true, NULL)) {
      gif.reset(); // loop the same gif again
    }
  } else {
    // Print a status line once a second, forever, so whenever you connect
    // the Serial Monitor you see the current state within ~1 second -
    // no need to catch the exact moment of boot.
    if (millis() - lastStatusPrint > 1000) {
      lastStatusPrint = millis();
      Serial.printf("[STATUS] gifOpenOk: %s | state: %s\n",
                    gifOpenOk ? "yes" : "no",
                    audioIsTalking ? "talking" : "silence");
    }
    delay(200);
    audioGifOpenForState(audioIsTalking);
  }
}

void setup() {
  Serial.begin(115200);

  for (int i = 5; i > 0; i--) {
    Serial.printf("Starting in %d...\n", i);
    delay(1000);
  }

  dumpPartitionTable();

  /*-------------------- START THE HUB75E DISPLAY --------------------*/
  HUB75_I2S_CFG mxconfig(
    PANEL_RES_X,
    PANEL_RES_Y,
    PANEL_CHAIN,
    _pins_x1
  );
  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_20M;
  mxconfig.latch_blanking = 4;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(128); //0-255
  dma_display->clearScreen();

  dma_display->fillScreenRGB888(255,0,0);
  delay(500);
  dma_display->fillScreenRGB888(0,255,0);
  delay(500);
  dma_display->fillScreenRGB888(0,0,255);
  delay(500);
  dma_display->clearScreen();

  /*-------------------- Audio-reactive GIF init --------------------*/
  analogReadResolution(12);       // 0-4095
  analogSetAttenuation(ADC_11db); // full 0-3.3V range for the mic swing
  pinMode(MIC_PIN, INPUT);
  audioGifCalibrateNoiseFloor();  // stay quiet during boot for a clean baseline

  gif.begin(GIF_PALETTE_RGB565_BE);
  audioGifOpenForState(false);    // start on the silence gif
}

void loop() {
  updateAudioReactiveGif();
}