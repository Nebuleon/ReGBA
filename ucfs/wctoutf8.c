/* The FreeDOS-32 Unicode Support Library version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "wctoutf8.c" is part of the FreeDOS-32 Unicode
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

/** \param s where to store the converted UTF-8 character;
 *  \param wc the wide character to convert;
 *  \param size max number of bytes to store in \c s;
 *  \retval >0 the length in bytes of the converted UTF-8 character, stored in \c s;
 *  \retval -EINVAL invalid wide character (don't know how to convert it to UTF-8);
 *  \retval -ENAMETOOLONG \c size too small to store the UTF-8 character.
 */
int unicode_wctoutf8(char *s, wchar_t wc, size_t size)
{
	if (wc >= 0)
	{
		if (wc < 0x000080)
		{
			if (size < 1) return -ENAMETOOLONG;
			*s = (char) wc;
			return 1;
		}
		if (wc < 0x000800)
		{
			if (size < 2) return -ENAMETOOLONG;
			*(s + 1) = (char) (0x80 | (wc & 0x3F)); wc >>= 6;
			*s = (char) (0xC0 | wc);
			return 2;
		}
		if (wc < 0x010000)
		{
			if (size < 3) return -ENAMETOOLONG;
			*(s + 2) = (char) (0x80 | (wc & 0x3F)); wc >>= 6;
			*(s + 1) = (char) (0x80 | (wc & 0x3F)); wc >>= 6;
			*s = (char) (0xE0 | wc);
			return 3;
		}
		if (wc < 0x200000)
		{
			if (size < 4) return -ENAMETOOLONG;
			*(s + 3) = (char) (0x80 | (wc & 0x3F)); wc >>= 6;
			*(s + 2) = (char) (0x80 | (wc & 0x3F)); wc >>= 6;
			*(s + 1) = (char) (0x80 | (wc & 0x3F)); wc >>= 6;
			*s = (char) (0xF0 | wc);
			return 4;
		}
	}
	return -EINVAL;
}
