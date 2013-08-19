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
#include "font.h"

#include <pspctrl.h>

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>

#include <pspgu.h>
#include <psppower.h>
#include <psprtc.h>

static float *screen_vertex = (float *)0x441FC100;
static u32 *ge_cmd = (u32 *)0x441FC000;
static u16 *psp_gu_vram_base = (u16 *)(0x44000000);
static u32 *ge_cmd_ptr = (u32 *)0x441FC000;
static u32 gecbid;
static u32 video_direct = 0;

static u32 __attribute__((aligned(16))) display_list[32];

#define GBA_SCREEN_WIDTH 240
#define GBA_SCREEN_HEIGHT 160

#define PSP_SCREEN_WIDTH 480
#define PSP_SCREEN_HEIGHT 272
#define PSP_LINE_SIZE 512

#define PSP_ALL_BUTTON_MASK 0xFFFF

#define GE_CMD_FBP    0x9C
#define GE_CMD_FBW    0x9D
#define GE_CMD_TBP0   0xA0
#define GE_CMD_TBW0   0xA8
#define GE_CMD_TSIZE0 0xB8
#define GE_CMD_TFLUSH 0xCB
#define GE_CMD_CLEAR  0xD3
#define GE_CMD_VTYPE  0x12
#define GE_CMD_BASE   0x10
#define GE_CMD_VADDR  0x01
#define GE_CMD_IADDR  0x02
#define GE_CMD_PRIM   0x04
#define GE_CMD_FINISH 0x0F
#define GE_CMD_SIGNAL 0x0C
#define GE_CMD_NOP    0x00

#define GE_CMD(cmd, operand)                                                \
  *ge_cmd_ptr = (((GE_CMD_##cmd) << 24) | (operand));                       \
  ge_cmd_ptr++                                                              \

static u16 *screen_texture = (u16 *)(0x4000000 + (512 * 272 * 2));
static u16 *current_screen_texture = (u16 *)(0x4000000 + (512 * 272 * 2));
static u16 *GBAScreen = (u16 *)(0x4000000 + (512 * 272 * 2));
static u32 GBAScreenPitch = 240;

static void Ge_Finish_Callback(int id, void *arg)
{
}

void init_video()
{
  sceDisplaySetMode(0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);

  sceDisplayWaitVblankStart();
  sceDisplaySetFrameBuf((void*)psp_gu_vram_base, PSP_LINE_SIZE,
   PSP_DISPLAY_PIXEL_FORMAT_565, PSP_DISPLAY_SETBUF_NEXTFRAME);

  sceGuInit();

  sceGuStart(GU_DIRECT, display_list);
  sceGuDrawBuffer(GU_PSM_5650, (void*)0, PSP_LINE_SIZE);
  sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT,
   (void*)0, PSP_LINE_SIZE);
  sceGuClear(GU_COLOR_BUFFER_BIT);

  sceGuOffset(2048 - (PSP_SCREEN_WIDTH / 2), 2048 - (PSP_SCREEN_HEIGHT / 2));
  sceGuViewport(2048, 2048, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT);

  sceGuScissor(0, 0, PSP_SCREEN_WIDTH + 1, PSP_SCREEN_HEIGHT + 1);
  sceGuEnable(GU_SCISSOR_TEST);
  sceGuTexMode(GU_PSM_5650, 0, 0, GU_FALSE);
  sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGBA);
  sceGuTexFilter(GU_LINEAR, GU_LINEAR);
  sceGuEnable(GU_TEXTURE_2D);

  sceGuFrontFace(GU_CW);
  sceGuDisable(GU_BLEND);

  sceGuFinish();
  sceGuSync(0, 0);

  sceDisplayWaitVblankStart();
  sceGuDisplay(GU_TRUE);

  PspGeCallbackData gecb;
  gecb.signal_func = NULL;
  gecb.signal_arg = NULL;
  gecb.finish_func = Ge_Finish_Callback;
  gecb.finish_arg = NULL;
  gecbid = sceGeSetCallback(&gecb);

  screen_vertex[0] = 0 + 0.5;
  screen_vertex[1] = 0 + 0.5;
  screen_vertex[2] = 0 + 0.5;
  screen_vertex[3] = 0 + 0.5;
  screen_vertex[4] = 0;
  screen_vertex[5] = GBA_SCREEN_WIDTH - 0.5;
  screen_vertex[6] = GBA_SCREEN_HEIGHT - 0.5;
  screen_vertex[7] = PSP_SCREEN_WIDTH - 0.5;
  screen_vertex[8] = PSP_SCREEN_HEIGHT - 0.5;
  screen_vertex[9] = 0;

  // Set framebuffer to PSP VRAM
  GE_CMD(FBP, ((u32)psp_gu_vram_base & 0x00FFFFFF));
  GE_CMD(FBW, (((u32)psp_gu_vram_base & 0xFF000000) >> 8) | PSP_LINE_SIZE);
  // Set texture 0 to the screen texture
  GE_CMD(TBP0, ((u32)screen_texture & 0x00FFFFFF));
  GE_CMD(TBW0, (((u32)screen_texture & 0xFF000000) >> 8) | GBA_SCREEN_WIDTH);
  // Set the texture size to 256 by 256 (2^8 by 2^8)
  GE_CMD(TSIZE0, (8 << 8) | 8);
  // Flush the texture cache
  GE_CMD(TFLUSH, 0);
  // Use 2D coordinates, no indeces, no weights, 32bit float positions,
  // 32bit float texture coordinates
  GE_CMD(VTYPE, (1 << 23) | (0 << 11) | (0 << 9) |
   (3 << 7) | (0 << 5) | (0 << 2) | 3);
  // Set the base of the index list pointer to 0
  GE_CMD(BASE, 0);
  // Set the rest of index list pointer to 0 (not being used)
  GE_CMD(IADDR, 0);
  // Set the base of the screen vertex list pointer
  GE_CMD(BASE, ((u32)screen_vertex & 0xFF000000) >> 8);
  // Set the rest of the screen vertex list pointer
  GE_CMD(VADDR, ((u32)screen_vertex & 0x00FFFFFF));
  // Primitive kick: render sprite (primitive 6), 2 vertices
  GE_CMD(PRIM, (6 << 16) | 2);
  // Done with commands
  GE_CMD(FINISH, 0);
  // Raise signal interrupt
  GE_CMD(SIGNAL, 0);
  GE_CMD(NOP, 0);
  GE_CMD(NOP, 0);
}

u32 screen_flip = 0;

void ReGBA_RenderScreen()
{
  if(video_direct == 0)
  {
    u32 *old_ge_cmd_ptr = ge_cmd_ptr;
    sceKernelDcacheWritebackAll();

    // Render the current screen
    ge_cmd_ptr = ge_cmd + 2;
    GE_CMD(TBP0, ((u32) GBAScreen & 0x00FFFFFF));
    GE_CMD(TBW0, (((u32) GBAScreen & 0xFF000000) >> 8) |
     GBA_SCREEN_WIDTH);
    ge_cmd_ptr = old_ge_cmd_ptr;

    sceGeListEnQueue(ge_cmd, ge_cmd_ptr, gecbid, NULL);

    // Flip to the next screen
    screen_flip ^= 1;

    if(screen_flip)
      GBAScreen = screen_texture + (240 * 160 * 2);
    else
      GBAScreen = screen_texture;
  }
}

void video_resolution_large()
{
  if(video_direct != 1)
  {
    video_direct = 1;
    screen_pixels = psp_gu_vram_base;
    screen_pitch = 512;
    sceGuStart(GU_DIRECT, display_list);
    sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT,
     (void*)0, PSP_LINE_SIZE);
    sceGuFinish();
  }
}

