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

struct StringCut {
	uint32_t Start;  // Starting character index of the cut, inclusive.
	uint32_t End;    // Ending character index of the cut, exclusive.
};

uint16_t* GBAScreen;
uint32_t  GBAScreenPitch = GBA_SCREEN_WIDTH;

volatile unsigned int VideoFastForwarded;
uint_fast8_t AudioFrameskip = 0;
uint_fast8_t AudioFrameskipControl = 0;
uint_fast8_t SufficientAudioControl = 0;
uint_fast8_t UserFrameskipControl = 0;

uint_fast8_t FramesBordered = 0;

SDL_Surface *GBAScreenSurface = NULL;
SDL_Surface *OutputSurface = NULL;
SDL_Surface *BorderSurface = NULL;

video_scale_type PerGameScaleMode = 0;
video_scale_type ScaleMode = scaled_aspect;

#define COLOR_PROGRESS_BACKGROUND   RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_PROGRESS_TEXT_CONTENT RGB888_TO_RGB565(255, 255, 255)
#define COLOR_PROGRESS_TEXT_OUTLINE RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_PROGRESS_CONTENT      RGB888_TO_RGB565(  0, 128, 255)
#define COLOR_PROGRESS_OUTLINE      RGB888_TO_RGB565(255, 255, 255)

#define PROGRESS_WIDTH 240
#define PROGRESS_HEIGHT 18

static bool InFileAction = false;
static enum ReGBA_FileAction CurrentFileAction;
static struct timespec LastProgressUpdate;

void init_video()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		printf("Failed to initialize SDL !!\n");
		return;   // for debug
		// exit(1);
	}

	SDL_ShowCursor(SDL_DISABLE);
	OutputSurface = SDL_SetVideoMode(GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, 16, SDL_HWSURFACE |
#ifdef SDL_TRIPLEBUF
		SDL_TRIPLEBUF
#else
		SDL_DOUBLEBUF
#endif
		);
	GBAScreenSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, 16,
	  GBA_RED_MASK,
	  GBA_GREEN_MASK,
	  GBA_BLUE_MASK,
	  0 /* alpha: none */);

	GBAScreen = (uint16_t*) GBAScreenSurface->pixels;
}

