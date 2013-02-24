/**************************************************************************
 * FreeDOS 32 FAT Driver                                                  *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2001-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "lfn.c" - Convert long file names in the standard DOS          *
 *                   directory entry format and generate short name       *
 *                   aliases from long file names                         *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS 32 FAT Driver.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is free software; you can redistribute it    *
 * and/or modify it under the terms of the GNU General Public License     *
 * as published by the Free Software Foundation; either version 2 of the  *
 * License, or (at your option) any later version.                        *
 *                                                                        *
 * The FreeDOS 32 FAT Driver is distributed in the hope that it will be   *
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the FreeDOS 32 FAT Driver; see the file COPYING;            *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#include "fat.h"
#include <unicode.h>
#include "lfn.h"

#define FD32_LFNPMAX 260 /* Max length for a long file name path  */
#define FD32_LFNMAX  255 /* Max length for a long file name       */
#define FD32_SFNPMAX 64  /* Max length for a short file name path */
#define FD32_SFNMAX  14  /* Max length for a short file name      */

#define FD32_GENSFN_CASE_CHANGED  (1 << 0)
#define FD32_GENSFN_WAS_INVALID   (1 << 1)
#define FD32_GENSFN_FORMAT_MASK   (0xFF << 8)
#define FD32_GENSFN_FORMAT_FCB    (0x00 << 8)
#define FD32_GENSFN_FORMAT_NORMAL (0x01 << 8)

/* Flag bit settings */
#define RESPECT_WIDTH	1  /* Fixed width wanted 	*/
#define ADD_PLUS	2  /* Add + for positive/floats */
#define SPACE_PAD	4  /* Padding possibility	*/
#define ZERO_PAD	8
#define LEFT_PAD 	16

/* Define the DEBUG symbol in order to activate driver's log output */
#ifdef DEBUG
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif



#define STD_SIZE	0
#define SHORT_SIZE	1
#define LONG_SIZE	2
#define va_list void*

#define __dj_va_rounded_size(T)  \
  (((sizeof (T) + sizeof (int) - 1) / sizeof (int)) * sizeof (int))

#define va_arg(ap, T) \
    (ap = (va_list) ((char *) (ap) + __dj_va_rounded_size (T)),	\
     *((T *) (void *) ((char *) (ap) - __dj_va_rounded_size (T))))

#define va_end(ap)

#define va_start(ap, last_arg) \
 (ap = ((va_list) __builtin_next_arg (last_arg)))

#define INT_MAX    	2147483647

char todigit(int c)
{
    if (c >= 0 && c <= 9) return(c+'0');
    else if (c >= 10 && c <= 15) return(c + 'A' - 10);
    else return(c);    
}

unsigned ucvt(unsigned long v,char *buffer,int base,int width,int flag)
{
    register int i = 0,j;
    unsigned ret = 0,abs_base;
    unsigned long abs_v;
    char tmp[12];
    /* Extract the less significant digit */
    /* Put it into temporary buffer       */
    /* It has to be local to have 	  */
    /* reentrant functions		  */
    /*
    MG: fix to handle zero correctly
    if (v == 0) {
	*buffer++ = '0';
	*buffer = 0;
	return(1);
    }
    */
    
    /* MG: is this required? */
    /* the vsprintf() function seems to call this procedure with */
    /* this flag inverted */
    flag ^= LEFT_PAD;
    
    abs_base = (base >= 0) ? base : -base;
    if (base < 0) abs_v = ((long)(v) < 0) ? -v : v;
    else abs_v = v;
    /* Process left space-padding flags */
    if (flag & ADD_PLUS || ((base < 0) && ((long)(v) < 0))) {
	ret++;
    }
    /* MG: fix to handle zero correctly */
    if (v == 0)
        tmp[i++]='0';
    else
        while (abs_v > 0) {
	    tmp[i++] = todigit(abs_v % abs_base);
	    abs_v = abs_v / abs_base;
        }
    ret += i;
    if ((flag & LEFT_PAD) && (flag & SPACE_PAD)) {
	j = ret;
	while (j < width) {
	    *buffer++ = ' ';
	    ret++;
	    j++;
	}
    }
    /* Add plus if wanted */
    if (base < 0) {
	if (((long)(v)) < 0) *buffer++ = '-';
        else if (flag & ADD_PLUS) *buffer++ = '+';
    } else if (flag & ADD_PLUS) *buffer++ = '+';
    /* Process left zero-padding flags */
    if ((flag & LEFT_PAD) && (flag & ZERO_PAD)) {
	j = ret;
	while (j < width) {
	    *buffer++ = '0';
	    ret++;
	    j++;
	}
    }
    /* Reverse temporary buffer into output buffer */
    /* If wanted left pad with zero/space; anyway only one at once is ok */
    for (j = i-1; j >= 0; j--) *buffer++ = tmp[j];
    if ((flag & (SPACE_PAD)) && !(flag & LEFT_PAD)) {
	/* If wanted, pad with space to specified width */
	j = ret;
	while (j < width) {
	    *buffer++ = ' ';
	    ret++;
	    j++;
	}
    }
    /* Null terminate the output buffer */
    *buffer = 0;
    return(ret);
}

