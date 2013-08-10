/* unofficial gpSP kai
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

#include "common.h"

unsigned int swi_hle_handle[0x2B][3] = {
  /* { (unsigned int) void* HandlerAddress, unsigned int InputParameterCount, unsigned int ReturnsValue } */
  { 0, 0, 0, },              // SWI 0:  SoftReset
  { 0, 0, 0, },              // SWI 1:  RegisterRAMReset
  { 0, 0, 0, },              // SWI 2:  Halt
  { 0, 0, 0, },              // SWI 3:  Stop/Sleep
  { 0, 0, 0, },              // SWI 4:  IntrWait
  { 0, 0, 0, },              // SWI 5:  VBlankIntrWait
  { 1 /* special */, 0, 0, },              // SWI 6:  Div
  { 1 /* special */, 0, 0, },              // SWI 7:  DivArm
  { 0, 0, 0, },      // SWI 8:  Sqrt
  { 0, 0, 0, },      // SWI 9:  ArcTan
  { 0, 0, 0, },    // SWI A:  ArcTan2
  { 0, 0, 0, },              // SWI B:  CpuSet
  { 0, 0, 0, },              // SWI C:  CpuFastSet
  { 0, 0, 0, },              // SWI D:  GetBIOSCheckSum
  { 0, 0, 0, },  // SWI E:  BgAffineSet
  { 0, 0, 0, },  // SWI F:  ObjAffineSet
  { 0, 0, 0, },              // SWI 10: BitUnpack
  { 0, 0, 0, },              // SWI 11: LZ77UnCompWram
  { 0, 0, 0, },              // SWI 12: LZ77UnCompVram
  { 0, 0, 0, },              // SWI 13: HuffUnComp
  { 0, 0, 0, },              // SWI 14: RLUnCompWram
  { 0, 0, 0, },              // SWI 15: RLUnCompVram
  { 0, 0, 0, },              // SWI 16: Diff8bitUnFilterWram
  { 0, 0, 0, },              // SWI 17: Diff8bitUnFilterVram
  { 0, 0, 0, },              // SWI 18: Diff16bitUnFilter
  { 0, 0, 0, },              // SWI 19: SoundBias
  { 0, 0, 0, },              // SWI 1A: SoundDriverInit
  { 0, 0, 0, },              // SWI 1B: SoundDriverMode
  { 0, 0, 0, },              // SWI 1C: SoundDriverMain
  { 0, 0, 0, },              // SWI 1D: SoundDriverVSync
  { 0, 0, 0, },              // SWI 1E: SoundChannelClear
  { 0, 0, 0, },              // SWI 1F: MidiKey2Freq
  { 0, 0, 0, },              // SWI 20: SoundWhatever0
  { 0, 0, 0, },              // SWI 21: SoundWhatever1
  { 0, 0, 0, },              // SWI 22: SoundWhatever2
  { 0, 0, 0, },              // SWI 23: SoundWhatever3
  { 0, 0, 0, },              // SWI 24: SoundWhatever4
  { 0, 0, 0, },              // SWI 25: MultiBoot
  { 0, 0, 0, },              // SWI 26: HardReset
  { 0, 0, 0, },              // SWI 27: CustomHalt
  { 0, 0, 0, },              // SWI 28: SoundDriverVSyncOff
  { 0, 0, 0, },              // SWI 29: SoundDriverVSyncOn
  { 0, 0, 0, },              // SWI 2A: SoundGetJumpList
};
