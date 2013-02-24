//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2007.
//

#ifndef __SSFDC_H__
#define __SSFDC_H__

// export ssfdc function
int ssfdc_nftl_init(void);
int ssfdc_nftl_release(void);

int ssfdc_nftl_open(void);
void ssfdc_nftl_getInfo(unsigned int zone, pflash_info_t pflashinfo);

unsigned int ssfdc_nftl_read(unsigned int nSectorAddr, unsigned char *pBuffer, unsigned int nSectorNum);
unsigned int ssfdc_nftl_write(unsigned int nSectorAddr, unsigned char *pBuffer, unsigned int nSectorNum);

int ssfdc_nftl_flush_cache(void);
int ssfdc_nftl_format(void);

#endif	// __SSFDC_H__