void SetMenuResolution()
{
#ifdef GCW_ZERO
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_UnlockSurface(OutputSurface);
	OutputSurface = SDL_SetVideoMode(GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_LockSurface(OutputSurface);
#endif
}

void SetGameResolution()
{
#ifdef GCW_ZERO
	video_scale_type ResolvedScaleMode = ResolveSetting(ScaleMode, PerGameScaleMode);
	unsigned int Width = GBA_SCREEN_WIDTH, Height = GBA_SCREEN_HEIGHT;
	if (ResolvedScaleMode != hardware)
	{
		Width = GCW0_SCREEN_WIDTH;
		Height = GCW0_SCREEN_HEIGHT;
	}
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_UnlockSurface(OutputSurface);
	OutputSurface = SDL_SetVideoMode(Width, Height, 16, SDL_HWSURFACE |
#ifdef SDL_TRIPLEBUF
		SDL_TRIPLEBUF
#else
		SDL_DOUBLEBUF
#endif
		);
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_LockSurface(OutputSurface);
#endif
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

/***************************************************************************
 *   16-bit I/O version used by the sub-pixel and bilinear scalers         *
 *   (C) 2013 kuwanger                                                     *
 ***************************************************************************/
static inline uint16_t bgr555_to_rgb565_16(uint16_t px)
{
	return ((px & 0x7c00) >> 10)
	  | ((px & 0x03e0) << 1)
	  | ((px & 0x001f) << 11);
}

// Explaining the magic constants:
// F7DEh is the mask to remove the lower bit of all color
// components before dividing them by 2. Otherwise, the lower bit
// would bleed into the high bit of the next component.

// RRRRR GGGGGG BBBBB        RRRRR GGGGGG BBBBB
// 11110 111110 11110 [>> 1] 01111 011111 01111

// 0821h is the mask to gather the low bits again for averaging
// after discarding them.

// RRRRR GGGGGG BBBBB       RRRRR GGGGGG BBBBB
// 00001 000001 00001 [+ X] 00010 000010 00010

// E79Ch is the mask to remove the lower 2 bits of all color
// components before dividing them by 4. Otherwise, the lower bits
// would bleed into the high bits of the next component.

// RRRRR GGGGGG BBBBB        RRRRR GGGGGG BBBBB
// 11100 111100 11100 [>> 2] 00111 001111 00111

// 1863h is the mask to gather the low bits again for averaging
// after discarding them.

// RRRRR GGGGGG BBBBB       RRRRR GGGGGG BBBBB
// 00011 000011 00011 [+ X] 00110 000110 00110

/* Calculates the average of two RGB565 pixels. The source of the pixels is
 * the lower 16 bits of both parameters. The result is in the lower 16 bits.
 */
#define Average(A, B) ((((A) & 0xF7DE) >> 1) + (((B) & 0xF7DE) >> 1) + ((A) & (B) & 0x0821))

/* Calculates the average of two pairs of RGB565 pixels. The result is, in
 * the lower bits, the average of both lower pixels, and in the upper bits,
 * the average of both upper pixels. */
#define Average32(A, B) ((((A) & 0xF7DEF7DE) >> 1) + (((B) & 0xF7DEF7DE) >> 1) + ((A) & (B) & 0x08210821))

/* Raises a pixel from the lower half to the upper half of a pair. */
#define Raise(N) ((N) << 16)

/* Extracts the upper pixel of a pair into the lower pixel of a pair. */
#define Hi(N) ((N) >> 16)

/* Extracts the lower pixel of a pair. */
#define Lo(N) ((N) & 0xFFFF)

/* Calculates the average of two RGB565 pixels. The source of the pixels is
 * the lower 16 bits of both parameters. The result is in the lower 16 bits.
 * The average is weighted so that the first pixel contributes 3/4 of its
 * color and the second pixel contributes 1/4. */
#define AverageQuarters3_1(A, B) ( (((A) & 0xF7DE) >> 1) + (((A) & 0xE79C) >> 2) + (((B) & 0xE79C) >> 2) + ((( (( ((A) & 0x1863) + ((A) & 0x0821) ) << 1) + ((B) & 0x1863) ) >> 2) & 0x1863) )

#define RED_FROM_RGB565(rgb565) (((rgb565) >> 11) & 0x1F)
#define RED_TO_RGB565(r) (((r) & 0x1F) << 11)
#define GREEN_FROM_RGB565(rgb565) (((rgb565) >> 5) & 0x3F)
#define GREEN_TO_RGB565(g) (((g) & 0x3F) << 5)
#define BLUE_FROM_RGB565(rgb565) ((rgb565) & 0x1F)
#define BLUE_TO_RGB565(b) ((b) & 0x1F)

/*
 * Blends, with sub-pixel accuracy, 3/4 of the first argument with 1/4 of the
 * second argument. The pixel format of both arguments is RGB 565. The first
 * pixel is assumed to be to the left of the second pixel.
 */
static inline uint16_t SubpixelRGB3_1(uint16_t A, uint16_t B)
{
	return RED_TO_RGB565(RED_FROM_RGB565(A))
	     | GREEN_TO_RGB565(GREEN_FROM_RGB565(A) * 3 / 4 + GREEN_FROM_RGB565(B) * 1 / 4)
	     | BLUE_TO_RGB565(BLUE_FROM_RGB565(A) * 1 / 4 + BLUE_FROM_RGB565(B) * 3 / 4);
}

/*
 * Blends, with sub-pixel accuracy, 1/2 of the first argument with 1/2 of the
 * second argument. The pixel format of both arguments is RGB 565. The first
 * pixel is assumed to be to the left of the second pixel.
 */
static inline uint16_t SubpixelRGB1_1(uint16_t A, uint16_t B)
{
	return RED_TO_RGB565(RED_FROM_RGB565(A) * 3 / 4 + RED_FROM_RGB565(B) * 1 / 4)
	     | GREEN_TO_RGB565(GREEN_FROM_RGB565(A) * 1 / 2 + GREEN_FROM_RGB565(B) * 1 / 2)
	     | BLUE_TO_RGB565(BLUE_FROM_RGB565(A) * 1 / 4 + BLUE_FROM_RGB565(B) * 3 / 4);
}

/*
 * Blends, with sub-pixel accuracy, 1/4 of the first argument with 3/4 of the
 * second argument. The pixel format of both arguments is RGB 565. The first
 * pixel is assumed to be to the left of the second pixel.
 */
static inline uint16_t SubpixelRGB1_3(uint16_t A, uint16_t B)
{
	return RED_TO_RGB565(RED_FROM_RGB565(B) * 1 / 4 + RED_FROM_RGB565(A) * 3 / 4)
	     | GREEN_TO_RGB565(GREEN_FROM_RGB565(B) * 3 / 4 + GREEN_FROM_RGB565(A) * 1 / 4)
	     | BLUE_TO_RGB565(BLUE_FROM_RGB565(B));
}

/* Upscales an image by 33% in width and 50% in height; also does color
 * conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 3/2)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    g    gh   hi   i    j    jk   kl   l
	 */

	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 2) {
		for (x = 0; x < src_x / 6; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = bgr555_to_rgb565(*(uint32_t*) (from    )),
			         d_c = bgr555_to_rgb565(*(uint32_t*) (from + 2)),
			         f_e = bgr555_to_rgb565(*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				  Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				  Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				  Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				  Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch    )),
				         j_i = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
				         l_k = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					  Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					  Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					  Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					  Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				// -- Row 3 --
				// Generate gh_g from h_g.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(h_g) == Lo(h_g))
					? h_g
					: Lo(h_g) /* 'g' verbatim to low pixel */ |
					  Raise(Average(Hi(h_g), Lo(h_g))) /* gh to high pixel */;

				// Generate i_hi from g_h and j_i.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(h_g) == Lo(j_i))
					? Lo(j_i) | Raise(Lo(j_i))
					: Raise(Lo(j_i)) /* 'i' verbatim to high pixel */ |
					  Average(Lo(j_i), Hi(h_g)) /* hi to low pixel */;

				// Generate jk_j from j_i and l_k.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(j_i) == Lo(l_k))
					? Lo(l_k) | Raise(Lo(l_k))
					: Hi(j_i) /* 'j' verbatim to low pixel */ |
					  Raise(Average(Hi(j_i), Lo(l_k))) /* jk to high pixel */;

				// Generate l_kl from l_k.
				*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(l_k) == Lo(l_k))
					? l_k
					: Raise(Hi(l_k)) /* 'l' verbatim to high pixel */ |
					  Average(Hi(l_k), Lo(l_k)) /* kl to low pixel */;
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 1 whole lines of source and 2 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip +     src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 2 * dst_pitch);
	}
}

