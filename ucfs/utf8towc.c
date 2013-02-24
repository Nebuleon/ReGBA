/* The FreeDOS-32 Unicode Support Library version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "utf8towc.c" is part of the FreeDOS-32 Unicode
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

/*
 * A UTF-8 character is converted to a wide character (UTF-32 or UCS-4)
 * using the following rules (binary numbers):
 * \code
 * UTF-32                     - UTF-8
 * 00000000 00000000 0aaaaaaa - 0aaaaaaa
 * 00000000 00000bbb bbaaaaaa - 110bbbbb 10aaaaaa
 * 00000000 ccccbbbb bbaaaaaa - 1110cccc 10bbbbbb 10aaaaaa
 * 000dddcc ccccbbbb bbaaaaaa - 11110ddd 10cccccc 10bbbbbb 10aaaaaa
 * \endcode
 */

typedef  unsigned char       uint8_t;

/* Bit mask and bit values of a UTF-8 character lead byte */
static struct { uint8_t mask; uint8_t val; } t[4] =
{ { 0x80, 0x00 }, { 0xE0, 0xC0 }, { 0xF0, 0xE0 }, { 0xF8, 0xF0 } };


/** \param lead_byte the first byte of a UTF-8 character;
 *  \retval >0 the length in bytes of the UTF-8 character;
 *  \retval -EILSEQ invalid UTF-8 lead byte;
 *  \remarks For performance reasons, this function does not parse
 *           the whole UTF-8 byte sequence, just the first byte.
 *           If checking the validity of the whole UTF-8 byte sequence
 *           is needed, use #unicode_utf8towc.
 */
int unicode_utf8len(int lead_byte)
{
	int k;
	for (k = 0; k < 4; k++)
		if ((lead_byte & t[k].mask) == t[k].val)
			return k + 1;
	return -EILSEQ;
}


/** \param result where to store the converted wide character;
 *  \param string buffer containing the UTF-8 character to convert;
 *  \param size max number of bytes of \c string to examine;
 *  \retval >0 the length in bytes of the processed UTF-8 character, the wide character is stored in \c result;
 *  \retval -EILSEQ invalid UTF-8 byte sequence;
 *  \retval -ENAMETOOLONG \c size too small to parse the UTF-8 character.
 */
int unicode_utf8towc(wchar_t *restrict result, const char *restrict string, size_t size)
{
	wchar_t wc = 0;
	unsigned k, j;
	if (!size) return -ENAMETOOLONG;
	for (k = 0; k < 4; k++)
		if ((*string & t[k].mask) == t[k].val)
		{
			if (size < k + 1) return -ENAMETOOLONG;
			wc = (wchar_t) (unsigned char) *string & ~t[k].mask;
			for (j = 0; j < k; j++)
			{
				if ((*(++string) & 0xC0) != 0x80) return -EILSEQ;
				wc = (wc << 6) | ((wchar_t) (unsigned char) *string & 0x3F);
			}
			*result = wc;
			return k + 1;
		}
	return -EILSEQ;
}
