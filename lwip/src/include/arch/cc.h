/**************************************************************************
*                                                                         *
*   PROJECT     : uCOS_LWIP (uC/OS LwIP port)                             *
*                                                                         *
*   MODULE      : CC.h                                                    *
*                                                                         *
*   AUTHOR      : Michael Anburaj                                         *
*                 URL  : http://geocities.com/michaelanburaj/             *
*                 EMAIL: michaelanburaj@hotmail.com                       *
*                                                                         *
*   PROCESSOR   : Any                                                     *
*                                                                         *
*   TOOL-CHAIN  : Any                                                     *
*                                                                         *
*   DESCRIPTION :                                                         *
*   Architecture related header.                                          *
*                                                                         *
**************************************************************************/

#ifndef __ARCH_CC_H__
#define __ARCH_CC_H__

/* Include some files for defining library routines */
#include <string.h>

#include "little/cpu.h"  //add little/ by Regen

/* Define platform endianness */
#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif /* BYTE_ORDER */

/* Define generic types used in lwIP */
typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;

typedef u32_t mem_ptr_t;

/*define prinf formatters for lwip types*/
#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"

/* Compiler hints for packing structures */
#define PACK_STRUCT_FIELD(x) x __attribute__((packed))
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END

/* prototypes for printf() and abort() */
#if 1
#include <stdio.h>
#include <stdlib.h>
/* Plaform specific diagnostic output */
#define LWIP_PLATFORM_DIAG(x)	{printf x;}

#define LWIP_PLATFORM_ASSERT(x)  {printf("Assertion \"%s\" failed at line %d in %s\n", \
                                     x, __LINE__, __FILE__); /* FIXME */ /*((void(*)(void)) 0)()*/while(1);}
#else
#define LWIP_PLATFORM_DIAG(x)
#define LWIP_PLATFORM_ASSERT(x) {while(1);}
#endif

#define SYS_ARCH_DECL_PROTECT(x) u32_t cpu_sr
#define SYS_ARCH_PROTECT(x)      OS_ENTER_CRITICAL()
#define SYS_ARCH_UNPROTECT(x)    OS_EXIT_CRITICAL()

#endif /* __ARCH_CC_H__ */

