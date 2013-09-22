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

uint16_t* GBAScreen;
uint32_t  GBAScreenPitch = GBA_SCREEN_WIDTH;

volatile uint_fast8_t VideoFastForwarded;

SDL_Surface *GBAScreenSurface = NULL;
SDL_Surface *OutputSurface = NULL;
SDL_Surface *BorderSurface = NULL;

video_scale_type ScaleMode = fullscreen;

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

bool ApplyBorder(const char* Filename)
{
	SDL_Surface* JustLoaded = loadPNG(Filename, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT);
	bool Result = false;
	if (JustLoaded != NULL)
	{
		if (JustLoaded->w == GCW0_SCREEN_WIDTH && JustLoaded->h == GCW0_SCREEN_HEIGHT)
		{
			if (BorderSurface != NULL)
			{
				SDL_FreeSurface(BorderSurface);
				BorderSurface = NULL;
			}
			BorderSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, 16,
			  OutputSurface->format->Rmask,
			  OutputSurface->format->Gmask,
			  OutputSurface->format->Bmask,
			  OutputSurface->format->Amask);
			SDL_BlitSurface(JustLoaded, NULL, BorderSurface, NULL);
			Result = true;
		}
		SDL_FreeSurface(JustLoaded);
		JustLoaded = NULL;
	}
	return Result;
}

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

static inline void gba_render(uint32_t* Dest, uint32_t* Src, uint32_t Width, uint32_t Height)
{
	Dest = (uint32_t *) ((uint16_t *) Dest
		+ (Height - GBA_SCREEN_HEIGHT) / 2 * Width
		+ (Width - GBA_SCREEN_WIDTH) / 2);
	uint32_t LineSkip = (Width - GBA_SCREEN_WIDTH) * sizeof(uint16_t) / sizeof(uint32_t);

	uint32_t X, Y;
	for (Y = 0; Y < GBA_SCREEN_HEIGHT; Y++)
	{
		for (X = 0; X < GBA_SCREEN_WIDTH * sizeof(uint16_t) / sizeof(uint32_t); X++)
		{
			*Dest++ = bgr555_to_rgb565(*Src++);
		}
		Dest += LineSkip;
	}
}

void ApplyScaleMode(video_scale_type NewMode)
{
	switch (NewMode)
	{
		case unscaled:
			// Either show the border
			if (BorderSurface != NULL)
			{
				SDL_BlitSurface(BorderSurface, NULL, OutputSurface, NULL);
			}
			// or clear the rest of the screen to prevent image remanence.
			else
			{
				memset(OutputSurface->pixels, 0, GCW0_SCREEN_WIDTH * GCW0_SCREEN_HEIGHT * sizeof(u16));
			}
			break;

		case scaled_aspect:
			// Unimplemented
			break;

		case fullscreen:
			break;
	}
	ScaleMode = NewMode;
}

void ReGBA_RenderScreen(void)
{
	int16_t Y = GetVerticalAxisValue();

	if (Y < -32700)
		ApplyScaleMode(fullscreen);
	else if (Y > 32700)
		ApplyScaleMode(unscaled);

	if (ReGBA_IsRenderingNextFrame())
	{
		Stats.RenderedFrames++;
		ApplyScaleMode(ScaleMode);
		if (ScaleMode == unscaled)
		{
			gba_render((uint32_t *) OutputSurface->pixels, (uint32_t *) GBAScreenSurface->pixels, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT);
		}
		else
		{
			uint32_t *src = (uint32_t *) GBAScreen;
			gba_upscale((uint32_t*) OutputSurface->pixels, src, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT);
		}
		ReGBA_DisplayFPS();

		SDL_Flip(OutputSurface);

		while (ReGBA_GetAudioSamplesAvailable() > AUDIO_OUTPUT_BUFFER_SIZE * 3 * OUTPUT_FREQUENCY_DIVISOR + (VideoFastForwarded - AudioFastForwarded) * ((int) (SOUND_FREQUENCY / 59.73)))
			usleep(1000);
	}

	int16_t X = GetHorizontalAxisValue();

	if (X > 0)
		FastForwardValue = (uint_fast8_t) (50 * (uint32_t) X / 32767);
	else
		FastForwardValue = 0;

	if (FastForwardControl >= 60)
	{
		FastForwardControl -= 60;
		VideoFastForwarded++;
	}
	FastForwardControl += FastForwardValue;
}

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

void print_string_ext(const char *str, u16 fg_color,
 u32 x, u32 y, void *_dest_ptr, u32 pitch)
{
  u16 *dest_ptr;
  uint_fast8_t current_char;
  u32 current_halfword;
  u32 glyph_offset, glyph_width;
  u32 glyph_row, glyph_column;
  u32 i = 0;
  u32 str_index = 0;
  u32 current_x = x;

  while((current_char = str[str_index++]) != '\0')
  {
    dest_ptr = (u16 *)_dest_ptr + (y * pitch) + current_x;
    if(current_char == '\n')
    {
      y += _font_height;
      current_x = x;
    }
    else
    {
      glyph_offset = current_char * _font_height;
	  glyph_width = _font_width[current_char];
      current_x += glyph_width;
      for(glyph_row = 0; glyph_row < _font_height; glyph_row++, glyph_offset++)
      {
		current_halfword = _font_bits[glyph_offset];
		for (glyph_column = 0; glyph_column < glyph_width; glyph_column++)
		{
			if ((current_halfword >> (15 - glyph_column)) & 0x01)
				*dest_ptr = fg_color;
			dest_ptr++;
		}
		  dest_ptr += pitch - glyph_width;
	  }
	}

    if(current_x >= GCW0_SCREEN_WIDTH)
      break;
  }
}

void print_string(const char *str, u16 fg_color,
 u32 x, u32 y)
{
  print_string_ext(str, fg_color, x, y, OutputSurface->pixels, GCW0_SCREEN_WIDTH);
}