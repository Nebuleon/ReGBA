/* unofficial gameplaySP kai
 *
 * Copyright (C) 2007 NJ
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

#ifndef UNICODE_H
#define UNICODE_H

// 関数宣言
// UTF-8 -> Shift_JIS
int utf8_to_sjis(const void *utf8_text, void *buf);

// UTF-16 -> Shift_JIS
int utf16_to_sjis(const void *utf16_text, void *buf);

// Shift_JIS -> UTF-8
int sjis_to_utf8(const void *sjis_text, void *buf);

// Shift_JIS -> UTF-16LE
int sjis_to_utf16le(const void *sjis_text, void *buf);

// Shift_JIS -> UTF-16BE
int sjis_to_utf16be(const void *sjis_text, void *buf);

unsigned short sjis_to_ucs2(unsigned short sjis);

int sjis_to_cut(void *buf, const void *sjis_text, int len);

#endif
