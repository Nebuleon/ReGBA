/* The FreeDOS-32 Unicode Support Library version 2.0
 * Copyright (C) 2001-2005  Salvatore ISAJA
 *
 * This file "utf16towc.c" is part of the FreeDOS-32 Unicode
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
 * A UTF-16 character is converted to a wide character (UTF-32 or UCS-4)
 * using the following rules (binary numbers):
 * \code
 * UTF-32                     - UTF-16
 * 00000000 aaaaaaaa aaaaaaaa <-> aaaaaaaa aaaaaaaa
 * 000bbbbb aaaaaaaa aaaaaaaa <-> 110110cc ccaaaaaa  110111aa aaaaaaaa
 * \endcode
 * where \c cccc = \c bbbbb - 1.
 */


/** \param lead_word the first uint16_t of a UTF-16 character;
 *  \retval >0 the length in uint16_t units of the UTF-16 character;
 *  \remarks For performance reasons, this function does not parse
 *           the whole UTF-16 word sequence, just the first uint16_t.
 *           If checking the validity of the whole UTF-16 word sequence
 *           is needed, use #unicode_utf16towc.
 */
int unicode_utf16len(int lead_word)
{
	int res = 1;
	if ((lead_word & 0xFC00) == 0xD800) res = 2;
	return res;
}


/** \param result where to store the converted wide character;
 *  \param string buffer containing the UTF-16 character to convert;
 *  \param size max number of uint16_t units of \c string to examine;
 *  \retval >0 the length in uint16_t units of the processed UTF-16
 *             character, the wide character is stored in \c result;
 *  \retval -EILSEQ invalid UTF-16 word sequence;
 *  \retval -ENAMETOOLONG \c size too small to parse the UTF-16 character.
 */
int unicode_utf16towc(wchar_t *restrict result, const uint16_t *restrict string, size_t size)
{
	if (!size) return -ENAMETOOLONG;
	if ((*string & 0xFC00) != 0xD800)
	{
		*result = (wchar_t) *string;
		return 1;
	}
	if (size < 2) return -ENAMETOOLONG;
	*result = ((*string & 0x03FF) << 10) + 0x010000;
	if ((*(++string) & 0xFC00) != 0xDC00) return -EILSEQ;
	*result |= *string & 0x03FF;
	return 2;
}
