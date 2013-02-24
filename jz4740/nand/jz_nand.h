#ifndef __JZ_NAND_H__
#define __JZ_NAND_H__

#define CONFIG_SSFDC_HW_RS_ECC 
#define CONFIG_SSFDC_NAND_PAGE_SIZE 2048
#define CONFIG_SSFDC_NAND_PAGE_PER_BLOCK 128
#define CONFIG_SSFDC_NAND_ROW_CYCLE 3
#define CONFIG_SSFDC_WRITE_VERIFY 0
#define CONFIG_SSFDC_WEAR_ENABLE 1
#define JZ_WAIT_STAUTS_DELAY 30


#define NAND_CMD_READ1_00                 0x00
#define NAND_CMD_READ1_01                 0x01
#define NAND_CMD_READ2                    0x50
#define NAND_CMD_READ_ID1                 0x90
#define NAND_CMD_READ_ID2                 0x91
#define NAND_CMD_RESET                    0xFF
#define NAND_CMD_PAGE_PROGRAM_START       0x80
#define NAND_CMD_PAGE_PROGRAM_STOP        0x10
#define NAND_CMD_BLOCK_ERASE_START        0x60
#define NAND_CMD_BLOCK_ERASE_CONFIRM      0xD0
#define NAND_CMD_READ_STATUS              0x70

#define NANDFLASH_CLE           0x00008000 //PA[15]
#define NANDFLASH_ALE           0x00010000 //PA[16]

#define NAND_IO_ADDR           *((volatile unsigned char *) NANDFLASH_BASE)

#define JZ_NAND_SET_CLE    (NANDFLASH_BASE |=  NANDFLASH_CLE)
#define JZ_NAND_CLR_CLE    (NANDFLASH_BASE &= ~NANDFLASH_CLE)
#define JZ_NAND_SET_ALE    (NANDFLASH_BASE |=  NANDFLASH_ALE)
#define JZ_NAND_CLR_ALE    (NANDFLASH_BASE &= ~NANDFLASH_ALE)



#define JZ_NAND_SELECT     (REG_EMC_NFCSR |=  EMC_NFCSR_NFCE1)//EMC_NFCSR_NFE1 |
#define JZ_NAND_DESELECT   (REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))
#define __nand_ecc_enable()   (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST)
#define __nand_ecc_disable()  (REG_EMC_NFECR &= ~EMC_NFECR_ECCE)

#define __nand_ready()		((REG_GPIO_PXPIN(2) & 0x40000000) ? 1 : 0)
#define __nand_ecc_hamming()    (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_HAMMING)
#define __nand_ecc_rs_encoding()  (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_ENCODING)
#define __nand_ecc_rs_decoding()  (REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_DECODING)
#define __nand_ecc_encode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_ENCF))
#define __nand_ecc_decode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_DECF))

#define __nand_ecc()		(REG_EMC_NFECC & 0x00fcffff)


#define JZSOC_ECC_BLOCK		512 /* 9-bytes RS ECC per 512-bytes data */
#define PAR_SIZE                9

#define JZSOC_ECC_STEPS      (CONFIG_SSFDC_NAND_PAGE_SIZE / JZSOC_ECC_BLOCK)



#endif /* __JZ_NAND_H__ */
