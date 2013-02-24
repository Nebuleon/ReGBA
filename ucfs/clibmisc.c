/*
**********************************************************************
*                          Micrium, Inc.
*                      949 Crestview Circle
*                     Weston,  FL 33327-1848
*
*                            uC/FS
*
*             (c) Copyright 2001 - 2003, Micrium, Inc.
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : clibmisc.c
Purpose     : C Library function implementation.
              To avoid usage of C runtime library function, the FS
              uses its own implementation of the required functions.
              Do not use them yourself in the application, because
              they might be limited to the functionality required by
              the FS. They are NOT meant to be a replacement for an
              ANSI C runtime library !
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

/*********************************************************************
*
*             #include Section
*
**********************************************************************
*/

#include "fs_port.h"
#ifndef FS_FARCHARPTR
  #define FS_FARCHARPTR char *
#endif
#include "fs_conf.h"
#include "fs_clib.h"


/*********************************************************************
*
*             Local Variables        
*
**********************************************************************
*/

const unsigned char _ToUpperTable[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
  0x40, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z', 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
  0x60, 'A',  'B',  'C',  'D',  'E',  'F',  'G',
  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',
  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',
  'X',  'Y',  'Z', 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
  0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
  0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
  0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
  0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
  0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7,
  0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
  0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
  0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
  0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
  0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};


/*********************************************************************
*
*             Global functions
*
**********************************************************************
*/

/*********************************************************************
*
*             FS__CLIB_strchr
*
  Description:
  FS internal function. Locate the first occurrence of c (converted
  to a char) in the string pointed to by s.

  Parameters:
  s           - Pointer to a zero terminated string.
  c           - 'Character' value to find.
 
  Return value:
  ==0         - c was not found
  !=0         - Pointer to the located character in s.
*/

FS_FARCHARPTR FS__CLIB_strchr(const char *s, int c) {
  const char ch = c;

  for (; *s != ch; ++s) {
    if (*s == '\0') {
			return 0;
    }
  }
	return ((FS_FARCHARPTR)s);
}


/*********************************************************************
*
*             FS__CLIB_strlen
*
  Description:
  FS internal function. Compute the length of a string pointed to by s.

  Parameters:
  s           - Pointer to a zero terminated string.
 
  Return value:
  Number of characters preceding the terminating 0.
*/

FS_size_t FS__CLIB_strlen(const char *s) {	
	const char *sc;

  for (sc = s; *sc != '\0'; ++sc) {
  }
	return (sc - s);
}


/*********************************************************************
*
*             FS__CLIB_strncmp
*
  Description:
  FS internal function. Compare not more than n characters from the
  array pointed to by s1 to the array pointed to by s2.

  Parameters:
  s1          - Pointer to a character array.
  s2          - Pointer to a character array.
  n           - Number of characters to compare.
 
  Return value:
  <0          - s1<s2
  ==0         - s1==s2.
  >0          - s1>s2
*/

