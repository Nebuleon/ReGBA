#ifndef __DRAW_H__
#define __DRAW_H__

#include "port.h"

extern SDL_Surface* GBAScreenSurface;

typedef enum
{
  unscaled,
  fullscreen,
  scaled_aspect,
} video_scale_type;

void init_video();

#endif /* __DRAW_H__ */
