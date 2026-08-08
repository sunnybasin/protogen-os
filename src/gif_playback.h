// gif_playback.h - turns raw gif data into pixels on the panel, applies
// the currently selected tint, and provides the shared helpers
// (applyGif/applyColor/etc.) that both the physical button and the web
// API call into, so the two control paths can never drift out of sync.
#pragma once

// ------------------------------------------------------------------
// Recolor a grayscale/white GIF pixel using the currently selected tint
// mode, scaled by that pixel's original brightness. x is the pixel's
// column position, needed for the left-to-right gradient modes.
// ------------------------------------------------------------------
uint16_t tintedColor565(uint16_t rgb565, int x) {
  // Double-click (or the web page) resets us to the GIF's own colors.
  if (normalColorMode) {
    // gif.begin(GIF_PALETTE_RGB565_BE) hands back palette colors in
    // big-endian byte order, but drawPixel() expects a native
    // (little-endian, on ESP32) RGB565 value. Swap the bytes back
    // before returning it.
    return (uint16_t)((rgb565 >> 8) | (rgb565 << 8));
  }

  uint8_t r5 = (rgb565 >> 11) & 0x1F;
  float frac = r5 / 31.0f;

  const int matrixWidth = PANEL_RES_X * PANEL_CHAIN;
  TintColor tint;

  if (currentTintIndex == RAINBOW_INDEX) {
    uint8_t pos = (uint8_t)((millis() / RAINBOW_SPEED_DIVISOR) % 256);
    tint = wheelColor(pos);
  } else if (currentTintIndex == GRADIENT_RAINBOW_INDEX) {
    uint8_t pos = (uint8_t)(((x * 256 / matrixWidth) + (millis() / RAINBOW_SPEED_DIVISOR)) % 256);
    tint = wheelColor(pos);
  } else if (currentTintIndex == PASTEL_GRADIENT_INDEX) {
    uint8_t pos = (uint8_t)(((x * 256 / matrixWidth) + (millis() / RAINBOW_SPEED_DIVISOR)) % 256);
    TintColor full = wheelColor(pos);
    tint.r = (uint8_t)(full.r + (255 - full.r) * PASTEL_BLEND_FACTOR);
    tint.g = (uint8_t)(full.g + (255 - full.g) * PASTEL_BLEND_FACTOR);
    tint.b = (uint8_t)(full.b + (255 - full.b) * PASTEL_BLEND_FACTOR);
  } else {
    tint = tintColors[currentTintIndex];
  }

  uint8_t tr = (uint8_t)(tint.r * frac);
  uint8_t tg = (uint8_t)(tint.g * frac);
  uint8_t tb = (uint8_t)(tint.b * frac);
  return dma_display->color565(tr, tg, tb);
}

// ------------------------------------------------------------------
// GIF draw callback - called once per decoded scanline
// ------------------------------------------------------------------
void GifDraw(GIFDRAW *pDraw) {
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
        dma_display->drawPixel(x, y, tintedColor565(usPalette[c], x));
      }
    }
  } else {
    for (int x = 0; x < iWidth; x++) {
      dma_display->drawPixel(x, y, tintedColor565(usPalette[s[x]], x));
    }
  }
}

void openCurrentGif() {
  gif.close();
  GifEntry &entry = gifList[currentGifIndex];
  gifOpenOk = gif.open((uint8_t *)entry.data, entry.len, GifDraw);
  if (!gifOpenOk) {
    Serial.printf("Failed to open gif: %s\n", entry.name);
  } else {
    Serial.printf("Playing gif: %s\n", entry.name);
  }
  nextFrameTime = millis(); // draw the first frame of the new gif immediately
}

// ------------------------------------------------------------------
// Shared state-changing helpers - used by the button and the web API
// alike, so both control paths stay perfectly in sync.
// ------------------------------------------------------------------
void applyGif(int idx) {
  if (idx >= 0 && idx < NUM_GIFS) {
    currentGifIndex = idx;
    openCurrentGif();
  }
}

void applyColor(int idx) {
  if (idx >= 0 && idx < NUM_TINT_MODES) {
    currentTintIndex = idx;
    normalColorMode = false;
  }
}

void applyNormal() {
  normalColorMode = true;
}

void applyBrightness(int v) {
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  currentBrightness = v;
  dma_display->setBrightness8((uint8_t)currentBrightness);
}

void applyPower(bool on) {
  displayPoweredOn = on;
  if (!on) {
    dma_display->clearScreen();
  } else {
    openCurrentGif(); // force an immediate redraw instead of waiting for the next frame tick
  }
}

// ------------------------------------------------------------------
// Plays a boot-sequence gif (startup/initializing/pairing) once through,
// or until maxMs elapses, whichever comes first - in its own real colors
// rather than whatever tint happens to be selected. Blocking is fine
// here since these only run during setup(), before the button/loop
// logic is live.
// ------------------------------------------------------------------
void playBootGif(GifEntry &entry, unsigned long maxMs) {
  bool ok = gif.open((uint8_t *)entry.data, entry.len, GifDraw);
  if (!ok) {
    Serial.printf("Failed to open %s gif\n", entry.name);
    return;
  }
  Serial.printf("Playing %s gif\n", entry.name);

  bool prevNormalColorMode = normalColorMode;
  normalColorMode = true; // show it in its real colors, not tinted

  unsigned long start = millis();
  int delayMs = 0;
  while (millis() - start < maxMs) {
    int result = gif.playFrame(false, &delayMs);
    if (delayMs < 0 || delayMs > MAX_FRAME_DELAY_MS) delayMs = MAX_FRAME_DELAY_MS;
    if (delayMs > 0) delay(delayMs);
    if (!result) gif.reset(); // keep it on screen for the full duration even if it's a single frame
  }
  gif.close();

  normalColorMode = prevNormalColorMode;
}