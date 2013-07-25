/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
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

#ifndef GU_H
#define GU_H

#define FONT_WIDTH  6
#define FONT_HEIGHT 10

//16bit per/pixel
#define GBA_SCREEN_WIDTH 240
#define GBA_SCREEN_HEIGHT 160
#define GBA_SCREEN_BUFF_SIZE GBA_SCREEN_WIDTH*GBA_SCREEN_HEIGHT

#define NDS_SCREEN_WIDTH 256
#define NDS_SCREEN_HEIGHT 192

#define PSP_SCREEN_WIDTH 480
#define PSP_SCREEN_HEIGHT 272

#define FRAME_GAME 0
#define FRAME_MENU 1

// Already provided by ds2io.h on the Supercard DSTwo port.
// #define SCREEN_WIDTH    NDS_SCREEN_WIDTH
// #define SCREEN_HEIGHT   NDS_SCREEN_HEIGHT

typedef enum
{
  unscaled,
  scaled_aspect,
  fullscreen,
  option1,
  option2,
} video_scale_type;

typedef enum
{
  filter_nearest,
  filter_bilinear
} video_filter_type;

typedef enum
{
  PROGRESSIVE,
  INTERLACE
} VIDEO_INTERLACE_TYPE;

typedef enum
{
  R4_3,
  R16_9
} VIDEO_RATIO_TYPE;

typedef struct
{
  float u1;
  float v1;
  float x1;
  float y1;
  float z1;
  float u2;
  float v2;
  float x2;
  float y2;
  float z2;
} SPRITE;

typedef struct
{
  int u;
  int displaymode;
  int width;
  int height;
  int x;
  int y;
  int z;
} VIDEO_OUT_PARAMETER;

typedef struct
{
  int x;
  int y;
  int width;
  int height;
} VIEW_PORT;

typedef struct
{
  u32 x;
  u32 y;
} TEXTURE_BIT;


typedef struct
{
  u32 pitch;
  u32 width;
  u32 height;
} TEXTURE_SIZE;

typedef struct
{
  u32 width;
  u32 height;
} SCREEN_SIZE;

typedef struct
{
  VIDEO_OUT_PARAMETER video_out;  /* pspDveMgrSetVideoOut のパラメータ */
  int filter[2];                  /* MENU表示時のフィルタ */
  TEXTURE_SIZE texture_size;      /* テクスチャサイズ */
  TEXTURE_BIT texture_bit;        /* テクスチャの縦横のビット数 */
  SCREEN_SIZE screen_size;        /* 表示バッファのサイズ */
  VIEW_PORT view;                 /* 表示範囲 */
  SPRITE screen_setting_1;        /* スプライトデータ 1 */
  SPRITE screen_setting_2;        /* スプライトデータ 1 */
} SCREEN_PARAMETER;

#ifndef NDS_LAYER
#define flip_gba_screen flip_screen
#endif

void init_video();
void video_resolution(u32 mode);
void clear_screen(u16 color);
void clear_gba_screen(u16 color);
void blit_to_screen(u16 *src, u32 w, u32 h, u32 x, u32 y);
void copy_screen(u16 *buffer);
extern void flip_screen(void);
extern void flip_gba_screen(void);
void video_read_mem_savestate();
void video_write_mem_savestate();

void debug_screen_clear();
void debug_screen_start();
void debug_screen_end();
void debug_screen_printf(const char *format, ...);
void debug_screen_printl(const char *format, ...);
void debug_screen_newline(u32 count);
void debug_screen_update();

extern u32 screen_pitch;
extern u32 screen_width;
extern u32 screen_width2;
extern u32 screen_height2;
extern u16 *screen_address;
extern u16 *gba_screen_address;
extern u32 gba_screen_pitch;

#ifdef NDS_LAYER /* Supercard DSTwo */
extern void** gba_screen_addr_ptr;
extern u32 gba_screen_num;
#endif

#endif
