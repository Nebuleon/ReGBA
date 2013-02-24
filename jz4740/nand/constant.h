//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#ifndef __CONSTANT_H__
#define __CONSTANT_H__

//------------------------------------------------------------------------------
//
// NAND PARAM: Get from the NAND datasheet, now support >=64 only
//
//------------------------------------------------------------------------------
#define NAND_OOB_SIZE			64


//------------------------------------------------------------------------------
//
// SSFDC PARAM
//
//------------------------------------------------------------------------------
#define SSFDC_BLOCK_STATUS_NORMAL		0xFF
#define SSFDC_BLOCK_VALID_ENTRY			0x80000000
#define SSFDC_BLOCK_ADDR_MASK			0x7FFFFFFF
#define SSFDC_BLOCK_TAG_FREE			0x01
#define SSFDC_BLOCK_TAG_BAD				0x02


//------------------------------------------------------------------------------
//
// ERROR INDEX
//
//------------------------------------------------------------------------------
#define EIO			1
#define ENOMEM		2
#define ENODEV		3
#define EBUSY		4
#define EINVAL		1


#endif	// __CONSTANT_H__

