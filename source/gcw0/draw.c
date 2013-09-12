/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"

#define GCW0_SCREEN_WIDTH  320
#define GCW0_SCREEN_HEIGHT 240

uint16_t* GBAScreen;
uint32_t  GBAScreenPitch = GBA_SCREEN_WIDTH;

volatile uint_fast8_t VideoFastForwarded;

SDL_Surface *GBAScreenSurface = NULL;
SDL_Surface *OutputSurface = NULL;

const u32 video_scale = 1;

void init_video()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		printf("Failed to initialize SDL !!\n");
		return;   // for debug
		// exit(1);
	}

	OutputSurface = SDL_SetVideoMode(GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	GBAScreenSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, 16,
	  GBA_RED_MASK,
	  GBA_GREEN_MASK,
	  GBA_BLUE_MASK,
	  0 /* alpha: none */);

	GBAScreen = (uint16_t*) GBAScreenSurface->pixels;

	SDL_ShowCursor(0);
}

#if 0
void video_resolution_large()
{
  if(current_scale != unscaled)
  {
    resolution_width = 320;
    resolution_height = 240;
  }
}

void video_resolution_small()
{
  if(current_scale != screen_scale)
  {
    resolution_width = small_resolution_width;
    resolution_height = small_resolution_height;
  }
}

void set_gba_resolution(video_scale_type scale)
{
  if(screen_scale != scale)
  {
    screen_scale = scale;
    switch(scale)
    {
      case unscaled:
      case scaled_aspect:
      case fullscreen:
        small_resolution_width = 240 * video_scale;
        small_resolution_height = 160 * video_scale;
        break;
    }
  }
}

void clear_screen(u16 color)
{
  u16 *dest_ptr = get_screen_pixels();
  u32 line_skip = get_screen_pitch() - screen->w;
  u32 x, y;

  for(y = 0; y < screen->h; y++)
  {
    for(x = 0; x < screen->w; x++, dest_ptr++)
    {
      *dest_ptr = color;
    }
    dest_ptr += line_skip;
  }
}
#endif

#define integer_scale_copy_2()                                                \
  current_scanline_ptr[x2] = current_pixel;                                   \
  current_scanline_ptr[x2 - 1] = current_pixel;                               \
  x2 -= 2                                                                     \

#define integer_scale_copy_3()                                                \
  current_scanline_ptr[x2] = current_pixel;                                   \
  current_scanline_ptr[x2 - 1] = current_pixel;                               \
  current_scanline_ptr[x2 - 2] = current_pixel;                               \
  x2 -= 3                                                                     \

#define integer_scale_copy_4()                                                \
  current_scanline_ptr[x2] = current_pixel;                                   \
  current_scanline_ptr[x2 - 1] = current_pixel;                               \
  current_scanline_ptr[x2 - 2] = current_pixel;                               \
  current_scanline_ptr[x2 - 3] = current_pixel;                               \
  x2 -= 4                                                                     \

#define integer_scale_horizontal(scale_factor)                                \
  for(y = 0; y < 160; y++)                                                    \
  {                                                                           \
    for(x = 239, x2 = (240 * video_scale) - 1; x >= 0; x--)                   \
    {                                                                         \
      current_pixel = current_scanline_ptr[x];                                \
      integer_scale_copy_##scale_factor();                                    \
      current_scanline_ptr[x2] = current_scanline_ptr[x];                     \
      current_scanline_ptr[x2 - 1] = current_scanline_ptr[x];                 \
      current_scanline_ptr[x2 - 2] = current_scanline_ptr[x];                 \
    }                                                                         \
    current_scanline_ptr += pitch;                                            \
  }                                                                           \

/***************************************************************************
 *   Scaler copyright (C) 2013 by Paul Cercueil                            *
 *   paul@crapouillou.net                                                  *
 ***************************************************************************/
static inline uint32_t bgr555_to_rgb565(uint32_t px)
{
	return ((px & 0x7c007c00) >> 10)
	  | ((px & 0x03e003e0) << 1)
	  | ((px & 0x001f001f) << 11);
}

