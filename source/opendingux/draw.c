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

void init_video()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK) < 0)
	{
		printf("Failed to initialize SDL !!\n");
		return;   // for debug
		// exit(1);
	}

	OutputSurface = SDL_SetVideoMode(GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);
	GBAScreenSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT + 2 /* required to support scaling from 3 rows to 4 maintaining the aspect ratio */, 16,
	  GBA_RED_MASK,
	  GBA_GREEN_MASK,
	  GBA_BLUE_MASK,
	  0 /* alpha: none */);

	// Start drawing on the second line, to have a black line on the top and
	// bottom for the aspect-preserving scaler
	GBAScreen = (uint16_t*) ((uint8_t*) GBAScreenSurface->pixels + GBAScreenSurface->pitch);

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

/* Returns the average of two RGB565 pixels. */
static inline uint16_t Average(uint16_t A, uint16_t B)
{
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
	return ((A & 0xF7DE) >> 1) + ((B & 0xF7DE) >> 1) +
		(A & B & 0x0821);
}

/* Returns the average of two pairs of RGB565 pixels, as a pair thereof. */
static inline uint32_t Average32(uint32_t A, uint32_t B)
{
	return ((A & 0xF7DEF7DE) >> 1) + ((B & 0xF7DEF7DE) >> 1) +
		(A & B & 0x08210821);
}

/* Puts a pixel into the upper half of a pair. */
static inline uint32_t Raise(uint32_t N)
{
	return N << 16;
}

/* Extracts the upper pixel of a pair into the lower pixel of a pair. */
static inline uint32_t HighOf(uint32_t N)
{
	return N >> 16;
}

/* Extracts the lower pixel of a pair. */
static inline uint32_t LowOf(uint32_t N)
{
	return N & 0xFFFF;
}

