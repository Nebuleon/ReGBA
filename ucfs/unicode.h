/* Header file for the FreeDOS-32 Unicode Support Library version 2.0
 * by Salvo Isaja, 2005
 */
#ifndef __FD32_UNICODE_H
#define __FD32_UNICODE_H

/** \file
 *  \brief Unicode facilities to manage UTF-8, UTF-16 and wide characters.
 */

#if 0 /* Standard headers */
 #include <stdint.h>
 #include <errno.h>
 #include <stdlib.h>
 #include <wchar.h>
#else /* FreeDOS-32 headers */
//#include <dr-env.h>
#include <errno.h>
#include <lfn.h>
#endif

#ifdef __GNUC__
 #define restrict __restrict__
#endif

/// Gets the length of a UTF-8 character.
int unicode_utf8len(int lead_byte);

/// UTF-8 to wide character.
int unicode_utf8towc(wchar_t *restrict result, const char *restrict string, size_t size);

/// Wide character to UTF-8.
int unicode_wctoutf8(char *s, wchar_t wc, size_t size);

/// Gets the length of a UTF-16 character.
int unicode_utf16len(int lead_word);

/// UTF-16 to wide character.
int unicode_utf16towc(wchar_t *restrict result, const uint16_t *restrict string, size_t size);

/// Wide character to UTF-16.
int unicode_wctoutf16(uint16_t *s, wchar_t wc, size_t size);

/// Simple case folding of a wide character.
wchar_t unicode_simple_fold(wchar_t wc);

#endif /* #ifndef __FD32_UNICODE_H */
