/*
 * frmwrk.h
 *
 * Board global macros definition.
 *
 * Author: Seeger Chin
 * e-mail: seeger.chin@gmail.com
 *
 * Copyright (C) 2006 Ingenic Semiconductor Inc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


#ifndef __FRMWRK_H__
#define __FRMWRK_H__

#include "sysdefs.h"

/* ********************************************************************* */
/* Module configuration */

#ifndef _SYS_STKSIZE
#define _SYS_STKSIZE 20*1024
#endif
#ifndef _EXC_STKSIZ
#define _EXC_STKSIZE 20*1024
#endif


/* ********************************************************************* */
/* Interface macro & data definition */

#define C_abRTMem

/* ********************************************************************* */

#ifdef __cplusplus
}
#endif

#endif /*__FRMWRK_H__*/

