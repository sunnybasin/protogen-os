// display_state.h - the shared runtime state that the button logic, the
// web API, and the gif-drawing code all read and write. Keeping these
// together in one place makes it easy to see everything that "currently
// selected X" actually means in one spot.
#pragma once

MatrixPanel_I2S_DMA *dma_display = nullptr;
int currentBrightness = 20; // 0-255, also settable from the web page
bool displayPoweredOn = true; // web page's on/off toggle

int currentTintIndex = 0;
// When true, tinting is bypassed entirely and the GIF's own palette
// colors are drawn as-is. Entered via a double-click (or the web page's
// "Normal Colors" button); cleared by the next single click / color pick.
bool normalColorMode = false;
unsigned long lastClickReleaseTime = 0;

int currentGifIndex = 0;

Bounce2::Button button = Bounce2::Button();
unsigned long buttonPressStartTime = 0;
bool buttonHoldHandled = false;

AnimatedGIF gif;
bool gifOpenOk = false;

// When the next frame is allowed to be drawn. Replaces blocking on
// AnimatedGIF's internal delay() so button polling never stalls.
unsigned long nextFrameTime = 0;