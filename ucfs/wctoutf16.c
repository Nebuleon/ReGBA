/* The FreeDOS-32 Unicode Support Library version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "wctoutf16.c" is part of the FreeDOS-32 Unicode
 * Support Library (the Program).
 *
 * The Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "unicode.h"

/** \param s where to store the converted UTF-16 character;
 *  \param wc the wide character to convert;
 *  \param size max number of uint16_t units to store in \c s;
 *  \retval >0 the length in uint16_t units of the converted UTF-16 character, stored in \c s;
 *  \retval -EINVAL invalid wide character (don't know how to convert it to UTF-16);
 *  \retval -ENAMETOOLONG \c size too small to store the UTF-16 character.
 */
int unicode_wctoutf16(uint16_t *s, wchar_t wc, size_t size)
{
	if (wc >= 0)
	{
		if (wc < 0x010000)
		{
			*s = (uint16_t) wc;
			return 1;
		}
		if (wc < 0x200000)
		{
			*s       = (uint16_t) (0xD800 + (((wc >> 16) - 1) << 6) + ((wc & 0x00FC00) >> 2));
			*(s + 1) = (uint16_t) (0xDC00 + (wc & 0x0003FF));
			return 2;
		}
	}
	return -EINVAL;
}