int FS__CLIB_strncmp(const char *s1, const char *s2, FS_size_t n) {
  for (; 0 < n; ++s1, ++s2, --n) {
    if (*s1 != *s2) {
			return (*(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : +1);
    }
    else if (*s1 == '\0') {
			return 0;
    }
  }
	return 0;
}


/*********************************************************************
*
*             FS__CLIB_strcmp
*
  Description:
  FS internal function. Compare the sring pointed to by s1 with the 
  string pointed to by s2.

  Parameters:
  s1          - Pointer to a zero terminated string.
  s2          - Pointer to a zero terminated string.
 
  Return value:
  <0          - s1<s2
  ==0         - s1==s2.
  >0          - s1>s2
*/

int FS__CLIB_strcmp(const char *s1, const char *s2)	{
  for (; *s1 == *s2; ++s1, ++s2) {
    if (*s1 == '\0') 
      return 0;
  }
  return (*(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : +1);
}

/*********************************************************************
*
*             FS__CLIB_strcasecmp
*
  Description:
  FS internal function. Compare the sring pointed to by s1 with the 
  string pointed to by s2, case insensitive

  Parameters:
  s1          - Pointer to a zero terminated string.
  s2          - Pointer to a zero terminated string.
 
  Return value:
  <0          - s1<s2
  ==0         - s1==s2.
  >0          - s1>s2
*/

int FS__CLIB_strcasecmp(const char *s1, const char *s2)
{
    return(strcasecmp(s1, s2));
}

/*********************************************************************
*
*             FS__CLIB_strcasencmp
*
  Description:
  FS internal function. Compare the sring pointed to by s1 with the 
  string pointed to by s2, case insensitive

  Parameters:
  s1          - Pointer to a zero terminated string.
  s2          - Pointer to a zero terminated string.
 
  Return value:
  <0          - s1<s2
  ==0         - s1==s2.
  >0          - s1>s2
*/

int FS__CLIB_strncasecmp(const char *s1, const char *s2, FS_size_t n)
{
    return(strncasecmp(s1, s2, n));
}


/*********************************************************************
*
*             FS__CLIB_atoi
*
  Description:
  FS internal function. Convert string to int. The function stops with
  the first character it cannot convert. It expects decimal numbers only.
  It can handle +/- at the beginning and leading 0. It cannot handle
  HEX or any other numbers.

  Parameters:
  s           - Pointer to a zero terminated string.
 
  Return value:
  0           - In case of any problem or if the converted value is zero.
  !=0         - Integer value of the converted string.
*/

int FS__CLIB_atoi(const char *s) {
  int value;
  int len;
  int i;
  unsigned int base;
  const char *t;
  char c;
  signed char sign;

  value = 0;
  /* Check for +/- */
  sign = 1;
  len = FS__CLIB_strlen(s);
  if (len <= 0) {
    return 0;
  }
  t = s;
  if (*t == '-') {
    t++;
    sign = -1;
  }
  else if (*t == '+') {
    t++;
  }
  /* Skip leading 0 */
  len = FS__CLIB_strlen(t);
  if (len <= 0) {
    return 0;
  }
  while (1) {
    if (*t != '0') {
      break;
    }
    t++;
    len--;
    if (len <= 0) {
      break;
    }
  }
  /* Find end of number */
  for (i = 0; i < len; i++) {
    if (t[i] > '9') {
      break;
    }
    if (t[i] < '0') {
      break;
    }
  }
  len = i;
  if (len <= 0) {
      return 0;
  }
  /* Calculate base */
  base = 1;
  for (i = 1; i < len; i++) {
    base *= 10;
  }
  /* Get value */
  for (i = 0; i < len; i++) {
    c = t[i];
    if (c > '9') {
      break;
    }
    if (c < '0') {
      break;
    }
    c -= '0';
    value += c*base;
    base /= 10;
  }
  return sign * value;
}
#define OS_MEMSET
#define OS_MEMCPY
#ifndef OS_MEMSET
/*********************************************************************
*
*             FS__CLIB_memset
*
  Description:
  FS internal function. Copy the value of c (converted to an unsigned
  char) into each of the first n characters of the object pointed to 
  by s.

  Parameters:
  s           - Pointer to an object.
  c           - 'Character' value to be set.
  n           - Number of characters to be set.
 
  Return value:
  Value of s.
*/

void *FS__CLIB_memset(void *s, int c, FS_size_t n) {
  const unsigned char uc = c;
  unsigned char *su = (unsigned char *)s;
  
  for (; 0 < n; ++su, --n) {
    *su = uc;
  }
  return s;
}
#else
void *FS__CLIB_memset(void *s, int c, FS_size_t n) {
    return (void *)memset(s,(unsigned char)c,n);
}
#endif
#ifndef OS_MEMCPY
/*********************************************************************
*
*             FS__CLIB_memcpy
*
  Description:
  FS internal function. Copy n characters from the object pointed to
  by s2 into the object pointed to by s1. 

  Parameters:
  s1          - Pointer to an object.
  s2          - Pointer to an object.
  n           - Number of characters to copy.
 
  Return value:
  Value of s1.
*/
void *FS__CLIB_memcpy(void *s1, const void *s2, FS_size_t n) {	
	
	char *su1 = (char *)s1;
	const char *su2 = (const char *)s2;
	  
	for (; 0 < n; ++su1, ++su2, --n) {
		*su1 = *su2;
	}
	return s1;
}
#else
void *FS__CLIB_memcpy(void *s1, const void *s2, FS_size_t n) {	
	return (void *)memcpy(s1,s2,n);
}	
#endif

/*********************************************************************
*
*             FS__CLIB_strncpy
*
  Description:
  FS internal function. Copy not more than n characters from the array
  pointed to by s2 to the array pointed to by s1.

  Parameters:
  s1          - Pointer to a character array.
  s2          - Pointer to a character array.
  n           - Number of characters to copy.
 
  Return value:
  Value of s1.
*/

char *FS__CLIB_strncpy(char *s1, const char *s2, FS_size_t n) {
  char *s;
  
  for (s = s1; (0 < n) && (*s2 != '\0'); --n) {
    *s++ = *s2++;	/* copy at most n chars from s2[] */
  }
  for (; 0 < n; --n) {
    *s++ = '\0';
  }
  return s1;
}


/*********************************************************************
*
*             FS__CLIB_toupper
*
  Description:
  FS internal function. Convert a lowecase letter to a corresponding
  uppercase letter. 

  Parameters:
  c           - Letter to convert.
 
  Return value:
  Corresponding uppercase character.
*/

int FS__CLIB_toupper(int c)	{
  return (int)_ToUpperTable[c];
}

