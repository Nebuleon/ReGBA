#ifndef __DRAW_H__
#define __DRAW_H__

#include "port.h"

#define GCW0_SCREEN_WIDTH  320
#define GCW0_SCREEN_HEIGHT 240

extern SDL_Surface* GBAScreenSurface;
extern SDL_Surface* OutputSurface;

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
  scaled_aspect_bilinear,
  fullscreen_bilinear,
  scaled_aspect_subpixel,
  fullscreen_subpixel,
  unscaled,
  hardware
} video_scale_type;

enum HorizontalAlignment {
	LEFT,
	CENTER,
	RIGHT
};

enum VerticalAlignment {
	TOP,
	MIDDLE,
	BOTTOM
};

extern video_scale_type PerGameScaleMode;
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

extern void gba_render_half(uint16_t* Dest, uint16_t* Src, uint32_t DestX, uint32_t DestY,
	uint32_t SrcPitch, uint32_t DestPitch);

void PrintString(const char* String, uint16_t TextColor,
	void* Dest, uint32_t DestPitch, uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height,
	enum HorizontalAlignment HorizontalAlignment, enum VerticalAlignment VerticalAlignment);

void PrintStringOutline(const char* String, uint16_t TextColor, uint16_t OutlineColor,
	void* Dest, uint32_t DestPitch, uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height,
	enum HorizontalAlignment HorizontalAlignment, enum VerticalAlignment VerticalAlignment);

extern uint32_t GetRenderedWidth(const char* str);

extern uint32_t GetRenderedHeight(const char* str);

void ReGBA_VideoFlip();

void SetMenuResolution();

void SetGameResolution();

/*
 * Returns a new allocation containing a copy of the GBA screen. Its pixels
 * and lines are packed (the pitch is 480 bytes), and its pixel format is
 * XBGR 1555.
 * Output assertions:
 *   The returned pointer is non-NULL. If it is NULL, then this is a fatal
 *   error.
 */
extern uint16_t* copy_screen();

#define RGB888_TO_RGB565(r, g, b) ( \
  (((r) & 0xF8) << 8) | \
  (((g) & 0xFC) << 3) | \
  (((b) & 0xF8) >> 3) \
  )

#endif /* __DRAW_H__ */
