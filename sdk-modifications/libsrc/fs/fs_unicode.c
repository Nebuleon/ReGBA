//fs_unicode.c

#include <string.h>
#include "fs_common.h"

//void _FAT_unicode_init_default() // ANSI CODE PAGE
//{
//	_L2UTable = NULL;
//	_U2LTable = NULL;
//	_ankTable = NULL;
//}

static inline const char* _FAT_utf8decode(const char* utf8, u16 *ucs)
{
    unsigned char c = *utf8++;
    unsigned long code;
    int tail = 0;

    if ((c <= 0x7f) || (c >= 0xc2)) {
        /* Start of new character. */
        if (c < 0x80) {        /* U-00000000 - U-0000007F, 1 byte */
            code = c;
        } else if (c < 0xe0) { /* U-00000080 - U-000007FF, 2 bytes */
            tail = 1;
            code = c & 0x1f;
        } else if (c < 0xf0) { /* U-00000800 - U-0000FFFF, 3 bytes */
            tail = 2;
            code = c & 0x0f;
        } else if (c < 0xf5) { /* U-00010000 - U-001FFFFF, 4 bytes */
            tail = 3;
            code = c & 0x07;
        } else {
            /* Invalid size. */
            code = 0;
        }

        while (tail-- && ((c = *utf8++) != 0)) {
            if ((c & 0xc0) == 0x80) {
                /* Valid continuation character. */
                code = (code << 6) | (c & 0x3f);

            } else {
                /* Invalid continuation char */
                code = 0xfffd;
                utf8--;
                break;
            }
        }
    } else {
        /* Invalid UTF-8 char */
        code = 0;
    }
    /* currently we don't support chars above U-FFFF */
    *ucs = (code < 0x10000) ? code : 0;
    return utf8;
}

void _FAT_utf8_to_unicode16( const char* src, unsigned short* dest )
{
	while('\0' != *src)
	{
		src = _FAT_utf8decode(src, dest++);
	}

	*dest = '\0';
}

#if 0
static inline int _FAT_utf8coding(const u16* ucs, char* dest)
{
	int len= 0;

	if (*ucs < 0x80) //one byte
	{
		dest[len++] = *ucs;
	}
	else if (*ucs < 0x800) //two bytes
	{
		dest[len++] = 0xC0 | (*ucs >> 6 & 0x1F);
		dest[len++] = 0x80 | (*ucs & 0x3F);
	}
	else //if(*ucs < 0x10000) //three bytes
	{
		dest[len++] = 0xE0 | (*ucs >> 12);
		dest[len++] = 0x80 | (*ucs >>6 & 0x3F);
		dest[len++] = 0x80 | (*ucs &0x3F);
	}

	return len;
}
#endif

void _FAT_unicode16_to_utf8( const u16* src, char* dest)
{
	int len=0;
	while(*src)
	{
		if (*src < 0x80) //1 byte
		{
			dest[len++] = *src;
		}
		else if (*src < 0x800) //2 bytes
		{
			dest[len++] = 0xC0 | (*src >> 6 & 0x1F);
			dest[len++] = 0x80 | (*src & 0x3F);
		}
		else //if(*src < 0x10000) //3 bytes
		{
			dest[len++] = 0xE0 | (*src >> 12);
			dest[len++] = 0x80 | (*src >>6 & 0x3F);
			dest[len++] = 0x80 | (*src &0x3F);
		}
		src ++;
	}
	dest[len] = 0;
}

u32 _unistrnlen( const u16* unistr, u32 maxlen )
{
	const u16 * pstr = NULL;
	u32 len = 0;
	if( NULL == unistr )
		return 0;

	if( 0 == maxlen )
		return 0;

	pstr = unistr;

	while( len < maxlen && *pstr != 0x0000 )
	{
		++len; 
		++pstr;
	}
	return len;
}

int _unistrncmp( const u16* src, const u16* dest, u32 maxlen )
{
	if( NULL == src || NULL == dest )
	{
		if( src == dest ) return 0;
		return (NULL == src ) ? -1 : 1;
	}

	while( *src == *dest && maxlen && *src != 0x0000 && *dest != 0x0000 )
	{
		++src;
		++dest;
		--maxlen;
	}
	if( 0 == maxlen ) return 0;

	return *src > *dest ? 1 : -1;
}

const u16 * _unistrchr( const u16 * str, u16 unichar )
{
	if( NULL == str )
		return NULL;

	while( *str != unichar && *str != 0x0000 )
	{
		++str;
	}
	return str;
}

int _uniisalnum( u8 ch )
{
	return isalnum( ch );
}

