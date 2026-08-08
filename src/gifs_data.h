// gifs_data.h - every embedded gif, and the lists/entries that reference
// them. To add a new gif to the button-cycled list:
//   1. Convert it with bin2h.py (see project notes):
//        python bin2h.py newone.gif newone_gif src/gifs/newone_gif.h
//   2. #include "gifs/newone_gif.h" below, alongside the existing ones.
//   3. Add a line to gifList[] further down:
//        { newone_gif, newone_gif_len, "newone" },
//   The web page's gif buttons update automatically - no other code
//   changes needed anywhere else in the firmware.
#pragma once

#include "gifs/startup_gif.h"
#include "gifs/initializing_gif.h"
#include "gifs/pairing_gif.h"
#include "gifs/silence_gif.h"
#include "gifs/talking_gif.h"
#include "gifs/amongus_gif.h"
#include "gifs/bsod_gif.h"
#include "gifs/cute_gif.h"
#include "gifs/shocked_gif.h"
#include "gifs/bk_gif.h"

struct GifEntry { const uint8_t *data; unsigned int len; const char *name; };

// These three play once during boot, each in their own real colors (not
// tinted) - see playBootGif() in gif_playback.h. None of them are part of
// gifList[] below, so none are reachable via the button or the web page.
GifEntry startupGif = { startup_gif, startup_gif_len, "startup" };
GifEntry initGif = { initializing_gif, initializing_gif_len, "initializing" };
GifEntry pairingGif = { pairing_gif, pairing_gif_len, "pairing" };

// The list of GIFs to cycle through on a button hold (or pick directly
// from the web page).
GifEntry gifList[] = {
  { silence_gif, silence_gif_len, "silence" },
  { cute_gif, cute_gif_len, "cute" },
  { amongus_gif, amongus_gif_len, "amongus" },
  { bsod_gif, bsod_gif_len, "bsod" },
  { bk_gif, bk_gif_len, "bj" },
  { shocked_gif, shocked_gif_len, "shocked" },
  { talking_gif, talking_gif_len, "talking" },
  // { newone_gif, newone_gif_len, "newone" },  <- example of adding another
};
const int NUM_GIFS = sizeof(gifList) / sizeof(gifList[0]);