/* Upscales an image by 33% in width and in height; also does color conversion
 * using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *    m n o p q r
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    gm   ghmn hino io   jp   jkpq klqr lr
	 *    m    mn   no   o    p    pq   qr   r
	 */

	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 3) {
		for (x = 0; x < src_x / 6; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = bgr555_to_rgb565(*(uint32_t*) (from    )),
			         d_c = bgr555_to_rgb565(*(uint32_t*) (from + 2)),
			         f_e = bgr555_to_rgb565(*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				  Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				  Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				  Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				  Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch    )),
				         j_i = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
				         l_k = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					  Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					  Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					  Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					  Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				if (likely(y + 2 < src_y))  // Is there a source row 3?
				{
					// -- Row 3 --
					uint32_t n_m = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch * 2    )),
					         p_o = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
					         r_q = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 8));

					// Generate ghmn_gm from h_g and n_m.
					uint32_t hn_gm = Average32(h_g, n_m);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(hn_gm) == Lo(hn_gm))
						? hn_gm
						: Lo(hn_gm) /* gm verbatim to low pixel */ |
						  Raise(Average(Hi(hn_gm), Lo(hn_gm))) /* ghmn to high pixel */;

					// Generate io_hino from h_g, j_i, n_m and p_o.
					uint32_t io_hn =
						Hi(hn_gm) /* hn verbatim to low pixel */ |
						Raise(Average(Lo(j_i), Lo(p_o))) /* io to high pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(io_hn) == Lo(io_hn))
						? io_hn
						: Raise(Hi(io_hn)) /* io verbatim to high pixel */ |
						  Average(Hi(io_hn), Lo(io_hn)) /* hino to low pixel */;

					// Generate lr_klqr from l_k and r_q.
					uint32_t lr_kq = Average32(l_k, r_q);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(lr_kq) == Lo(lr_kq))
						? lr_kq
						: Raise(Hi(lr_kq)) /* lr verbatim to high pixel */ |
						  Average(Hi(lr_kq), Lo(lr_kq)) /* klqr to low pixel */;

					// Generate jkpq_jp from j_i, l_k, p_o and r_q.
					uint32_t kq_jp =
						Raise(Lo(lr_kq)) /* kq verbatim to high pixel */ |
						Average(Hi(j_i), Hi(p_o)) /* jp to low pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(kq_jp) == Lo(kq_jp))
						? kq_jp
						: Lo(kq_jp) /* jp verbatim to low pixel */ |
						  Raise(Average(Hi(kq_jp), Lo(kq_jp))) /* jkpq to high pixel */;

					// -- Row 4 --
					// Generate mn_m from n_m.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3) = likely(Hi(n_m) == Lo(n_m))
						? n_m
						: Lo(n_m) /* 'm' verbatim to low pixel */ |
						  Raise(Average(Hi(n_m), Lo(n_m))) /* mn to high pixel */;

					// Generate o_no from n_m and p_o.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = likely(Hi(n_m) == Lo(p_o))
						? Lo(p_o) | Raise(Lo(p_o))
						: Raise(Lo(p_o)) /* 'o' verbatim to high pixel */ |
						  Average(Lo(p_o), Hi(n_m)) /* no to low pixel */;

					// Generate pq_p from p_o and r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 8) = likely(Hi(p_o) == Lo(r_q))
						? Lo(r_q) | Raise(Lo(r_q))
						: Hi(p_o) /* 'p' verbatim to low pixel */ |
						  Raise(Average(Hi(p_o), Lo(r_q))) /* pq to high pixel */;

					// Generate r_qr from r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 12) = likely(Hi(r_q) == Lo(r_q))
						? r_q
						: Raise(Hi(r_q)) /* 'r' verbatim to high pixel */ |
						  Average(Hi(r_q), Lo(r_q)) /* qr to low pixel */;
				}
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}
}

/* Upscales an image based on subpixel rendering; also does color conversion
 * using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 3/2)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_subpixel(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint_fast16_t sectY;
	for (sectY = 0; sectY < src_y / 2; sectY++)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < src_x / 3; sectX++)
		{
			uint_fast16_t rightCol = (sectX == src_x / 3 - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section.
			 *
			 * a b c | d
			 * e f g | h
			 */
			uint32_t a = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 --

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: SubpixelRGB1_3(a, b);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: SubpixelRGB1_1(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: SubpixelRGB3_1(c, d);

			// -- Row 2 --
			// All pixels in this row are blended from the two rows.
			uint32_t e = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch    )),
			         f = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 2)),
			         g = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 4)),
			         h = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + rightCol));

			// -- Row 2 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch) = likely(a == e)
				? a
				: Average(a, e);

			// -- Row 2 pixel 2 (X = 0.75) --
			uint16_t e1f3 = likely(e == f)
				? e
				: SubpixelRGB1_3(e, f);
			uint16_t a1b3 = likely(a == b)
				? a
				: SubpixelRGB1_3(a, b);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 2) = likely(a1b3 == e1f3)
				? a1b3
				: Average(a1b3, e1f3);

			// -- Row 2 pixel 3 (X = 1.5) --
			uint16_t fg = likely(f == g)
				? f
				: SubpixelRGB1_1(f, g);
			uint16_t bc = likely(b == c)
				? b
				: SubpixelRGB1_1(b, c);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 4) = likely(bc == fg)
				? bc
				: Average(bc, fg);

			// -- Row 2 pixel 4 (X = 2.25) --
			uint16_t g3h1 = likely(g == h)
				? g
				: SubpixelRGB3_1(g, h);
			uint16_t c3d1 = likely(c == d)
				? c
				: SubpixelRGB3_1(c, d);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 6) = /* in Y */ likely(g3h1 == c3d1)
				? c3d1
				: Average(c3d1, g3h1);

			// -- Row 3 --

			// -- Row 3 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2) = e;

			// -- Row 3 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 2) = likely(e == f)
				? e
				: SubpixelRGB1_3(e, f);

			// -- Row 3 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(f == g)
				? f
				: SubpixelRGB1_1(f, g);

			// -- Row 3 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 6) = likely(g == h)
				? g
				: SubpixelRGB3_1(g, h);

			from += 3;
			to   += 4;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 1 whole lines of source and 2 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 1 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 2 * dst_pitch);
	}
}

