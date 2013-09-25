#ifndef IMAGEIO_H
#define IMAGEIO_H

#include <stdint.h>
#include <SDL.h>

struct SDL_Surface;

/** Loads an image from a PNG file into a newly allocated 32bpp RGBA surface.
  */
SDL_Surface *loadPNG(const char* Path, uint32_t MaxWidth, uint32_t MaxHeight);

#endif
