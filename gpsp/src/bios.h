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

#ifndef BIOS_H
#define BIOS_H

// 関数宣言
void bios_halt();
u32 bios_sqrt(u32 value);
void bios_cpuset(u32 source, u32 dest, u32 cnt);
void bios_cpufastset(u32 source, u32 dest, u32 cnt);
void bios_bgaffineset(u32 source, u32 dest, u32 num);
void bios_objaffineset(u32 source, u32 dest, u32 num, u32 offset);

#endif