/* Upscales an image by 33% in width and in height, based on subpixel
 * rendering; also does color conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect_subpixel(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint_fast16_t sectY;
	for (sectY = 0; sectY < src_y / 3; sectY++)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < src_x / 3; sectX++)
		{
			uint_fast16_t rightCol = (sectX == src_x / 3 - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 * e f g | h
			 * i j k | l
			 * ---------
			 * m n o | p
			 */
			uint32_t a = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 --
			// All pixels in this row use 0 as the Y coordinate.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: SubpixelRGB1_3(a, b);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: SubpixelRGB1_1(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: SubpixelRGB3_1(c, d);

			// -- Row 2 --
			// All pixels in this row use 0.75 as the Y coordinate.
			uint32_t e = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch    )),
			         f = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 2)),
			         g = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 4)),
			         h = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + rightCol));

			// -- Row 2 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch) = likely(a == e)
				? a
				: AverageQuarters3_1(e, a);

			// -- Row 2 pixel 2 (X = 0.75) --
			uint16_t e1f3 = likely(e == f)
				? e
				: SubpixelRGB1_3(e, f);
			uint16_t a1b3 = likely(a == b)
				? a
				: SubpixelRGB1_3(a, b);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 2) = /* in Y */ likely(a1b3 == e1f3)
				? a1b3
				: AverageQuarters3_1(/* in X, bottom */ e1f3, /* in X, top */ a1b3);

			// -- Row 2 pixel 3 (X = 1.5) --
			uint16_t fg = likely(f == g)
				? f
				: SubpixelRGB1_1(f, g);
			uint16_t bc = likely(b == c)
				? b
				: SubpixelRGB1_1(b, c);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 4) = /* in Y */ likely(bc == fg)
				? bc
				: AverageQuarters3_1(/* in X, bottom */ fg, /* in X, top */ bc);

			// -- Row 2 pixel 4 (X = 2.25) --
			uint16_t g3h1 = likely(g == h)
				? g
				: SubpixelRGB3_1(g, h);
			uint16_t c3d1 = likely(c == d)
				? c
				: SubpixelRGB3_1(c, d);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 6) = /* in Y */ likely(g3h1 == c3d1)
				? c3d1
				: AverageQuarters3_1(/* in X, bottom */ g3h1, /* in X, top */ c3d1);

			// -- Row 3 --
			// All pixels in this row use 1.5 as the Y coordinate.
			uint32_t i = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2    )),
			         j = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 2)),
			         k = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
			         l = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + rightCol));

			// -- Row 3 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2) = likely(e == i)
				? e
				: Average(e, i);

			// -- Row 3 pixel 2 (X = 0.75) --
			uint16_t i1j3 = likely(i == j)
				? i
				: SubpixelRGB1_3(i, j);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 2) = /* in Y */ likely(e1f3 == i1j3)
				? e1f3
				: Average(e1f3, i1j3);

			// -- Row 3 pixel 3 (X = 1.5) --
			uint16_t jk = likely(j == k)
				? j
				: SubpixelRGB1_1(j, k);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = /* in Y */ likely(fg == jk)
				? fg
				: Average(fg, jk);

			// -- Row 3 pixel 4 (X = 2.25) --
			uint16_t k3l1 = likely(k == l)
				? k
				: SubpixelRGB3_1(k, l);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 6) = /* in Y */ likely(g3h1 == k3l1)
				? g3h1
				: Average(g3h1, k3l1);

			// -- Row 4 --
			// All pixels in this row use 2.25 as the Y coordinate.
			uint32_t m = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3    )),
			         n = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 2)),
			         o = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 4)),
			         p = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + rightCol));

			// -- Row 4 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3) = likely(i == m)
				? i
				: AverageQuarters3_1(i, m);

			// -- Row 4 pixel 2 (X = 0.75) --
			uint16_t m1n3 = likely(m == n)
				? m
				: SubpixelRGB1_3(m, n);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 2) = /* in Y */ likely(i1j3 == m1n3)
				? i1j3
				: AverageQuarters3_1(/* in X, top */ i1j3, /* in X, bottom */ m1n3);

			// -- Row 4 pixel 3 (X = 1.5) --
			uint16_t no = likely(n == o)
				? n
				: SubpixelRGB1_1(n, o);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = /* in Y */ likely(jk == no)
				? jk
				: AverageQuarters3_1(/* in X, top */ jk, /* in X, bottom */ no);

			// -- Row 4 pixel 4 (X = 2.25) --
			uint16_t o3p1 = likely(o == p)
				? o
				: SubpixelRGB3_1(o, p);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 6) = /* in Y */ likely(k3l1 == o3p1)
				? k3l1
				: AverageQuarters3_1(/* in X, top */ k3l1, /* in X, bottom */ o3p1);

			from += 3;
			to   += 4;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}

	if (src_y % 3 == 1)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < src_x / 3; sectX++)
		{
			uint_fast16_t rightCol = (sectX == src_x / 3 - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 */
			uint32_t a = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			from += 3;
			to   += 4;
		}
	}
}

/* Upscales an image by 33% in width and in height with bilinear filtering;
 * also does color conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect_bilinear(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint_fast16_t sectY;
	for (sectY = 0; sectY < src_y / 3; sectY++)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < src_x / 3; sectX++)
		{
			uint_fast16_t rightCol = (sectX == src_x / 3 - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 * e f g | h
			 * i j k | l
			 * ---------
			 * m n o | p
			 */
			uint32_t a = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 --
			// All pixels in this row use 0 as the Y coordinate.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			// -- Row 2 --
			// All pixels in this row use 0.75 as the Y coordinate.
			uint32_t e = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch    )),
			         f = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 2)),
			         g = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 4)),
			         h = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + rightCol));

			// -- Row 2 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch) = likely(a == e)
				? a
				: AverageQuarters3_1(e, a);

			// -- Row 2 pixel 2 (X = 0.75) --
			uint16_t e1f3 = likely(e == f)
				? e
				: AverageQuarters3_1(f, e);
			uint16_t a1b3 = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 2) = /* in Y */ likely(a1b3 == e1f3)
				? a1b3
				: AverageQuarters3_1(/* in X, bottom */ e1f3, /* in X, top */ a1b3);

			// -- Row 2 pixel 3 (X = 1.5) --
			uint16_t fg = likely(f == g)
				? f
				: Average(f, g);
			uint16_t bc = likely(b == c)
				? b
				: Average(b, c);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 4) = /* in Y */ likely(bc == fg)
				? bc
				: AverageQuarters3_1(/* in X, bottom */ fg, /* in X, top */ bc);

			// -- Row 2 pixel 4 (X = 2.25) --
			uint16_t g3h1 = likely(g == h)
				? g
				: AverageQuarters3_1(g, h);
			uint16_t c3d1 = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 6) = /* in Y */ likely(g3h1 == c3d1)
				? c3d1
				: AverageQuarters3_1(/* in X, bottom */ g3h1, /* in X, top */ c3d1);

			// -- Row 3 --
			// All pixels in this row use 1.5 as the Y coordinate.
			uint32_t i = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2    )),
			         j = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 2)),
			         k = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
			         l = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + rightCol));

			// -- Row 3 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2) = likely(e == i)
				? e
				: Average(e, i);

			// -- Row 3 pixel 2 (X = 0.75) --
			uint16_t i1j3 = likely(i == j)
				? i
				: AverageQuarters3_1(j, i);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 2) = /* in Y */ likely(e1f3 == i1j3)
				? e1f3
				: Average(e1f3, i1j3);

			// -- Row 3 pixel 3 (X = 1.5) --
			uint16_t jk = likely(j == k)
				? j
				: Average(j, k);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = /* in Y */ likely(fg == jk)
				? fg
				: Average(fg, jk);

			// -- Row 3 pixel 4 (X = 2.25) --
			uint16_t k3l1 = likely(k == l)
				? k
				: AverageQuarters3_1(k, l);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 6) = /* in Y */ likely(g3h1 == k3l1)
				? g3h1
				: Average(g3h1, k3l1);

			// -- Row 4 --
			// All pixels in this row use 2.25 as the Y coordinate.
			uint32_t m = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3    )),
			         n = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 2)),
			         o = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 4)),
			         p = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + rightCol));

			// -- Row 4 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3) = likely(i == m)
				? i
				: AverageQuarters3_1(i, m);

			// -- Row 4 pixel 2 (X = 0.75) --
			uint16_t m1n3 = likely(m == n)
				? m
				: AverageQuarters3_1(n, m);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 2) = /* in Y */ likely(i1j3 == m1n3)
				? i1j3
				: AverageQuarters3_1(/* in X, top */ i1j3, /* in X, bottom */ m1n3);

			// -- Row 4 pixel 3 (X = 1.5) --
			uint16_t no = likely(n == o)
				? n
				: Average(n, o);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = /* in Y */ likely(jk == no)
				? jk
				: AverageQuarters3_1(/* in X, top */ jk, /* in X, bottom */ no);

			// -- Row 4 pixel 4 (X = 2.25) --
			uint16_t o3p1 = likely(o == p)
				? o
				: AverageQuarters3_1(o, p);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 6) = /* in Y */ likely(k3l1 == o3p1)
				? k3l1
				: AverageQuarters3_1(/* in X, top */ k3l1, /* in X, bottom */ o3p1);

			from += 3;
			to   += 4;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}

	if (src_y % 3 == 1)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < src_x / 3; sectX++)
		{
			uint_fast16_t rightCol = (sectX == src_x / 3 - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 */
			uint32_t a = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			from += 3;
			to   += 4;
		}
	}
}

