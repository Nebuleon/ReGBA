#ifndef __DRAW_H__
#define __DRAW_H__

#include "port.h"

#define GCW0_SCREEN_WIDTH  320
#define GCW0_SCREEN_HEIGHT 240

extern SDL_Surface* GBAScreenSurface;
extern SDL_Surface* OutputSurface;

extern SDL_Rect ScreenRectangle;

extern uint_fast8_t AudioFrameskip;
extern uint_fast8_t AudioFrameskipControl;
extern uint_fast8_t UserFrameskipControl;

// Incremented by the video thread when it decides to skip a frame due to
// fast forwarding.
extern volatile unsigned int VideoFastForwarded;

// How many back buffers have had ApplyScaleMode called upon them?
// At 3, even a triple-buffered surface has had ApplyScaleMode called on
// all back buffers, so stopping remanence with ApplyScaleMode is wasteful.
extern uint_fast8_t FramesBordered;

typedef enum
{
  scaled_aspect,
  fullscreen,
  unscaled,
} video_scale_type;

extern video_scale_type ScaleMode;

void init_video();
extern bool ApplyBorder(const char* Filename);

extern void ApplyScaleMode(video_scale_type NewMode);

/*
 * Tells the drawing subsystem that it should start calling ApplyScaleMode
 * again to stop image remanence. This is called by the menu and progress
 * functions to indicate that their screens may have created artifacts around
 * the GBA screen.
 */
extern void ScaleModeUnapplied();

extern void print_string(const char *str, u16 fg_color,
 u32 x, u32 y);

extern void print_string_outline(const char *str, u16 fg_color, u16 border_color,
 u32 x, u32 y);

extern uint32_t GetRenderedWidth(const char* str);

extern uint32_t GetRenderedHeight(const char* str);

#define RGB888_TO_RGB565(r, g, b) ( \
  (((r) & 0xF8) << 8) | \
  (((g) & 0xFC) << 3) | \
  (((b) & 0xF8) >> 3) \
  )

#endif /* __DRAW_H__ */
