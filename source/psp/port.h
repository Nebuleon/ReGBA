/* Per-platform settings and headers - gpSP on PSP
 *
 * Copyright (C) 2013 GBATemp user Nebuleon
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
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

#ifndef __GPSP_PORT_H__
#define __GPSP_PORT_H__

#include <unistd.h>

#include <pspctrl.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psprtc.h>
#include <psppower.h>
#include <pspsdk.h>
#include <pspkernel.h>
#include <systemctrl_se.h>
#include <psputility.h>
#include <kubridge.h>
#include <pspimpose_driver.h>

#ifdef USE_ADHOC
#include <pspnet.h>
#include <pspnet_adhoc.h>
#include <pspnet_adhocctl.h>
#include <pspnet_adhocmatching.h>
#include <pspwlan.h>
#include <psputility_netmodules.h>
#include "adhoc.h"
#endif

#include "homehook.h"
#include "dvemgr.h"

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspgu.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psprtc.h>

#define printf pspDebugScreenPrintf

#define convert_palette(value)                                              \
  value = ((value & 0x7FE0) << 1) | (value & 0x1F)                          \


typedef u32 FIXED16_16;   // Q16.16 fixed-point

#define FILE_OPEN_APPEND (PSP_O_CREAT | PSP_O_APPEND | PSP_O_TRUNC)
#define FILE_OPEN_READ PSP_O_RDONLY
#define FILE_OPEN_WRITE (PSP_O_CREAT | PSP_O_WRONLY | PSP_O_TRUNC)

#define FILE_OPEN(filename_tag, filename, mode)                             \
  s32 filename_tag = sceIoOpen(filename, FILE_OPEN_##mode, 0777)            \

#define FILE_CHECK_VALID(filename_tag)                                      \
  (filename_tag >= 0)                                                       \

#define FILE_TAG_INVALID                                                    \
  (-1)                                                                      \

#define FILE_CLOSE(filename_tag)                                            \
  sceIoClose(filename_tag)                                                  \

#define FILE_READ(filename_tag, buffer, size)                               \
  sceIoRead(filename_tag, buffer, size)                                     \

#define FILE_WRITE(filename_tag, buffer, size)                              \
  sceIoWrite(filename_tag, buffer, size)                                    \

#define FILE_SEEK(filename_tag, offset, type)                               \
  sceIoLseek(filename_tag, offset, PSP_##type)                              \

/* Tuning parameters for the PSP version of gpSP */
/* Its processors are:
 * a) a Sony CXD2962GG at 222 MHz based on the MIPS R4000
 *    32-bit core with 32 MiB of RAM (2.6 Gbit/s), or 64 MiB of RAM on the
 *    PSP Models 2000, 3000 and N1000;
 * b) an embedded graphics core at 111 MHz;
 * c) a Sony CXD1876 Media Engine chip based on the MIPS R4000
 *    64-bit core with 2 MiB of RAM (2.6 Gbit/s). */
#define READONLY_CODE_CACHE_SIZE          (4 * 1024 * 1024)
#define WRITABLE_CODE_CACHE_SIZE          (4 * 1024 * 1024)
/* The following parameter needs to be at least enough bytes to hold
 * the generated code for the largest instruction on your platform.
 * In most cases, that will be the ARM instruction
 * STMDB R0!, {R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15} */
#define TRANSLATION_CACHE_LIMIT_THRESHOLD (1024)

#define GET_DISK_FREE_SPACE FS_CMD_GET_DISKFREE

typedef s32 FILE_TAG_TYPE;

#endif