/* Upscales an image by 33% in with and in height; also does color conversion
 * using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y)
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

	const uint32_t dst_x = src_x * 4/3;

	uint32_t x, y;

	for (y=0; y<src_y/3; y++) {
		for (x=0; x<src_x/6; x++) {
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = bgr555_to_rgb565(*(uint32_t*) (from                )),
			         d_c = bgr555_to_rgb565(*(uint32_t*) (from             + 2)),
			         f_e = bgr555_to_rgb565(*(uint32_t*) (from             + 4)),
			         h_g = bgr555_to_rgb565(*(uint32_t*) (from + src_x        )),
			         j_i = bgr555_to_rgb565(*(uint32_t*) (from + src_x     + 2)),
			         l_k = bgr555_to_rgb565(*(uint32_t*) (from + src_x     + 4)),
			         n_m = bgr555_to_rgb565(*(uint32_t*) (from + src_x * 2    )),
			         p_o = bgr555_to_rgb565(*(uint32_t*) (from + src_x * 2 + 2)),
			         r_q = bgr555_to_rgb565(*(uint32_t*) (from + src_x * 2 + 4));

			from += 6;

			// -- Row 1 --
			// Generate ab_a from b_a.
			uint32_t ab_a = (HighOf(b_a) == LowOf(b_a))
				? b_a
				: LowOf(b_a) /* 'a' verbatim to low pixel */ |
				  Raise(Average(HighOf(b_a), LowOf(b_a))) /* ba to high pixel */;
			*(uint32_t*) (to) = ab_a;

			// Generate c_bc from b_a and d_c.
			uint32_t c_bc = (HighOf(b_a) == LowOf(d_c))
				? LowOf(d_c) | Raise(LowOf(d_c))
				: Raise(LowOf(d_c)) /* 'c' verbatim to high pixel */ |
				  Average(LowOf(d_c), HighOf(b_a)) /* bc to low pixel */;
			*(uint32_t*) (to + 2) = c_bc;

			// Generate de_d from d_c and f_e.
			uint32_t de_d = (HighOf(d_c) == LowOf(f_e))
				? LowOf(f_e) | Raise(LowOf(f_e))
				: HighOf(d_c) /* 'd' verbatim to low pixel */ |
				  Raise(Average(LowOf(f_e), HighOf(d_c))) /* de to high pixel */;
			*(uint32_t*) (to + 4) = de_d;

			// Generate f_ef from f_e.
			uint32_t f_ef = (HighOf(f_e) == LowOf(f_e))
				? f_e
				: Raise(HighOf(f_e)) /* 'f' verbatim to high pixel */ |
				  Average(HighOf(f_e), LowOf(f_e)) /* ef to low pixel */;
			*(uint32_t*) (to + 6) = f_ef;

			// -- Row 2 --
			// Generate abgh_ag from b_a and h_g.
			uint32_t bh_ag =
				Average32(b_a, h_g);
			uint32_t abgh_ag = (HighOf(bh_ag) == LowOf(bh_ag))
				? bh_ag
				: LowOf(bh_ag) /* ag verbatim to low pixel */ |
				  Raise(Average(HighOf(bh_ag), LowOf(bh_ag))) /* abgh to high pixel */;
			*(uint32_t*) (to + dst_x) = abgh_ag;

			// Generate ci_bchi from b_a, d_c, h_g and j_i.
			uint32_t ci_bh =
				HighOf(bh_ag) /* bh verbatim to low pixel */ |
				Raise(Average(LowOf(d_c), LowOf(j_i))) /* ci to high pixel */;
			uint32_t ci_bchi = (HighOf(ci_bh) == LowOf(ci_bh))
				? ci_bh
				: Raise(HighOf(ci_bh)) /* ci verbatim to high pixel */ |
				  Average(HighOf(ci_bh), LowOf(ci_bh)) /* bchi to low pixel */;
			*(uint32_t*) (to + dst_x + 2) = ci_bchi;

			// Generate fl_efkl from f_e and l_k.
			uint32_t fl_ek =
				Average32(f_e, l_k);
			uint32_t fl_efkl = (HighOf(fl_ek) == LowOf(fl_ek))
				? fl_ek
				: Raise(HighOf(fl_ek)) /* fl verbatim to high pixel */ |
				  Average(HighOf(fl_ek), LowOf(fl_ek)) /* efkl to low pixel */;
			*(uint32_t*) (to + dst_x + 6) = fl_efkl;

			// Generate dejk_dj from d_c, f_e, j_i and l_k.
			uint32_t ek_dj =
				Raise(LowOf(fl_ek)) /* ek verbatim to high pixel */ |
				Average(HighOf(d_c), HighOf(j_i)) /* dj to low pixel */;
			uint32_t dejk_dj = (HighOf(ek_dj) == LowOf(ek_dj))
				? ek_dj
				: LowOf(ek_dj) /* dj verbatim to low pixel */ |
				  Raise(Average(HighOf(ek_dj), LowOf(ek_dj))) /* dejk to high pixel */;
			*(uint32_t*) (to + dst_x + 4) = dejk_dj;

			// -- Row 3 --
			// Generate ghmn_gm from h_g and n_m.
			uint32_t hn_gm =
				Average32(h_g, n_m);
			uint32_t ghmn_gm = (HighOf(hn_gm) == LowOf(hn_gm))
				? hn_gm
				: LowOf(hn_gm) /* gm verbatim to low pixel */ |
				  Raise(Average(HighOf(hn_gm), LowOf(hn_gm))) /* ghmn to high pixel */;
			*(uint32_t*) (to + dst_x * 2) = ghmn_gm;

			// Generate io_hino from h_g, j_i, n_m and p_o.
			uint32_t io_hn =
				HighOf(hn_gm) /* hn verbatim to low pixel */ |
				Raise(Average(LowOf(j_i), LowOf(p_o))) /* io to high pixel */;
			uint32_t io_hino = (HighOf(io_hn) == LowOf(io_hn))
				? io_hn
				: Raise(HighOf(io_hn)) /* io verbatim to high pixel */ |
				  Average(HighOf(io_hn), LowOf(io_hn)) /* hino to low pixel */;
			*(uint32_t*) (to + dst_x * 2 + 2) = io_hino;

			// Generate lr_klqr from l_k and r_q.
			uint32_t lr_kq =
				Average32(l_k, r_q);
			uint32_t lr_klqr = (HighOf(lr_kq) == LowOf(lr_kq))
				? lr_kq
				: Raise(HighOf(lr_kq)) /* lr verbatim to high pixel */ |
				  Average(HighOf(lr_kq), LowOf(lr_kq)) /* klqr to low pixel */;
			*(uint32_t*) (to + dst_x * 2 + 6) = lr_klqr;

			// Generate jkpq_jp from j_i, l_k, p_o and r_q.
			uint32_t kq_jp =
				Raise(LowOf(lr_kq)) /* kq verbatim to high pixel */ |
				Average(HighOf(j_i), HighOf(p_o)) /* jp to low pixel */;
			uint32_t jkpq_jp = (HighOf(kq_jp) == LowOf(kq_jp))
				? kq_jp
				: LowOf(kq_jp) /* jp verbatim to low pixel */ |
				  Raise(Average(HighOf(kq_jp), LowOf(kq_jp))) /* jkpq to high pixel */;
			*(uint32_t*) (to + dst_x * 2 + 4) = jkpq_jp;

			// -- Row 4 --
			// Generate mn_m from n_m.
			uint32_t mn_m = (HighOf(n_m) == LowOf(n_m))
				? n_m
				: LowOf(n_m) /* 'm' verbatim to low pixel */ |
				  Raise(Average(HighOf(n_m), LowOf(n_m))) /* mn to high pixel */;
			*(uint32_t*) (to + dst_x * 3) = mn_m;

			// Generate o_no from n_m and p_o.
			uint32_t o_no = (HighOf(n_m) == LowOf(p_o))
				? LowOf(p_o) | Raise(LowOf(p_o))
				: Raise(LowOf(p_o)) /* 'o' verbatim to high pixel */ |
				  Average(LowOf(p_o), HighOf(n_m)) /* no to low pixel */;
			*(uint32_t*) (to + dst_x * 3 + 2) = o_no;

			// Generate pq_p from p_o and r_q.
			uint32_t pq_p = (HighOf(p_o) == LowOf(r_q))
				? LowOf(r_q) | Raise(LowOf(r_q))
				: HighOf(p_o) /* 'p' verbatim to low pixel */ |
				  Raise(Average(HighOf(p_o), LowOf(r_q))) /* pq to high pixel */;
			*(uint32_t*) (to + dst_x * 3 + 4) = pq_p;

			// Generate r_qr from r_q.
			uint32_t r_qr = (HighOf(r_q) == LowOf(r_q))
				? r_q
				: Raise(HighOf(r_q)) /* 'r' verbatim to high pixel */ |
				  Average(HighOf(r_q), LowOf(r_q)) /* qr to low pixel */;
			*(uint32_t*) (to + dst_x * 3 + 6) = r_qr;

			to += 8;
		}

		to += 3*dst_x;
		from += 2*src_x;
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
		uint8_t *src = (uint8_t *) GBAScreen;
		switch (ScaleMode)
		{
			case unscaled:
				gba_render((uint32_t *) OutputSurface->pixels, (uint32_t*) (src + GBAScreenSurface->pitch), GCW0_SCREEN_WIDTH, GCW0_SCREEN_HEIGHT);
				break;

			case fullscreen:
				gba_upscale((uint32_t*) OutputSurface->pixels, (uint32_t*) (src + GBAScreenSurface->pitch), GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT);
				break;

			case scaled_aspect:
				gba_upscale_aspect((uint16_t*) ((uint8_t*)
					OutputSurface->pixels +
					(((GCW0_SCREEN_HEIGHT - GBA_SCREEN_HEIGHT * 4 / 3) / 2) * OutputSurface->pitch)) /* center vertically */,
					(uint16_t*) src, GBA_SCREEN_WIDTH, GBA_SCREEN_HEIGHT);
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