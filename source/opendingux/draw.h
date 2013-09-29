#ifndef __DRAW_H__
#define __DRAW_H__

#include "port.h"

#define GCW0_SCREEN_WIDTH  320
#define GCW0_SCREEN_HEIGHT 240

extern SDL_Surface* GBAScreenSurface;
extern SDL_Surface* OutputSurface;

extern uint_fast8_t AudioFrameskip;
extern uint_fast8_t AudioFrameskipControl;

typedef enum
{
  fullscreen,
  unscaled,
  scaled_aspect,
} video_scale_type;

extern video_scale_type ScaleMode;

void init_video();
extern bool ApplyBorder(const char* Filename);

extern void ApplyScaleMode(video_scale_type NewMode);

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