unsigned dcvt(long v,char *buffer,int base,int width,int flag)
{
    return(ucvt((unsigned long)(v),buffer,-base,width,flag));
}

int isnumber(char c, int base)
{
    char max;

    if (base > 9) {
      max = '9';
    } else {
      max = '0' + base - 1;
    }
    if (c >= '0' && c <= max) {
        return 1;
    }
   
    if (base < 11) {
      return 0;
    }

    max = 'A' + (base - 11);
    if (toupper(c) >= 'A' && toupper(c) <= max) {
        return 1;
    }
   
    return 0;
}

int tonumber(char c)
{
    if (c >= '0' && c <= '9') return(c - '0');
    else if (c >= 'A' && c <= 'F') return(c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') return(c - 'a' + 10);
    else return(c);
}

long unsigned strtou(char *s,int base,char **scan_end)
{
    int value,overflow = 0;
    long unsigned result = 0,oldresult;
    /* Skip trailing zeros */
    while (*s == '0') s++;
    if (*s == 'x' && base == 16) {
	s++;
	while (*s == '0') s++;
    }
    /* Convert number */
    while (isnumber(*s,base)) {
	value = tonumber(*s++);
	if (value > base || value < 0) return(0);
	oldresult = result;
	result *= base;
	result += value;
	/* Detect overflow */
	if (oldresult > result) overflow = 1;
    }
    if (scan_end != 0L) *scan_end = s;
    if (overflow) result = INT_MAX;
    return(result);
}

#ifdef FATWRITE
#if 0
/* Returns nonzero if a UTF-8 string is a valid FAT long file name. */
/* Called by allocate_lfn_dir_entries (direntry.c).                 */
int lfn_is_valid(char *s)
{
  for (; *s; s++)
  {
    if (Ch < 0x20) return 0;
    switch (Ch)
    {
      /* + , ; = [ ] . are not valid for short names but are ok for LFN */
      case 0x22 : /* " */
      case 0x2A : /* * */
      case 0x2F : /* / */
      case 0x3A : /* : */
      case 0x3C : /* < */
      case 0x3E : /* > */
      case 0x3F : /* ? */
      case 0x5C : /* \ */
      case 0x7C : /* | */ return 0;
    }
  }
  return 1;
}
#endif


/** \brief Converts a substring in UTF-8 to a subscript in a code page.
 *  \internal
 *  \param nls  NLS operations for the code page;
 *  \param dest destination substring in the national character set;
 *  \param src  source string in UTF-8;
 *  \param stop if not NULL, address in \c src where to stop processing;
 *  \param size max number of bytes to write in \c dest.
 *  \retval 0  success, all characters successfully converted;
 *  \retval >0 success, some inconvertable characters replaced with '_';
 *  \retval <0 error.
 */
static int gen_short_fname2(const struct nls_operations *nls, char *dest, const char *src, const char *stop, size_t size)
{
	/* TODO: Must report WAS_INVALID if an extended char maps to ASCII! */
	int     res = 0;
	int     skip;
	wchar_t wc;
	while (*src)
	{
		if (stop && (src == stop)) break;
		skip = unicode_utf8towc(&wc, src, 6);
		if (skip < 0) return skip;
		src += skip;
		skip =  default_wctomb(dest, wc, size);
		if (skip < 0)
		{
			*(dest++) = '_';
			res = 1;
		}
		else
		{
			if (skip == 1) *dest = default_toupper(*dest);
			dest += skip;
			size -= skip;
		}
	}
	return res;
}


/* Generates a valid 8.3 file name from a valid long file name. */
/* The generated short name has not a numeric tail.             */
/* Returns 0 on success, or a negative error code on failure.   */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
static int gen_short_fname1(const struct nls_operations *nls, char *Dest, char *Source, DWORD Flags)
{
  BYTE  ShortName[11] = "           ";
  char  Purified[FD32_LFNPMAX]; /* A long name without invalid 8.3 characters */
  char *DotPos = NULL; /* Position of the last embedded dot in Source */
  char *s;
  int   Res = 0;
//  printf("source:%s\n",Source);
  /* Find the last embedded dot, if present */
  if (!(*Source)) return -EINVAL;
  for (s = Source + 1; *s; s++)
    if ((*s == '.') && (*(s-1) != '.') && (*(s+1) != '.') && *(s+1))
      DotPos = s;

  /* Convert all characters of Source that are invalid for short names */
  for (s = Purified; *Source;)
    if (!(*Source & 0x80))
    {
      if ((*Source >= 'a') && (*Source <= 'z'))
      {
        *s++ = *Source + 'A' - 'a';
        Res |= FD32_GENSFN_CASE_CHANGED;
      }
      else if (*Source < 0x20) return -EINVAL;
      else switch (*Source)
      {
        /* Spaces and periods must be removed */
        case ' ': break;
        case '.': if (Source == DotPos) DotPos = s, *s++ = '.';
                                   else Res |= FD32_GENSFN_WAS_INVALID;
                  break;
        /* + , ; = [ ] are valid for LFN but not for short names */
        case '+': case ',': case ';': case '=': case '[': case ']':
          *s++ = '_'; Res |= FD32_GENSFN_WAS_INVALID; break;
        /* Check for invalid LFN characters */
        case '"': case '*' : case '/': case ':': case '<': case '>':
        case '?': case '\\': case '|':
          return -EINVAL;
        /* Return any other single-byte character unchanged */
        default : *s++ = *Source;
      }
      Source++;
    }
    else /* Process extended characters */
    {
      wchar_t Ch;//, upCh;
      int     Skip;

      Skip = unicode_utf8towc(&Ch, Source, 6);
      if (Skip < 0) return Skip;
      Source += Skip;
      //upCh = unicode_simple_fold(Ch);
      //if (upCh != Ch) Res |= FD32_GENSFN_CASE_CHANGED;
      //if (upCh < 0x80) Res |= FD32_GENSFN_WAS_INVALID;
      //Skip = unicode_wctoutf8(s, upCh, 6); /* FIXME: Overflow possible */
      Skip = unicode_wctoutf8(s, Ch, 6); /* FIXME: Overflow possible */
      if (Skip < 0) return Skip;
      s += Skip;
    }
  *s = 0;

//  printf("purified:%s\n",Purified);
  /* Convert the Purified name to the OEM code page in FCB format */
  if (gen_short_fname2(nls, ShortName, Purified, DotPos, 8)) Res |= FD32_GENSFN_WAS_INVALID;
  if (DotPos) if (gen_short_fname2(nls, &ShortName[8], DotPos + 1, NULL, 3)) Res |= FD32_GENSFN_WAS_INVALID;
  if (ShortName[0] == ' ') return -EINVAL;
  if (ShortName[0] == 0xE5) Dest[0] = (char) 0x05;


  /* Return the generated short name in the specified format */
  switch (Flags & FD32_GENSFN_FORMAT_MASK)
  {
    case FD32_GENSFN_FORMAT_FCB    : memcpy(Dest, ShortName, 11);
	                             Dest[11]=0;
                                     return Res;
    case FD32_GENSFN_FORMAT_NORMAL : fat_expand_fcb_name(nls, Dest, ShortName, 12); /* was from the FS layer */
                                     return Res;
    default                        : return -ENOTSUP;
  }
}


/* Compares two file names in FCB format. Name2 may contain '?' wildcards. */
/* Returns zero if the names match, or nonzero if they don't match.        */
/* TODO: Works only on single byte character sets!                       */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
int fat_compare_fcb_names(BYTE *Name1, BYTE *Name2)
{
  int k;
  for (k = 0; k < 11; k++)
  {
    if (Name2[k] == '?') continue;
    if (Name1[k] != Name2[k]) return -1;
  }
  return 0;
}


/* Generate a valid 8.3 file name for a long file name, and makes sure */
/* the generated name is unique in the specified directory appending a */
/* "~Counter" if necessary.                                            */
/* Returns 0 if the passed long name is already a valid 8.3 file name  */
/* (including upper case), thus no extra directory entry is required.  */
/* Returns a positive number on successful generation of a short name  */
/* alias for a long file name which is not a valid 8.3 name.           */
/* Returns a negative error code on failure.                           */
/* Called by allocate_lfn_dir_entries (direntry.c).                    */
int gen_short_fname(int Idx, FS_u32 Unit,FS_u32 DirStart,FS_u32 DirSize,char *LongName, FBYTE *ShortName, FWORD Hint)
{
  BYTE       Aux[11];
  BYTE       Auxtemp[11];
  BYTE       szCounter[6];
  int        Counter, szCounterLen;
  int        k, Res;
  char      *s;
  tDirEntry  E;
  struct nls_operations *nls;
  char *buffer;
  FS__fat_dentry_type *ss;
  FS_u32 i;
  FS_u32 dsec;
  int err;
  int curpos=0;
  int findflag=0; 

  buffer = FS__fat_malloc(FS_FAT_SEC_SIZE);
  if (!buffer) {
    return -1;
  }

  LOG_PRINTF(("Generating unique short file name for \"%s\"\n", LongName));
  Res = gen_short_fname1(nls, ShortName, LongName, FD32_GENSFN_FORMAT_FCB); /* was from FS layer */
//printf("gen_short_fname1 returned %d\n", Res);
  if (Res <= 0) return Res;
  /* TODO: Check case change! */
  if (!(Res & FD32_GENSFN_WAS_INVALID)) return 1;
  /* Now append "~Counter" to the short name, incrementing "Counter" */
  /* until the file name is unique in that Path                      */

  for (Counter = 1; Counter < 999999; Counter++)
  {
	  memcpy(Aux, ShortName, 11);
	  /* TODO: it would be better: snprintf(szCounter, sizeof(szCounter), "%d", Counter); */
	  sprintf(szCounter, "%d", Counter);
	  szCounterLen = strlen(szCounter);
	  /* TODO: This may not work properly with multibyte charsets, but this is not
	   *       a problem for now, since the whole NLS stuff has to be replaced. */
	  for (k = 0; (k < 7 - szCounterLen) && (Aux[k] != ' '); k += default_mblen(&Aux[k], NLS_MB_MAX_LEN));
	  Aux[k++] = '~';
	  for (s = szCounter; *s; Aux[k++] = *s++);
	  
	  findflag = 0;
	  /***look for name***/
	  for (i = 0; i < DirSize; i++) 
	  {
		  dsec = FS__fat_dir_realsec(Idx, Unit, DirStart, i);
		  if (dsec == 0) 
		  {
			  FS__fat_free(buffer);
			  return -1;
		  }
		  err = FS__lb_read(FS__pDevInfo[Idx].devdriver, Unit, dsec, (void*)buffer);
		  if (err < 0) 
		  {
			  FS__fat_free(buffer);
			  return -1;
		  }
		  
		  ss = (FS__fat_dentry_type*)buffer;
		  // look for a short name 
		  while (1) 
		  {
			  if (ss >= (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) 
			  {
				  break;  /* End of sector reached */
			  }
			  
			  if (ss->data[11] != 0x00) 
			  { /* not an empty entry */
				  if (ss->data[0] != (unsigned char)0xe5) 
				  { /* not a deleted file */
					  if (ss->data[11] != 0x0f) 
					  {
						  FS__CLIB_strncpy(Auxtemp,ss->data,11);
//						  printf("AUX:%s-%s\n",Auxtemp,Aux);
						  if (fat_compare_fcb_names(Auxtemp, Aux) == 0) 
						  {
							  findflag=1;
							  break;
						  }
					  }
					    
				  }
			  }

			  ss++;
		  }
		  if(findflag == 1) break;
		  if (ss < (FS__fat_dentry_type*)(buffer + FS_FAT_SEC_SIZE)) 
		  {
			  break;
		  }

	  
	  }	  /***look for name***/
          if(findflag == 0) 
	  {
		  memcpy(ShortName, Aux, 11);
//		  printf("Shortname get:%s\n",ShortName);
		  FS__fat_free(buffer);
		  return 1;
	  }

  }

  FS__fat_free(buffer);
  return -1;
}
#endif /* #ifdef FATWRITE */

#if 0
/* This function is temporary, while waiting for the new NLS support */
static int utf8_strupr(char *restrict dest, const char *restrict source)
{
	wchar_t wc;
	int res;
	while (*source)
	{
		res = unicode_utf8towc(&wc, source, 6);
		if (res < 0) return -1;
		source += res;
		res = unicode_wctoutf8(dest, toupper(wc), 6);
		if (res < 0) return -1;
		dest += res;
	}
	*dest = 0;
	return 0;
}
#endif

/* Given a file name Source in UTF-8, checks if it's valid */
/* and returns in Dest the file name in FCB format.        */
/* On success, returns 0 if no wildcards are present, or a */
/* positive number if '?' wildcards are present in Dest.   */
/* On failure, returns a negative error code.              */
/* NOTE: Pasted from fd32/filesys/names.c (that has to be removed) */
int fat_build_fcb_name(FBYTE *Dest, char *Source)
{
  int   WildCards = 0;
//  char  SourceU[FD32_LFNPMAX];
  int   Res;
  int   k;
  struct nls_operations *nls;
  /* Name and extension have to be padded with spaces */
  memset(Dest, 0x20, 11);
  
  /* Build ".          " and "..         " if Source is "." or ".." */
  if ((strcmp(Source, ".") == 0) || (strcmp(Source, "..") == 0))
  {
    for (; *Source; Source++, Dest++) *Dest = *Source;
    return 0;
  }

  // if ((Res = utf8_strupr(SourceU, Source)) < 0) return -EILSEQ;
//  for (k = 0; (SourceU[k] != '.') && SourceU[k]; k++);
  for (k = 0; (Source[k] != '.') && Source[k]; k++);
  Res = gen_short_fname2(nls, Dest, Source, Source[k] ? &Source[k] : NULL, 8);
  if (Res < 0) return Res;
  if (Source[k])
  {
    Res = gen_short_fname2(nls, &Dest[8], &Source[k + 1], NULL, 3);
    if (Res < 0) return Res;
  }
  //utf8_to_oemcp(SourceU, SourceU[k] ? k : -1, Dest, 8);
  //if (SourceU[k]) utf8_to_oemcp(&SourceU[k + 1], -1, &Dest[8], 3);

  if (Dest[0] == ' ') return -EINVAL;
  if (Dest[0] == 0xE5) Dest[0] = 0x05;
  for (k = 0; k < 11;)
  {
    if (Dest[k] < 0x20) return -EINVAL;
    switch (Dest[k])
    {
      case '"': case '+': case ',': case '.': case '/': case ':': case ';':
      case '<': case '=': case '>': case '[': case '\\': case ']':  case '|':
        return -EINVAL;
      case '?': WildCards = 1;
                k++;
                break;
      case '*': WildCards = 1;
                if (k < 8) for (; k < 8; k++) Dest[k] = '?';
                else for (; k < 11; k++) Dest[k] = '?';
                break;
      default : k += default_mblen(&Dest[k], NLS_MB_MAX_LEN);
    }
  }
  Dest[k] = 0; 
  return WildCards;
}


/* Gets a UTF-8 short file name from an FCB file name. */
int fat_expand_fcb_name(const struct nls_operations *nls, char *Dest, const BYTE *Source, size_t size)
{
  const BYTE *NameEnd;
  const BYTE *ExtEnd;
  char  Aux[13];
  const BYTE *s = Source;
  char *a = Aux;

  /* Count padding spaces at the end of the name and the extension */
  for (NameEnd = Source + 7;  *NameEnd == ' '; NameEnd--);
  for (ExtEnd  = Source + 10; *ExtEnd  == ' '; ExtEnd--);

  /* Put the name, a dot and the extension in Aux */
  if (*s == 0x05) *a++ = (char) 0xE5, s++;
  for (; s <= NameEnd; *a++ = (char) *s++);
  if (Source + 8 <= ExtEnd) *a++ = '.';
  for (s = Source + 8; s <= ExtEnd; *a++ = (char) *s++);
  *a = 0;

  /* And finally convert Aux from the OEM code page to UTF-8 */
  for (a = Aux; *a; )
  {
    wchar_t wc;
    int res;
    res = default_mbtowc(&wc, a, 6);
    if (res < 0) return res;
    a += res;
    res = unicode_wctoutf8(Dest, wc, size);
    if (res < 0) return res;
    Dest += res;
    size -= res;
  }
  if (size < 1) return -EINVAL;
  *Dest = 0;
  return 0;
}


int UniToUtf8(FS_u16* src, char* putf8)
{
	int len=0;
	while(*src)
	{
		if (*src < 0x80) //one byte
		{
			putf8[len++] = *src;
			//return 1;
		}
		else if (*src < 0x800) //two byte
		{
			putf8[len++] = 0xC0 | (*src >> 6 & 0x1F);
			putf8[len++] = 0x80 | (*src & 0x3F);
			//return 2;
		}
		else if(*src < 0x10000) //3 bytes
		{
			putf8[len++] = 0xE0 | (*src >> 12);
			putf8[len++] = 0x80 | (*src >>6 & 0x3F);
			putf8[len++] = 0x80 | (*src &0x3F);
			//return 3;
        
		} 
		src ++;
	}
	putf8[len] = 0;
	//if char use morethen 3 char, how to do it?
    //But here have no problem, because the long file name are 2-byte unicode
	return len;
} 
