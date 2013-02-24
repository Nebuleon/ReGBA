/*
 common.h
 Common definitions and included files for the FATlib

 Copyright (c) 2006 Michael "Chishm" Chisholm
	
 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
  2. Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation and/or
     other materials provided with the distribution.
  3. The name of the author may not be used to endorse or promote products derived
     from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	2006-07-11 - Chishm
		* Original release
*/

#ifndef _COMMON_H
#define _COMMON_H

// When compiling for NDS, make sure NDS is defined
#ifndef NDS
 #if defined ARM9 || defined ARM7
  #define NDS
 #endif
#endif

#if 0
#ifdef NDS
 #include <nds/jtypes.h>
#else
 #include "gba_types.h"
#endif
#endif

#define BYTES_PER_READ 512

#ifndef NULL
 #define NULL 0
#endif

#ifndef bool
#define bool int
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

#ifndef u8
#define u8 unsigned char
#endif

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u32
#define u32 unsigned long
#endif

#ifndef s32
#define s32 long
#endif

struct _reent
{
  /* FILE is a big struct and may change over time.  To try to achieve binary
     compatibility with future versions, put stdin,stdout,stderr here.
     These are pointers into member __sf defined below.  */
//  __FILE *_stdin, *_stdout, *_stderr;	/* XXX */

	int _errno;			/* local copy of errno */

//  int  _inc;			/* used by tmpnam */

//  char *_emergency;

//  int __sdidinit;		/* 1 means stdio has been init'd */

//  int _current_category;	/* unused */
//  _CONST char *_current_locale;	/* unused */

//  struct _mprec *_mp;

//  void _EXFNPTR(__cleanup, (struct _reent *));

//  int _gamma_signgam;

  /* used by some fp conversion routines */
//  int _cvtlen;			/* should be size_t */
//  char *_cvtbuf;

//  struct _rand48 *_r48;
//  struct __tm *_localtime_buf;
//  char *_asctime_buf;

  /* signal info */
//  void (**(_sig_func))(int);

  /* atexit stuff */
//  struct _atexit *_atexit;
//  struct _atexit _atexit0;

//  struct _glue __sglue;			/* root of glue chain */
//  __FILE *__sf;			        /* file descriptors */
//  struct _misc_reent *_misc;            /* strtok, multibyte states */
//  char *_signal_buf;                    /* strsignal */
};

#endif // _COMMON_H