/* Upscales an image by 33% in width and 50% in height with bilinear
 * filtering; also does color conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 3/2)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_bilinear(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	const uint32_t dst_x = src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint_fast16_t sectY;
	for (sectY = 0; sectY < src_y / 2; sectY++)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < src_x / 3; sectX++)
		{
			uint_fast16_t rightCol = (sectX == src_x / 3 - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section.
			 *
			 * a b c | d
			 * e f g | h
			 */
			uint32_t a = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 --

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			// -- Row 2 --
			// All pixels in this row are blended from the two rows.
			uint32_t e = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch    )),
			         f = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 2)),
			         g = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 4)),
			         h = bgr555_to_rgb565_16(*(uint16_t*) ((uint8_t*) from + src_pitch + rightCol));

			// -- Row 2 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch) = likely(a == e)
				? a
				: Average(a, e);

			// -- Row 2 pixel 2 (X = 0.75) --
			uint16_t e1f3 = likely(e == f)
				? e
				: AverageQuarters3_1(f, e);
			uint16_t a1b3 = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 2) = likely(a1b3 == e1f3)
				? a1b3
				: Average(a1b3, e1f3);

			// -- Row 2 pixel 3 (X = 1.5) --
			uint16_t fg = likely(f == g)
				? f
				: Average(f, g);
			uint16_t bc = likely(b == c)
				? b
				: Average(b, c);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 4) = likely(bc == fg)
				? bc
				: Average(bc, fg);

			// -- Row 2 pixel 4 (X = 2.25) --
			uint16_t g3h1 = likely(g == h)
				? g
				: AverageQuarters3_1(g, h);
			uint16_t c3d1 = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 6) = /* in Y */ likely(g3h1 == c3d1)
				? c3d1
				: Average(c3d1, g3h1);

			// -- Row 3 --

			// -- Row 3 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2) = e;

			// -- Row 3 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 2) = likely(e == f)
				? e
				: AverageQuarters3_1(f, e);

			// -- Row 3 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(f == g)
				? f
				: Average(f, g);

			// -- Row 3 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 6) = likely(g == h)
				? g
				: AverageQuarters3_1(g, h);

			from += 3;
			to   += 4;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 1 whole lines of source and 2 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 1 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 2 * dst_pitch);
	}
}

static inline void gba_render(uint16_t* Dest, uint16_t* Src,
	uint32_t SrcPitch, uint32_t DestPitch)
{
	Dest = (uint16_t*) ((uint8_t*) Dest
		+ ((GCW0_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT) / 2 * DestPitch)
		+ ((GCW0_SCREEN_WIDTH - GBA_SCREEN_WIDTH) / 2 * sizeof(uint16_t))
	);
	uint32_t SrcSkip = SrcPitch - GBA_SCREEN_WIDTH * sizeof(uint16_t);
	uint32_t DestSkip = DestPitch - GBA_SCREEN_WIDTH * sizeof(uint16_t);

	uint32_t X, Y;
	for (Y = 0; Y < GBA_SCREEN_HEIGHT; Y++)
	{
		for (X = 0; X < GBA_SCREEN_WIDTH * sizeof(uint16_t) / sizeof(uint32_t); X++)
		{
			*(uint32_t*) Dest = bgr555_to_rgb565(*(uint32_t*) Src);
			Dest += 2;
			Src += 2;
		}
		Src = (uint16_t*) ((uint8_t*) Src + SrcSkip);
		Dest = (uint16_t*) ((uint8_t*) Dest + DestSkip);
	}
}

static inline void gba_convert(uint16_t* Dest, uint16_t* Src,
	uint32_t SrcPitch, uint32_t DestPitch)
{
	uint32_t SrcSkip = SrcPitch - GBA_SCREEN_WIDTH * sizeof(uint16_t);
	uint32_t DestSkip = DestPitch - GBA_SCREEN_WIDTH * sizeof(uint16_t);

	uint32_t X, Y;
	for (Y = 0; Y < GBA_SCREEN_HEIGHT; Y++)
	{
		for (X = 0; X < GBA_SCREEN_WIDTH * sizeof(uint16_t) / sizeof(uint32_t); X++)
		{
			*(uint32_t*) Dest = bgr555_to_rgb565(*(uint32_t*) Src);
			Dest += 2;
			Src += 2;
		}
		Src = (uint16_t*) ((uint8_t*) Src + SrcSkip);
		Dest = (uint16_t*) ((uint8_t*) Dest + DestSkip);
	}
}

/* Downscales an image by half in width and in height; also does color
 * conversion using the function above.
 * Input:
 *   Src: A pointer to the pixels member of a 240x160 surface to be read by
 *     this function. The pixel format of this surface is XBGR 1555.
 *   DestX: The column to start the thumbnail at in the destination surface.
 *   DestY: The row to start the thumbnail at in the destination surface.
 *   SrcPitch: The number of bytes making up a scanline in the source
 *     surface.
 *   DstPitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   Dest: A pointer to the pixels member of a surface to be filled with the
 *     downscaled GBA image. The pixel format of this surface is RGB 565.
 */
