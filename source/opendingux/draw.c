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
uint_fast8_t AudioFrameskip = 0;
uint_fast8_t AudioFrameskipControl = 0;
uint_fast8_t UserFrameskipControl = 0;

SDL_Surface *GBAScreenSurface = NULL;
SDL_Surface *OutputSurface = NULL;
SDL_Surface *BorderSurface = NULL;

SDL_Rect ScreenRectangle = {
	0, 0, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT
};

video_scale_type ScaleMode = scaled_aspect;

#define COLOR_PROGRESS_BACKGROUND RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_PROGRESS_TEXT       RGB888_TO_RGB565(255, 255, 255)
#define COLOR_PROGRESS_BORDER     RGB888_TO_RGB565(  0,   0,   0)
#define COLOR_PROGRESS_CONTENT    RGB888_TO_RGB565(255, 255, 255)

#define PROGRESS_WIDTH 240
#define PROGRESS_HEIGHT 13
#define PROGRESS_SPACING 4

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

/* Upscales an image by 33% in with and 50% in height; also does color
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

/* Upscales an image by 33% in with and in height; also does color conversion
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

static inline void gba_render(uint16_t* Dest, uint16_t* Src, uint32_t Width, uint32_t Height,
	uint32_t SrcPitch, uint32_t DestPitch)
{
	Dest = (uint16_t*) ((uint8_t*) Dest
		+ ((Height - GBA_SCREEN_HEIGHT) / 2 * DestPitch)
		+ ((Width - GBA_SCREEN_WIDTH) / 2 * sizeof(uint16_t))
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

void ApplyScaleMode(video_scale_type NewMode)
{
	switch (NewMode)
	{
		case unscaled:
			// Either show the border
			if (BorderSurface != NULL)
				SDL_BlitSurface(BorderSurface, NULL, OutputSurface, NULL);
			// or clear the rest of the screen to prevent image remanence.
			else
				memset(OutputSurface->pixels, 0, OutputSurface->pitch * GCW0_SCREEN_HEIGHT);
			break;

		case scaled_aspect:
			memset(OutputSurface->pixels, 0, OutputSurface->pitch * GCW0_SCREEN_HEIGHT);
			break;

		case fullscreen:
			break;
	}
	ScaleMode = NewMode;
}

void ReGBA_RenderScreen(void)
{
	if (ReGBA_IsRenderingNextFrame())
	{
		Stats.RenderedFrames++;
		ApplyScaleMode(ScaleMode);
		switch (ScaleMode)
		{
			case unscaled:
				gba_render(OutputSurface->pixels, GBAScreen, GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case fullscreen:
				gba_upscale(OutputSurface->pixels, GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;

			case scaled_aspect:
				gba_upscale_aspect((uint16_t*) ((uint8_t*)
					OutputSurface->pixels +
					(((GCW0_SCREEN_HEIGHT - (GBA_SCREEN_HEIGHT) * 4 / 3) / 2) * OutputSurface->pitch)) /* center vertically */,
					GBAScreen, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT, GBAScreenSurface->pitch, OutputSurface->pitch);
				break;
		}
		ReGBA_DisplayFPS();

		SDL_Flip(OutputSurface);

		while (ReGBA_GetAudioSamplesAvailable() > AUDIO_OUTPUT_BUFFER_SIZE * 3 * OUTPUT_FREQUENCY_DIVISOR + (VideoFastForwarded - AudioFastForwarded) * ((int) (SOUND_FREQUENCY / 59.73)))
		{
			if (AudioFrameskip > 0)
				AudioFrameskip--;
			usleep(1000);
		}
	}

	if (ReGBA_GetAudioSamplesAvailable() < AUDIO_OUTPUT_BUFFER_SIZE * 2 * OUTPUT_FREQUENCY_DIVISOR)
	{
		if (AudioFrameskip < MAX_AUTO_FRAMESKIP)
			AudioFrameskip++;
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

	if (AudioFrameskipControl > 0)
		AudioFrameskipControl--;
	else if (AudioFrameskipControl == 0)
		AudioFrameskipControl = AudioFrameskip;

	if (UserFrameskip != 0)
	{
		if (UserFrameskipControl == 0)
			UserFrameskipControl = UserFrameskip - 1;
		else
			UserFrameskipControl--;
		if (FastForwardControl < 60 && FastForwardValue + FastForwardControl >= 60)
			UserFrameskipControl = 0; // allow fast-forwarding even at FS 0
	}
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

uint32_t GetRenderedWidth(const char* str)
{
	uint_fast8_t current_char;
	u32 str_index = 0;
	uint32_t Result = 0;

	while((current_char = str[str_index++]) != '\0')
	{
		Result += _font_width[current_char];
	}

	return Result;
}

uint32_t GetRenderedHeight(const char* str)
{
	return _font_height;
}

void print_string(const char *str, u16 fg_color,
 u32 x, u32 y)
{
  print_string_ext(str, fg_color, x, y, OutputSurface->pixels, GCW0_SCREEN_WIDTH);
}

void print_string_outline(const char *str, u16 fg_color, u16 border_color,
 u32 x, u32 y)
{
	int32_t sx, sy;
	for (sx = -1; sx <= 1; sx++)
		for (sy = -1; sy <= 1; sy++)
			if (!(sx == 0 && sy == 0))
				print_string(str, border_color, x + sx, y + sy);
	print_string(str, fg_color, x, y);
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
	SDL_FillRect(OutputSurface, &ScreenRectangle, COLOR_PROGRESS_BACKGROUND);
	uint32_t Width = GetRenderedWidth(Line);
	uint32_t Height = GetRenderedHeight(Line);
	uint32_t Y = (GCW0_SCREEN_HEIGHT - (Height + PROGRESS_SPACING + PROGRESS_HEIGHT)) / 2;

	print_string_outline(Line, COLOR_PROGRESS_TEXT, COLOR_PROGRESS_BORDER, (GCW0_SCREEN_WIDTH - Width) / 2, Y);
	Y += Height + PROGRESS_SPACING;

	SDL_Rect TopLine = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2, Y, PROGRESS_WIDTH, 1 };
	SDL_FillRect(OutputSurface, &TopLine, COLOR_PROGRESS_CONTENT);

	SDL_Rect BottomLine = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2, Y + PROGRESS_HEIGHT - 1, PROGRESS_WIDTH, 1 };
	SDL_FillRect(OutputSurface, &BottomLine, COLOR_PROGRESS_CONTENT);

	SDL_Rect LeftLine = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2, Y, 1, PROGRESS_HEIGHT };
	SDL_FillRect(OutputSurface, &LeftLine, COLOR_PROGRESS_CONTENT);

	SDL_Rect RightLine = { (GCW0_SCREEN_WIDTH + PROGRESS_WIDTH) / 2 - 1, Y, 1, PROGRESS_HEIGHT };
	SDL_FillRect(OutputSurface, &RightLine, COLOR_PROGRESS_CONTENT);

	SDL_Rect Content = { (GCW0_SCREEN_WIDTH - PROGRESS_WIDTH) / 2 + 1, Y + 1, (uint32_t) ((uint64_t) Current * (PROGRESS_WIDTH - 2) / Total), PROGRESS_HEIGHT - 2 };
	SDL_FillRect(OutputSurface, &Content, COLOR_PROGRESS_CONTENT);
	SDL_Flip(OutputSurface);
}

void ReGBA_ProgressInitialise(enum ReGBA_FileAction Action)
{
	clock_gettime(CLOCK_MONOTONIC, &LastProgressUpdate);
	CurrentFileAction = Action;
	InFileAction = true;
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