/* Upscales an image by 33% in with and 50% in height; also does color
 * conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 3/2)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale(uint32_t *to, uint32_t *from,
	  uint32_t src_x, uint32_t src_y)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *
	 * After (parenthesis = average):
	 *    a      (a,b)      (b,c)      c      d      (d,e)      (e,f)      f
	 *    (a,g)  (a,b,g,h)  (b,c,h,i)  (c,i)  (d,j)  (d,e,j,k)  (e,f,k,l)  (f,l)
	 *    g      (g,h)      (h,i)      i      j      (j,k)      (k,l)      l
	 */

	const uint32_t dst_x = src_x * 4/3;

	uint32_t reg1, reg2, reg3, reg4, reg5;
	size_t x, y;


	for (y=0; y<src_y/2; y++) {
		for (x=0; x<src_x/6; x++) {
			prefetch(to+4, 1);

			/* Read b-a */
			reg1 = bgr555_to_rgb565(*from);
			reg2 = ((reg1 & 0xf7de0000) >> 1) + (reg1 & 0x08210000);

			/* Read h-g */
			reg3 = bgr555_to_rgb565(*(from++ + src_x/2));
			reg4 = ((reg3 & 0xf7de0000) >> 1) + (reg3 & 0x08210000);

			reg1 &= 0xffff;
			reg1 |= reg2 + ((reg1 & 0xf7de) << 15);

			/* Write (a,b)-a */
			*to = reg1;

			reg3 &= 0xffff;
			reg3 |= reg4 + ((reg3 & 0xf7de) << 15);

			/* Write (g,h)-g */
			*(to + 2*dst_x/2) = reg3;

			if (unlikely(reg1 != reg3))
			  reg1 = (reg1 & 0x08210821)
				+ ((reg1 & 0xf7def7de) >> 1)
				+ ((reg3 & 0xf7def7de) >> 1);

			/* Write (a,b,g,h)-(a,g) */
			*(to++ + dst_x/2) = reg1;

			/* Read d-c */
			reg1 = bgr555_to_rgb565(*from);
			reg2 = ((reg2 + ((reg1 & 0xf7de) << 15)) >> 16) | ((reg1 & 0xffff) << 16);

			/* Write c-(b,c) */
			*to = reg2;

			/* Read j-i */
			reg3 = bgr555_to_rgb565(*(from++ + src_x/2));
			reg4 = ((reg4 + ((reg3 & 0xf7de) << 15)) >> 16) | ((reg3 & 0xffff) << 16);

			/* Write i-(h,i) */
			*(to + 2*dst_x/2) = reg4;

			if (unlikely(reg2 != reg4))
			  reg2 = (reg2 & 0x08210821)
				+ ((reg2 & 0xf7def7de) >> 1)
				+ ((reg4 & 0xf7def7de) >> 1);

			/* Write (c,i)-(b,c,h,i) */
			*(to++ + dst_x/2) = reg2;

			/* Read f-e */
			reg2 = bgr555_to_rgb565(*from);
			reg4 = (reg2 & 0xf7def7de) >> 1;

			/* Write (d,e)-d */
			reg1 >>= 16;
			reg4 = reg1 | ((reg4 + ((reg1 & 0xf7de) >> 1) + (reg1 & 0x0821)) << 16);
			*to = reg4;

			/* Read l-k */
			reg1 = bgr555_to_rgb565(*(from++ + src_x/2));
			reg5 = (reg1 & 0xf7def7de) >> 1;

			/* Write (j,k)-j */
			reg3 >>= 16;
			reg5 = reg3 | ((reg5 + ((reg3 & 0xf7de) >> 1) + (reg3 & 0x0821)) << 16);
			*(to + 2*dst_x/2) = reg5;

			if (unlikely(reg4 != reg5))
			  reg4 = (reg4 & 0x08210821)
				+ ((reg4 & 0xf7def7de) >> 1)
				+ ((reg5 & 0xf7def7de) >> 1);

			/* Write (d,e,j,k)-(d,j) */
			*(to++ + dst_x/2) = reg4;

			/* Write f-(e,f) */
			reg3 = ((reg2 & 0xf7def7de) >> 1);
			reg2 = (reg2 & 0xffff0000) | ((reg3 + (reg3 >> 16) + (reg2 & 0x0821)) & 0xffff);
			*to = reg2;

			/* Write l-(k,l) */
			reg3 = ((reg1 & 0xf7def7de) >> 1);
			reg1 = (reg1 & 0xffff0000) | ((reg3 + (reg3 >> 16) + (reg1 & 0x0821)) & 0xffff);
			*(to + 2*dst_x/2) = reg1;

			if (unlikely(reg1 != reg2))
			  reg1 = (reg1 & 0x08210821)
				+ ((reg1 & 0xf7def7de) >> 1)
				+ ((reg2 & 0xf7def7de) >> 1);

			/* Write (f,l)-(e,f,k,l) */
			*(to++ + dst_x/2) = reg1;
		}

		to += 2*dst_x/2;
		from += src_x/2;
	}
}

void ReGBA_RenderScreen(void)
{
	int16_t X = GetHorizontalAxisValue();

	if (X > 0)
		FastForwardValue = (uint_fast8_t) (50 * (uint32_t) X / 32767);
	else
		FastForwardValue = 0;

	FastForwardControl += FastForwardValue;
	if (FastForwardControl >= 60)
	{
		FastForwardControl -= 60;
		VideoFastForwarded++;
	}
	else
	{
		/* Unscaled
		SDL_Rect rect = {
			(GCW0_SCREEN_WIDTH - GBA_SCREEN_WIDTH) / 2,
			(GCW0_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2,
			GBA_SCREEN_WIDTH,
			GBA_SCREEN_HEIGHT
		};
		SDL_BlitSurface(GBAScreenSurface, NULL, OutputSurface, &rect);
		*/
		uint32_t *src = (uint32_t *) GBAScreen;
		gba_upscale((uint32_t*) OutputSurface->pixels, src, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT);

		SDL_Flip(OutputSurface);

		while (ReGBA_GetAudioSamplesAvailable() > AUDIO_OUTPUT_BUFFER_SIZE * 3 + (VideoFastForwarded - AudioFastForwarded) * ((int) (SOUND_FREQUENCY / 59.73)))
			usleep(1000);
	}
}

