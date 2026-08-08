// colors.h - the tint color palette selectable via the button/web page,
// plus the special animated modes (rainbow, gradient rainbow, pastel
// gradient) that sit after the static colors in the same index space.
#pragma once

struct TintColor { uint8_t r, g, b; };

TintColor tintColors[] = {
  {255, 255, 255}, // white
  {255, 0, 0},     // red
  {0, 255, 0},     // green
  {0, 0, 255},     // blue
  {255, 128, 0},   // orange
  {160, 32, 240},  // purple
  {0, 255, 255},   // cyan
  {255, 255, 0},   // yellow
  {255, 20, 147},  // hot pink
  {255, 214, 170}, // warm white
};
const char *tintNames[] = {
  "White", "Red", "Green", "Blue", "Orange", "Purple", "Cyan", "Yellow", "Hot Pink", "Warm White"
};
const int NUM_STATIC_COLORS = sizeof(tintColors) / sizeof(tintColors[0]);
const int RAINBOW_INDEX = NUM_STATIC_COLORS;                  // uniform animated rainbow
const int GRADIENT_RAINBOW_INDEX = NUM_STATIC_COLORS + 1;     // left-to-right rainbow gradient
const int PASTEL_GRADIENT_INDEX = NUM_STATIC_COLORS + 2;      // pastel version of the gradient
const int NUM_TINT_MODES = NUM_STATIC_COLORS + 3;

// Classic color-wheel helper: pos 0-255 sweeps through the full hue circle.
TintColor wheelColor(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return TintColor{ (uint8_t)(255 - pos * 3), 0, (uint8_t)(pos * 3) };
  } else if (pos < 170) {
    pos -= 85;
    return TintColor{ 0, (uint8_t)(pos * 3), (uint8_t)(255 - pos * 3) };
  } else {
    pos -= 170;
    return TintColor{ (uint8_t)(pos * 3), (uint8_t)(255 - pos * 3), 0 };
  }
}