void gba_render_half(uint16_t* Dest, uint16_t* Src, uint32_t DestX, uint32_t DestY,
	uint32_t SrcPitch, uint32_t DestPitch)
{
	Dest = (uint16_t*) ((uint8_t*) Dest
		+ (DestY * DestPitch)
		+ (DestX * sizeof(uint16_t))
	);
	uint32_t SrcSkip = SrcPitch - GBA_SCREEN_WIDTH * sizeof(uint16_t);
	uint32_t DestSkip = DestPitch - (GBA_SCREEN_WIDTH / 2) * sizeof(uint16_t);

	uint32_t X, Y;
	for (Y = 0; Y < GBA_SCREEN_HEIGHT / 2; Y++)
	{
		for (X = 0; X < GBA_SCREEN_WIDTH * sizeof(uint16_t) / (sizeof(uint32_t) * 2); X++)
		{
			/* Before:
			*    a b c d
			*    e f g h
			*
			* After (multiple letters = average):
			*    abef cdgh
			*/
			uint32_t b_a = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) Src    )),
			         d_c = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) Src + 4)),
			         f_e = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) Src + SrcPitch    )),
			         h_g = bgr555_to_rgb565(*(uint32_t*) ((uint8_t*) Src + SrcPitch + 4));
			uint32_t bf_ae = likely(b_a == f_e)
				? b_a
				: Average32(b_a, f_e);
			uint32_t dh_cg = likely(d_c == h_g)
				? d_c
				: Average32(d_c, h_g);
			*(uint32_t*) Dest = likely(bf_ae == dh_cg)
				? bf_ae
				: Average(Hi(bf_ae), Lo(bf_ae)) |
				  Raise(Average(Hi(dh_cg), Lo(dh_cg)));
			Dest += 2;
			Src += 4;
		}
		Src = (uint16_t*) ((uint8_t*) Src + SrcSkip + SrcPitch);
		Dest = (uint16_t*) ((uint8_t*) Dest + DestSkip);
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
				if (SDL_MUSTLOCK(OutputSurface))
					SDL_UnlockSurface(OutputSurface);
				SDL_BlitSurface(BorderSurface, NULL, OutputSurface, NULL);
				if (SDL_MUSTLOCK(OutputSurface))
					SDL_LockSurface(OutputSurface);
			}
			// or clear the rest of the screen to prevent image remanence.
			else
				memset(OutputSurface->pixels, 0, OutputSurface->pitch * GCW0_SCREEN_HEIGHT);
			break;

		case scaled_aspect:
		case scaled_aspect_subpixel:
		case scaled_aspect_bilinear:
			memset(OutputSurface->pixels, 0, OutputSurface->pitch * GCW0_SCREEN_HEIGHT);
			break;

		case fullscreen:
		case fullscreen_subpixel:
		case fullscreen_bilinear:
		case hardware:
			break;
	}
}

void ScaleModeUnapplied()
{
	FramesBordered = 0;
}