#if 0
void update_display(void)
{
	if (!screen_scale)
		SDL_BlitSurface(screen,NULL,display,NULL);
	else
	{
		uint32_t *src = (uint32_t *)screen->pixels + 20 + 80 * (320 - 240);
		gba_upscale((uint32_t*)display->pixels, src, 240, 160, 320 - 240);
	}

	SDL_Flip(display);
}

void flip_screen()
{
  if (!screen)
	  return;
	
  if((video_scale != 1) && (current_scale != unscaled))
  {
    s32 x, y;
    s32 x2, y2;
    u16 *screen_ptr = get_screen_pixels();
    u16 *current_scanline_ptr = screen_ptr;
    u32 pitch = get_screen_pitch();
    u16 current_pixel;
    u32 i;

    switch(video_scale)
    {
      case 2:
        integer_scale_horizontal(2);
        break;

      case 3:
        integer_scale_horizontal(3);
        break;

      default:
      case 4:
        integer_scale_horizontal(4);
        break;

    }

    for(y = 159, y2 = (160 * video_scale) - 1; y >= 0; y--)
    {
      for(i = 0; i < video_scale; i++)
      {
        memcpy(screen_ptr + (y2 * pitch),
         screen_ptr + (y * pitch), 480 * video_scale);
        y2--;
      }
    }
  }
  update_normal();
}
#endif

u32 frame_to_render;

#if 0
void update_screen()
{
  if (!GBAScreenSurface)
		return;
  if(!skip_next_frame)
	  update_display();
}
#endif

video_scale_type screen_scale = fullscreen;
video_scale_type current_scale = fullscreen;
#if 0
video_filter_type screen_filter = filter_bilinear;
#endif

u16 *copy_screen()
{
	u32 pitch = GBAScreenPitch;
	u16 *copy = malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(uint16_t));
	u16 *dest_ptr = copy;
	u16 *src_ptr = GBAScreen;
	u32 line_skip = pitch - GBA_SCREEN_WIDTH;
	u32 x, y;

	for(y = 0; y < GBA_SCREEN_HEIGHT; y++)
	{
		for(x = 0; x < GBA_SCREEN_WIDTH; x++, src_ptr++, dest_ptr++)
		{
			*dest_ptr = *src_ptr;
		}
		src_ptr += line_skip;
	}

	return copy;
}

void blit_to_screen(u16 *src, u32 w, u32 h, u32 dest_x, u32 dest_y)
{	
  u32 pitch = GBAScreenPitch;
  u16 *dest_ptr = GBAScreen;
  u16 *src_ptr = src;
  u32 line_skip = pitch - w;
  u32 x, y;

  for(y = 0; y < h; y++)
  {
    for(x = 0; x < w; x++, src_ptr++, dest_ptr++)
    {
      *dest_ptr = *src_ptr;
    }
    dest_ptr += line_skip;
  }
}

#if 0
void print_string_ext(const char *str, u16 fg_color, u16 bg_color,
 u32 x, u32 y, void *_dest_ptr, u32 pitch, u32 pad)
{
  u16 *dest_ptr = (u16 *)_dest_ptr + (y * pitch) + x;
  u8 current_char = str[0];
  u32 current_row;
  u32 glyph_offset;
  u32 i = 0, i2, i3;
  u32 str_index = 1;
  u32 current_x = x;

  while(current_char)
  {
    if(current_char == '\n')
    {
      y += FONT_HEIGHT;
      current_x = x;
      dest_ptr = get_screen_pixels() + (y * pitch) + x;
    }
    else
    {
      glyph_offset = _font_offset[current_char];
      current_x += FONT_WIDTH;
      for(i2 = 0; i2 < FONT_HEIGHT; i2++, glyph_offset++)
      {
        current_row = _font_bits[glyph_offset];
        for(i3 = 0; i3 < FONT_WIDTH; i3++)
        {
          if((current_row >> (15 - i3)) & 0x01)
            *dest_ptr = fg_color;
          else
            *dest_ptr = bg_color;
          dest_ptr++;
        }
        dest_ptr += (pitch - FONT_WIDTH);
      }
      dest_ptr = dest_ptr - (pitch * FONT_HEIGHT) + FONT_WIDTH;
    }

    i++;

    current_char = str[str_index];

    if((i < pad) && (current_char == 0))
    {
      current_char = ' ';
    }
    else
    {
      str_index++;
    }

#ifdef ZAURUS
    if(current_x >= 320)
#else
    if(current_x >= 480)
#endif
      break;
  }
}

void print_string(const char *str, u16 fg_color, u16 bg_color,
 u32 x, u32 y)
{
  print_string_ext(str, fg_color, bg_color, x, y, get_screen_pixels(),
   get_screen_pitch(), 0);
}

void print_string_pad(const char *str, u16 fg_color, u16 bg_color,
 u32 x, u32 y, u32 pad)
{
  print_string_ext(str, fg_color, bg_color, x, y, get_screen_pixels(),
   get_screen_pitch(), pad);
}
#endif