void set_gba_resolution(video_scale_type scale)
{
  u32 filter_linear = 0;
  screen_scale = scale;
  switch(scale)
  {
    case unscaled:
      screen_vertex[2] = 120 + 0.5;
      screen_vertex[3] = 56 + 0.5;
      screen_vertex[7] = GBA_SCREEN_WIDTH + 120 - 0.5;
      screen_vertex[8] = GBA_SCREEN_HEIGHT + 56 - 0.5;
      break;

    case scaled_aspect:
      screen_vertex[2] = 36 + 0.5;
      screen_vertex[3] = 0 + 0.5;
      screen_vertex[7] = 408 + 36 - 0.5;
      screen_vertex[8] = PSP_SCREEN_HEIGHT - 0.5;
      break;

    case fullscreen:
      screen_vertex[2] = 0;
      screen_vertex[3] = 0;
      screen_vertex[7] = PSP_SCREEN_WIDTH;
      screen_vertex[8] = PSP_SCREEN_HEIGHT;
      break;
  }

  sceGuStart(GU_DIRECT, display_list);
  if(screen_filter == filter_bilinear)
    sceGuTexFilter(GU_LINEAR, GU_LINEAR);
  else
    sceGuTexFilter(GU_NEAREST, GU_NEAREST);

  sceGuFinish();
  sceGuSync(0, 0);

  clear_screen(0x0000);
}

void video_resolution_small()
{
  if(video_direct != 0)
  {
    set_gba_resolution(screen_scale);
    video_direct = 0;
    screen_pixels = screen_texture;
    screen_flip = 0;
    screen_pitch = 240;
    sceGuStart(GU_DIRECT, display_list);
    sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT,
     (void*)0, PSP_LINE_SIZE);
    sceGuFinish();
  }
}

void clear_screen(u16 color)
{
  u32 i;
  u16 *src_ptr = get_screen_pixels();

  sceGuSync(0, 0);

  for(i = 0; i < (512 * 272); i++, src_ptr++)
  {
    *src_ptr = color;
  }

  // I don't know why this doesn't work.
/*  color = (((color & 0x1F) * 255 / 31) << 0) |
   ((((color >> 5) & 0x3F) * 255 / 63) << 8) |
   ((((color >> 11) & 0x1F) * 255 / 31) << 16) | (0xFF << 24);

  sceGuStart(GU_DIRECT, display_list);
  sceGuDrawBuffer(GU_PSM_5650, (void*)0, PSP_LINE_SIZE);
  //sceGuDispBuffer(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT,
  // (void*)0, PSP_LINE_SIZE);
  sceGuClearColor(color);
  sceGuClear(GU_COLOR_BUFFER_BIT);
  sceGuFinish();
  sceGuSync(0, 0); */
}

void _flush_cache()
{
	sceKernelDcacheWritebackAll();
}