void ReGBA_RenderScreen(void)
{
	if (ReGBA_IsRenderingNextFrame())
	{
		Stats.TotalRenderedFrames++;
		Stats.RenderedFrames++;
		video_scale_type ResolvedScaleMode = ResolveSetting(ScaleMode, PerGameScaleMode);
		if (FramesBordered < 3)
		{
			ApplyScaleMode(ResolvedScaleMode);
			FramesBordered++;
		}
		switch (ResolvedScaleMode)
		{
#ifndef GCW_ZERO
			case hardware: /* Hardware, when there's no hardware to scale
			                  images, acts as unscaled */
#endif
			case unscaled:
				gba_render(OutputSurface->pixels, GBAScreen, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case fullscreen:
				gba_upscale(OutputSurface->pixels, GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case fullscreen_bilinear:
				gba_upscale_bilinear(OutputSurface->pixels, GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case fullscreen_subpixel:
				gba_upscale_subpixel(OutputSurface->pixels, GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case scaled_aspect:
				gba_upscale_aspect((uint16_t*) ((uint8_t*)
					OutputSurface->pixels +
					(((GCW0_SCREEN_HEIGHT - (GBA_SCREEN_HEIGHT) * 4 / 3) / 2) * OutputSurface->pitch)) /* center vertically */,
					GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case scaled_aspect_bilinear:
				gba_upscale_aspect_bilinear((uint16_t*) ((uint8_t*)
					OutputSurface->pixels +
					(((GCW0_SCREEN_HEIGHT - (GBA_SCREEN_HEIGHT) * 4 / 3) / 2) * OutputSurface->pitch)) /* center vertically */,
					GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case scaled_aspect_subpixel:
				gba_upscale_aspect_subpixel((uint16_t*) ((uint8_t*)
					OutputSurface->pixels +
					(((GCW0_SCREEN_HEIGHT - (GBA_SCREEN_HEIGHT) * 4 / 3) / 2) * OutputSurface->pitch)) /* center vertically */,
					GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

#ifdef GCW_ZERO
			case hardware:
				gba_convert(OutputSurface->pixels, GBAScreen, GBAScreenSurface->pitch, OutputSurface->pitch);
#endif
		}
		ReGBA_DisplayFPS();

		ReGBA_VideoFlip();

		while (true)
		{
			unsigned int AudioFastForwardedCopy = AudioFastForwarded;
			unsigned int FramesAhead = (VideoFastForwarded >= AudioFastForwardedCopy)
				? /* no overflow */ VideoFastForwarded - AudioFastForwardedCopy
				: /* overflow */    0x100 - (AudioFastForwardedCopy - VideoFastForwarded);
			uint32_t Quota = AUDIO_OUTPUT_BUFFER_SIZE * 3 * OUTPUT_FREQUENCY_DIVISOR + (uint32_t) (FramesAhead * (SOUND_FREQUENCY / 59.73f));
			if (ReGBA_GetAudioSamplesAvailable() <= Quota)
				break;
			usleep(1000);
		}
	}

	if (ReGBA_GetAudioSamplesAvailable() < AUDIO_OUTPUT_BUFFER_SIZE * 2 * OUTPUT_FREQUENCY_DIVISOR)
	{
		if (AudioFrameskip < MAX_AUTO_FRAMESKIP)
			AudioFrameskip++;
		SufficientAudioControl = 0;
	}
	else
	{
		SufficientAudioControl++;
		if (SufficientAudioControl >= 10)
		{
			SufficientAudioControl = 0;
			if (AudioFrameskip > 0)
				AudioFrameskip--;
		}
	}

	if (FastForwardFrameskip > 0 && FastForwardFrameskipControl > 0)
	{
		FastForwardFrameskipControl--;
		VideoFastForwarded = (VideoFastForwarded + 1) & 0xFF;
	}
	else
	{
		FastForwardFrameskipControl = FastForwardFrameskip;
		uint32_t ResolvedUserFrameskip = ResolveSetting(UserFrameskip, PerGameUserFrameskip);
		if (ResolvedUserFrameskip != 0)
		{
			if (UserFrameskipControl == 0)
				UserFrameskipControl = ResolvedUserFrameskip - 1;
			else
				UserFrameskipControl--;
		}
	}

	if (AudioFrameskipControl > 0)
		AudioFrameskipControl--;
	else
		AudioFrameskipControl = AudioFrameskip;
}

u16 *copy_screen()
{
	u32 pitch = GBAScreenPitch;
	u16 *copy = malloc(GBA_SCREEN_WIDTH * GBA_SCREEN_HEIGHT * sizeof(uint16_t));
	u16 *dest_ptr = copy;
	u16 *src_ptr = GBAScreen;
	u32 x, y;

	for(y = 0; y < GBA_SCREEN_HEIGHT; y++)
	{
		memcpy(dest_ptr, src_ptr, GBA_SCREEN_WIDTH * sizeof(u16));
		src_ptr += pitch;
		dest_ptr += GBA_SCREEN_WIDTH;
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

static uint32_t CutString(const char* String, const uint32_t MaxWidth,
	struct StringCut* Cuts, uint32_t CutsAllocated)
{
	uint32_t Cut = 0;
	uint32_t CutStart = 0, Cur = 0, CutWidth = 0;
	uint32_t LastSpace = -1;
	bool SpaceInCut = false;
	while (String[Cur] != '\0')
	{
		if (String[Cur] != '\n')
		{
			if (String[Cur] == ' ')
			{
				LastSpace = Cur;
				SpaceInCut = true;
			}
			CutWidth += _font_width[(uint8_t) String[Cur]];
		}

		if (String[Cur] == '\n' || CutWidth > MaxWidth)
		{
			if (Cut < CutsAllocated)
				Cuts[Cut].Start = CutStart;
			if (String[Cur] == '\n')
			{
				if (Cut < CutsAllocated)
					Cuts[Cut].End = Cur;
			}
			else if (CutWidth > MaxWidth)
			{
				if (SpaceInCut)
				{
					if (Cut < CutsAllocated)
						Cuts[Cut].End = LastSpace;
					Cur = LastSpace;
				}
				else
				{
					if (Cut < CutsAllocated)
						Cuts[Cut].End = Cur;
					Cur--; // Next iteration redoes this character
				}
			}
			CutStart = Cur + 1;
			CutWidth = 0;
			SpaceInCut = false;
			Cut++;
		}
		Cur++;
	}

	if (Cut < CutsAllocated)
	{
		Cuts[Cut].Start = CutStart;
		Cuts[Cut].End = Cur;
	}
	return Cut + 1;
}

uint32_t GetSectionRenderedWidth(const char* String, const uint32_t Start, const uint32_t End)
{
	uint32_t Result = 0, i;
	for (i = Start; i < End; i++)
		Result += _font_width[(uint8_t) String[i]];
	return Result;
}

void PrintString(const char* String, uint16_t TextColor,
	void* Dest, uint32_t DestPitch, uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height,
	enum HorizontalAlignment HorizontalAlignment, enum VerticalAlignment VerticalAlignment)
{
	struct StringCut* Cuts = malloc((Height / _font_height) * sizeof(struct StringCut));
	uint32_t CutCount = CutString(String, Width, Cuts, Height / _font_height), Cut;
	if (CutCount > Height / _font_height)
		CutCount = Height / _font_height;

	for (Cut = 0; Cut < CutCount; Cut++)
	{
		uint32_t TextWidth = GetSectionRenderedWidth(String, Cuts[Cut].Start, Cuts[Cut].End);
		uint32_t LineX, LineY;
		switch (HorizontalAlignment)
		{
			case LEFT:   LineX = X;                           break;
			case CENTER: LineX = X + (Width - TextWidth) / 2; break;
			case RIGHT:  LineX = (X + Width) - TextWidth;     break;
			default:     LineX = 0; /* shouldn't happen */    break;
		}
		switch (VerticalAlignment)
		{
			case TOP:
				LineY = Y + Cut * _font_height;
				break;
			case MIDDLE:
				LineY = Y + (Height - CutCount * _font_height) / 2 + Cut * _font_height;
				break;
			case BOTTOM:
				LineY = (Y + Height) - (CutCount - Cut) * _font_height;
				break;
			default:
				LineY = 0; /* shouldn't happen */
				break;
		}

		uint32_t Cur;
		for (Cur = Cuts[Cut].Start; Cur < Cuts[Cut].End; Cur++)
		{
			uint32_t glyph_offset = (uint32_t) String[Cur] * _font_height;
			uint32_t glyph_width = _font_width[(uint8_t) String[Cur]];
			uint32_t glyph_column, glyph_row;
			uint16_t current_halfword;

			for(glyph_row = 0; glyph_row < _font_height; glyph_row++, glyph_offset++)
			{
				current_halfword = _font_bits[glyph_offset];
				for (glyph_column = 0; glyph_column < glyph_width; glyph_column++)
				{
					if ((current_halfword >> (15 - glyph_column)) & 0x01)
						*(uint16_t*) ((uint8_t*) Dest + (LineY + glyph_row) * DestPitch + (LineX + glyph_column) * sizeof(uint16_t)) = TextColor;
				}
			}

			LineX += glyph_width;
		}
	}

	free(Cuts);
}

uint32_t GetRenderedWidth(const char* str)
{
	struct StringCut* Cuts = malloc(sizeof(struct StringCut));
	uint32_t CutCount = CutString(str, UINT32_MAX, Cuts, 1);
	if (CutCount > 1)
	{
		Cuts = realloc(Cuts, CutCount * sizeof(struct StringCut));
		CutString(str, UINT32_MAX, Cuts, CutCount);
	}

	uint32_t Result = 0, LineWidth, Cut;
	for (Cut = 0; Cut < CutCount; Cut++)
	{
		LineWidth = 0;
		uint32_t Cur;
		for (Cur = Cuts[Cut].Start; Cur < Cuts[Cut].End; Cur++)
		{
			LineWidth += _font_width[(uint8_t) str[Cur]];
		}
		if (LineWidth > Result)
			Result = LineWidth;
	}

	free(Cuts);

	return Result;
}

uint32_t GetRenderedHeight(const char* str)
{
	return CutString(str, UINT32_MAX, NULL, 0) * _font_height;
}

void PrintStringOutline(const char* String, uint16_t TextColor, uint16_t OutlineColor,
	void* Dest, uint32_t DestPitch, uint32_t X, uint32_t Y, uint32_t Width, uint32_t Height,
	enum HorizontalAlignment HorizontalAlignment, enum VerticalAlignment VerticalAlignment)
{
	uint32_t sx, sy;
	for (sx = 0; sx <= 2; sx++)
		for (sy = 0; sy <= 2; sy++)
			if (!(sx == 1 && sy == 1))
				PrintString(String, OutlineColor, Dest, DestPitch, X + sx, Y + sy, Width - 2, Height - 2, HorizontalAlignment, VerticalAlignment);
	PrintString(String, TextColor, Dest, DestPitch, X + 1, Y + 1, Width - 2, Height - 2, HorizontalAlignment, VerticalAlignment);
}

static void ProgressUpdateInternal(uint32_t Current, uint32_t Total)
{
	char* Line;
	switch (CurrentFileAction)
	{
		case FILE_ACTION_LOAD_BIOS:
			Line = "Reading the GBA BIOS";
			break;
		case FILE_ACTION_LOAD_BATTERY:
			Line = "Reading saved data";
			break;
		case FILE_ACTION_SAVE_BATTERY:
			Line = "Writing saved data";
			break;
		case FILE_ACTION_LOAD_STATE:
			Line = "Reading saved state";
			break;
		case FILE_ACTION_SAVE_STATE:
			Line = "Writing saved state";
			break;
		case FILE_ACTION_LOAD_ROM_FROM_FILE:
			Line = "Reading ROM from a file";
			break;
		case FILE_ACTION_DECOMPRESS_ROM_TO_RAM:
			Line = "Decompressing ROM";
			break;
		case FILE_ACTION_DECOMPRESS_ROM_TO_FILE:
			Line = "Decompressing ROM into a file";
			break;
		case FILE_ACTION_APPLY_GAME_COMPATIBILITY:
			Line = "Applying compatibility fixes";
			break;
		case FILE_ACTION_LOAD_GLOBAL_SETTINGS:
			Line = "Reading global settings";
			break;
		case FILE_ACTION_SAVE_GLOBAL_SETTINGS:
			Line = "Writing global settings";
			break;
		case FILE_ACTION_LOAD_GAME_SETTINGS:
			Line = "Loading per-game settings";
			break;
		case FILE_ACTION_SAVE_GAME_SETTINGS:
			Line = "Writing per-game settings";
			break;
		default:
			Line = "File action ongoing";
			break;
	}
	SDL_FillRect(OutputSurface, NULL, COLOR_PROGRESS_BACKGROUND);

	SDL_Rect TopLine = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2, (GCW0_SCREEN_HEIGHT - PROGRESS_HEIGHT) / 2, PROGRESS_WIDTH, 1 };
	SDL_FillRect(OutputSurface, &TopLine, COLOR_PROGRESS_OUTLINE);

	SDL_Rect BottomLine = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2, (GCW0_SCREEN_HEIGHT - PROGRESS_HEIGHT) / 2 + PROGRESS_HEIGHT - 1, PROGRESS_WIDTH, 1 };
	SDL_FillRect(OutputSurface, &BottomLine, COLOR_PROGRESS_OUTLINE);

	SDL_Rect LeftLine = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2, (GCW0_SCREEN_HEIGHT - PROGRESS_HEIGHT) / 2, 1, PROGRESS_HEIGHT };
	SDL_FillRect(OutputSurface, &LeftLine, COLOR_PROGRESS_OUTLINE);

	SDL_Rect RightLine = { (GCW0_SCREEN_WIDTH + PROGRESS_WIDTH) / 2 - 1, (GCW0_SCREEN_HEIGHT - PROGRESS_HEIGHT) / 2, 1, PROGRESS_HEIGHT };
	SDL_FillRect(OutputSurface, &RightLine, COLOR_PROGRESS_OUTLINE);

	SDL_Rect Content = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2 + 1, (GCW0_SCREEN_HEIGHT - PROGRESS_HEIGHT) / 2 + 1, (uint32_t) ((uint64_t) Current * (PROGRESS_WIDTH - 2) / Total), PROGRESS_HEIGHT - 2 };
	SDL_FillRect(OutputSurface, &Content, COLOR_PROGRESS_CONTENT);

	PrintStringOutline(Line, COLOR_PROGRESS_TEXT_CONTENT, COLOR_PROGRESS_TEXT_OUTLINE, OutputSurface->pixels, OutputSurface->pitch, 0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, CENTER, MIDDLE);

	ReGBA_VideoFlip();
}

void ReGBA_ProgressInitialise(enum ReGBA_FileAction Action)
{
	if (Action == FILE_ACTION_SAVE_BATTERY)
		return; // Ignore this completely, because it flashes in-game
	clock_gettime(CLOCK_MONOTONIC, &LastProgressUpdate);
	CurrentFileAction = Action;
	InFileAction = true;
	ScaleModeUnapplied();
	ProgressUpdateInternal(0, 1);
}

void ReGBA_ProgressUpdate(uint32_t Current, uint32_t Total)
{
	struct timespec Now, Difference;
	clock_gettime(CLOCK_MONOTONIC, &Now);
	Difference = TimeDifference(LastProgressUpdate, Now);
	if (InFileAction &&
	    (Difference.tv_sec > 0 || Difference.tv_nsec > 50000000 || Current == Total)
	   )
	{
		ProgressUpdateInternal(Current, Total);
		LastProgressUpdate = Now;
	}
}

void ReGBA_ProgressFinalise()
{
	InFileAction = false;
}

void ReGBA_VideoFlip()
{
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_UnlockSurface(OutputSurface);
	SDL_Flip(OutputSurface);
	if (SDL_MUSTLOCK(OutputSurface))
		SDL_LockSurface(OutputSurface);
}
