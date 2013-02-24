/*
**********************************************************************
*
*                            uC/MMC
*
*             (c) Copyright 2005 - 2007, Ingenic Semiconductor, Inc
*                      All rights reserved.
*
***********************************************************************

----------------------------------------------------------------------
File        : mmc_jz.c
Purpose     : Jz47xx low level functions to MMC/SD.

----------------------------------------------------------------------
Version-Date-----Author-Explanation
----------------------------------------------------------------------
1.00.00 20060831 WeiJianli     First release

----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
(none)
----------------------------------------------------------------------
*/

/***************************************************************************
 * Define for DMA mode
 *
 */
#include "jz4740.h"
#include "dm.h"
#include "mmc_config.h"
#include "mmc_protocol.h"
#include "mmc_api.h"
#include "mmc_core.h"
#include "ucos_ii.h"

#define MMC_DMA_ENABLE  1
#define MMC_DMA_INTERRUPT 1


static OS_EVENT *mmc_dma_rx_sem;
static OS_EVENT *mmc_dma_tx_sem;
static OS_EVENT *mmc_msc_irq_sem;
#define RX_DMA_CHANNEL 0
#define TX_DMA_CHANNEL 1
#define IRQ 2

#define PHYSADDR(x) ((x) & 0x1fffffff)

//#define DEBUG(x,y...) printf(y);
/***************************************************************************
 * Define card detect and power function etc.
 * This may be modified for a different platform.
 */

#define JZ4740_PAV_1_0 0
#define JZ4740_PAV_1_1 0
#define JZ4740_PAV_1_2 1

#if JZ4740_PAV_1_0
#define MMC_CD_PIN    (16 + 3 * 32)	/* Pin to check card insertion */
#define MMC_POWER_PIN (15 + 3 * 32)	/* Pin to enable/disable card power */
#endif


#if JZ4740_PAV_1_1
#define MMC_CD_PIN    (15 + 3 * 32)	/* Pin to check card insertion */
#define MMC_POWER_PIN (16 + 3 * 32)	/* Pin to enable/disable card power */
#endif

#if JZ4740_PAV_1_2
#define MMC_CD_PIN    (24 + 4 * 32)	/* Pin to check card insertion */
//#define MMC_POWER_PIN (17 + 3 * 32)	/* Pin to enable/disable card power */
//#define MMC_PW_PIN    (14 + 3 * 32)	/* Pin to check protect card */
#endif


void inline MMC_INIT_GPIO()
{
	__gpio_as_msc();
#ifdef MMC_POWER_PIN
	__gpio_as_output(MMC_POWER_PIN);
	__gpio_disable_pull(MMC_POWER_PIN);
	__gpio_set_pin(MMC_POWER_PIN);
#endif
#ifdef MMC_CD_PIN
	__gpio_as_input(MMC_CD_PIN);
	__gpio_enable_pull(MMC_CD_PIN);
#endif
#ifdef MMC_PW_PIN
	__gpio_as_input(MMC_PW_PIN);
	__gpio_disable_pull(MMC_PW_PIN);
#endif
}


#define MMC_POWER_OFF()				\
do {						\
      	__gpio_set_pin(MMC_POWER_PIN);		\
} while (0)

#define MMC_POWER_ON()				\
do {						\
      	__gpio_clear_pin(MMC_POWER_PIN);		\
} while (0)

#define MMC_INSERT_STATUS() __gpio_get_pin(MMC_CD_PIN)

#define MMC_RESET() __msc_reset()

#define MMC_IRQ_MASK()				\
do {						\
      	REG_MSC_IMASK = 0xffff;			\
      	REG_MSC_IREG = 0xffff;			\
} while (0)

/***********************************************************************
 *  MMC Events
 */
#define MMC_EVENT_NONE	        0x00	/* No events */
#define MMC_EVENT_RX_DATA_DONE	0x01	/* Rx data done */
#define MMC_EVENT_TX_DATA_DONE	0x02	/* Tx data done */
#define MMC_EVENT_PROG_DONE	0x04	/* Programming is done */

static int use_4bit;		/* Use 4-bit data bus */
extern int num_6;
extern int sd2_0;

/* Stop the MMC clock and wait while it happens */
static inline int jz_mmc_stop_clock(void)
{
	int timeout = 1000;

	DEBUG(3, "stop MMC clock\n");
	REG_MSC_STRPCL = MSC_STRPCL_CLOCK_CONTROL_STOP;

	while (timeout && (REG_MSC_STAT & MSC_STAT_CLK_EN)) {
		timeout--;
		if (timeout == 0) {
			DEBUG(3, "Timeout on stop clock waiting\n");
			return MMC_ERROR_TIMEOUT;
		}
		udelay(1);
	}
	DEBUG(3, "clock off time is %d microsec\n", timeout);
	return MMC_NO_ERROR;
}

/* Start the MMC clock and operation */
static inline int jz_mmc_start_clock(void)
{
	REG_MSC_STRPCL =
	    MSC_STRPCL_CLOCK_CONTROL_START | MSC_STRPCL_START_OP;
	return MMC_NO_ERROR;
}

static inline unsigned int jz_mmc_calc_clkrt(int is_sd, unsigned int rate)
{
	unsigned int clkrt;
	unsigned int clk_src = is_sd ? 24000000 : 20000000;

	clkrt = 0;
	while (rate < clk_src) {
		clkrt++;
		clk_src >>= 1;
	}
	return clkrt;
}

static int jz_mmc_check_status(struct mmc_request *request)
{
	unsigned int status = REG_MSC_STAT;

	/* Checking for response or data timeout */
	if (status & (MSC_STAT_TIME_OUT_RES | MSC_STAT_TIME_OUT_READ)) {
		printf("MMC/SD timeout, MMC_STAT 0x%x CMD %d\n", status,
		       request->cmd);
		return MMC_ERROR_TIMEOUT;
	}

	/* Checking for CRC error */
	if (status &
	    (MSC_STAT_CRC_READ_ERROR | MSC_STAT_CRC_WRITE_ERROR |
	     MSC_STAT_CRC_RES_ERR)) {
		printf("MMC/CD CRC error, MMC_STAT 0x%x\n", status);
		return MMC_ERROR_CRC;
	}

	return MMC_NO_ERROR;
}

/* Obtain response to the command and store it to response buffer */
static void jz_mmc_get_response(struct mmc_request *request)
{
	int i;
	unsigned char *buf;
	unsigned int data;

	DEBUG(3, "fetch response for request %d, cmd %d\n", request->rtype,
	      request->cmd);
	buf = request->response;
	request->result = MMC_NO_ERROR;

	switch (request->rtype) {
	case RESPONSE_R1:
	case RESPONSE_R1B:
	case RESPONSE_R6:
	case RESPONSE_R3:
	case RESPONSE_R4:
	case RESPONSE_R5:
		{
			data = REG_MSC_RES;
			buf[0] = (data >> 8) & 0xff;
			buf[1] = data & 0xff;
			data = REG_MSC_RES;
			buf[2] = (data >> 8) & 0xff;
			buf[3] = data & 0xff;
			data = REG_MSC_RES;
			buf[4] = data & 0xff;

			DEBUG(3,
			      "request %d, response [%02x %02x %02x %02x %02x]\n",
			      request->rtype, buf[0], buf[1], buf[2],
			      buf[3], buf[4]);
			break;
		}
	case RESPONSE_R2_CID:
	case RESPONSE_R2_CSD:
		{
			for (i = 0; i < 16; i += 2) {
				data = REG_MSC_RES;
				buf[i] = (data >> 8) & 0xff;
				buf[i + 1] = data & 0xff;
			}
			DEBUG(3, "request %d, response [", request->rtype);
#if CONFIG_MMC_DEBUG_VERBOSE > 2
			if (g_mmc_debug >= 3) {
				int n;
				for (n = 0; n < 17; n++)
					printk("%02x ", buf[n]);
				printk("]\n");
			}
#endif
			break;
		}
	case RESPONSE_NONE:
		DEBUG(3, "No response\n");
		break;

	default:
		DEBUG(3, "unhandled response type for request %d\n",
		      request->rtype);
		break;
	}
}

#if MMC_DMA_ENABLE

static int jz_mmc_receive_data_dma(struct mmc_request *req)
{
	int ch = RX_DMA_CHANNEL;
	unsigned int size = req->block_len * req->nob;
	unsigned char err = 0;

	/* flush dcache */
	dma_cache_wback_inv((unsigned long) req->buffer, size);
	/* setup dma channel */
    if(((unsigned long)req->buffer) & 0x3)
    {
        printf("ERROR %s: When use DMA, the source buffer address should be word aligned\n", __FUNCTION__);
        while(1);
    }
	REG_DMAC_DSAR(ch) = PHYSADDR(MSC_RXFIFO);	/* DMA source addr */
	REG_DMAC_DTAR(ch) = PHYSADDR((unsigned long) req->buffer);	/* DMA dest addr */
	REG_DMAC_DTCR(ch) = (size + 3) / 4;	/* DMA transfer count */
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_MSCIN;	/* DMA request type */

#if MMC_DMA_INTERRUPT
	REG_DMAC_DCMD(ch) =
	    DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
	    DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
//	OSSemPend(mmc_dma_rx_sem, 100, &err);
//    if(err == OS_NO_ERR)
//        REG_DMAC_DCCSR(ch) = 0;
    //Because there is no mechanism to exit when SD bus error, so wait until SD
    //  transfers done
    OSSemPend(mmc_dma_rx_sem, 0, &err);
    REG_DMAC_DCCSR(ch) = 0;
#else
	REG_DMAC_DCMD(ch) =
	    DMAC_DCMD_DAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
	    DMAC_DCMD_DS_32BIT;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
//	while (REG_DMAC_DTCR(ch));
    while (!__dmac_channel_transmit_end_detected(ch));
/* clear status and disable channel */
	REG_DMAC_DCCSR(ch) = 0;
#endif

#if MMC_DMA_INTERRUPT
	return (err != OS_NO_ERR);
#else
	return 0;
#endif
}

static int jz_mmc_transmit_data_dma(struct mmc_request *req)
{
	int ch = TX_DMA_CHANNEL;
	unsigned int size = req->block_len * req->nob;
	unsigned char err = 0;

printf("In DMA\n");
	/* flush dcache */
	dma_cache_wback_inv((unsigned long) req->buffer, size);
	/* setup dma channel */
//    if(((unsigned long)req->buffer) & 0x3)
//    {
//        printf("ERROR %s: When use DMA, the source buffer address should be word aligned\n", __FUNCTION__);
//        while(1);
//    }
	REG_DMAC_DSAR(ch) = PHYSADDR((unsigned long) req->buffer);	/* DMA source addr */
	REG_DMAC_DTAR(ch) = PHYSADDR(MSC_TXFIFO);	/* DMA dest addr */
	REG_DMAC_DTCR(ch) = (size + 3) / 4;	/* DMA transfer count */
	REG_DMAC_DRSR(ch) = DMAC_DRSR_RS_MSCOUT;	/* DMA request type */

#if MMC_DMA_INTERRUPT
	REG_DMAC_DCMD(ch) =
	    DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
	    DMAC_DCMD_DS_32BIT | DMAC_DCMD_TIE;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
//	OSSemPend(mmc_dma_tx_sem, 100, &err);
//    if(err == OS_NO_ERR)
//        REG_DMAC_DCCSR(ch) = 0;
    OSSemPend(mmc_dma_tx_sem, 0, &err);
    REG_DMAC_DCCSR(ch) = 0;
#else
	REG_DMAC_DCMD(ch) =
	    DMAC_DCMD_SAI | DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
	    DMAC_DCMD_DS_32BIT;
	REG_DMAC_DCCSR(ch) = DMAC_DCCSR_EN | DMAC_DCCSR_NDES;
	/* wait for dma completion */
	while (REG_DMAC_DTCR(ch));
	/* clear status and disable channel */
	REG_DMAC_DCCSR(ch) = 0;
#endif

#if MMC_DMA_INTERRUPT
	return (err != OS_NO_ERR);
#else
	return 0;
#endif
}

#endif				/* MMC_DMA_ENABLE */

static int jz_mmc_receive_data(struct mmc_request *req)
{
	unsigned int nob = req->nob;
	unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;	/* length in word */
	unsigned char *buf = req->buffer;
	unsigned int *wbuf = (unsigned int *) buf;
	unsigned int waligned = (((unsigned int) buf & 0x3) == 0);	/* word aligned ? */
	unsigned int stat, timeout, data, cnt;

	for (nob; nob >= 1; nob--) {
		timeout = 0x3FFFFFF;

		while (timeout) {
			timeout--;
			stat = REG_MSC_STAT;

			if (stat & MSC_STAT_TIME_OUT_READ)
				return MMC_ERROR_TIMEOUT;
			else if (stat & MSC_STAT_CRC_READ_ERROR)
				return MMC_ERROR_CRC;
			else if (!(stat & MSC_STAT_DATA_FIFO_EMPTY)
				 || (stat & MSC_STAT_DATA_FIFO_AFULL)) {
				/* Ready to read data */
				break;
			}

			udelay(1);
		}

		if (!timeout)
			return MMC_ERROR_TIMEOUT;

		/* Read data from RXFIFO. It could be FULL or PARTIAL FULL */
		//printf("Receice Data = %d\r\n", wblocklen);
		cnt = wblocklen;
		while (cnt) {
			data = REG_MSC_RXFIFO;
			if (waligned) {
				*wbuf++ = data;
			} else {
				*buf++ = (unsigned char) (data >> 0);
				*buf++ = (unsigned char) (data >> 8);
				*buf++ = (unsigned char) (data >> 16);
				*buf++ = (unsigned char) (data >> 24);
			}
			cnt--;
			while (cnt
			       && (REG_MSC_STAT &
				   MSC_STAT_DATA_FIFO_EMPTY));
		}
	}

	return MMC_NO_ERROR;
}

static int jz_mmc_transmit_data(struct mmc_request *req)
{
	unsigned int nob = req->nob;
	unsigned int wblocklen = (unsigned int) (req->block_len + 3) >> 2;	/* length in word */
	unsigned char *buf = req->buffer;
	unsigned int *wbuf = (unsigned int *)buf;
	unsigned int waligned = (((unsigned int) buf & 0x3) == 0);	/* word aligned ? */
	unsigned int stat, timeout, data, cnt;

	for (nob; nob >= 1; nob--) {
		timeout = 0x3FFFFFF;

		while (timeout) {
			timeout--;
			stat = REG_MSC_STAT;

			if (stat &
			    (MSC_STAT_CRC_WRITE_ERROR |
			     MSC_STAT_CRC_WRITE_ERROR_NOSTS))
				return MMC_ERROR_CRC;
			else if (!(stat & MSC_STAT_DATA_FIFO_FULL)) {
				/* Ready to write data */
				break;
			}

			udelay(1);
		}

		if (!timeout)
			return MMC_ERROR_TIMEOUT;

		/* Write data to TXFIFO */
		cnt = wblocklen;
		while (cnt) {
			while (REG_MSC_STAT & MSC_STAT_DATA_FIFO_FULL);

			if (waligned) {
				REG_MSC_TXFIFO = *wbuf++;
			} else {
				data = *buf++;
                data |= *buf++ << 8;
                data |= *buf++ << 16;
                data |= *buf++ << 24;
				REG_MSC_TXFIFO = data;
			}
			cnt--;
		}
	}

	return MMC_NO_ERROR;
}

/********************************************************************************************************************
** Name:	  int jz_mmc_exec_cmd()
** Function:      send command to the card, and get a response
** Input:	  struct mmc_request *req	: MMC/SD request
** Output:	  0:  right		>0:  error code
********************************************************************************************************************/
int jz_mmc_exec_cmd(struct mmc_request *request)
{
	unsigned int cmdat = 0, events = 0;
	int retval, timeout = 0x3fffff;
	unsigned char err = 0;
	/* Indicate we have no result yet */
	request->result = MMC_NO_RESPONSE;
	if (request->cmd == MMC_CIM_RESET) {
		/* On reset, 1-bit bus width */
		use_4bit = 0;

		/* Reset MMC/SD controller */
		__msc_reset();

		/* On reset, drop MMC clock down */
		jz_mmc_set_clock(0, MMC_CLOCK_SLOW);

		/* On reset, stop MMC clock */
		jz_mmc_stop_clock();
	}
	if (request->cmd == MMC_SEND_OP_COND) {
		DEBUG(3, "Have an MMC card\n");
		/* always use 1bit for MMC */
		use_4bit = 0;
	}
	if (request->cmd == SET_BUS_WIDTH) {
		if (request->arg == 0x2) {
			DEBUG(2, "Use 4-bit bus width\n");
			use_4bit = 1;
		} else {
			DEBUG(2, "Use 1-bit bus width\n");
			use_4bit = 0;
		}
	}

	/* stop clock */
	jz_mmc_stop_clock();

	/* mask all interrupts */
	//REG_MSC_IMASK = 0xffff;
	/* clear status */
	REG_MSC_IREG = 0xffff;
	/*open interrupt */
	REG_MSC_IMASK = (~7);
	/* use 4-bit bus width when possible */
	if (use_4bit)
		cmdat |= MSC_CMDAT_BUS_WIDTH_4BIT;

	/* Set command type and events */
	switch (request->cmd) {
		/* MMC core extra command */
	case MMC_CIM_RESET:
		cmdat |= MSC_CMDAT_INIT;	/* Initialization sequence sent prior to command */
		break;

		/* bc - broadcast - no response */
	case MMC_GO_IDLE_STATE:
	case MMC_SET_DSR:
		break;

		/* bcr - broadcast with response */
	case MMC_SEND_OP_COND:
	case MMC_ALL_SEND_CID:

	case MMC_GO_IRQ_STATE:
		break;

		/* adtc - addressed with data transfer */
	case MMC_READ_DAT_UNTIL_STOP:
	case MMC_READ_SINGLE_BLOCK:
	case MMC_READ_MULTIPLE_BLOCK:
	case SEND_SCR:
#if defined(MMC_DMA_ENABLE)
		cmdat |=
		    MSC_CMDAT_DATA_EN | MSC_CMDAT_READ | MSC_CMDAT_DMA_EN;
#else
		cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
		events = MMC_EVENT_RX_DATA_DONE;
		break;

	case 6:
		if (num_6 < 2) {

#if defined(MMC_DMA_ENABLE)
			cmdat |=
			    MSC_CMDAT_DATA_EN | MSC_CMDAT_READ |
			    MSC_CMDAT_DMA_EN;
#else
			cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_READ;
#endif
			events = MMC_EVENT_RX_DATA_DONE;
		}
		break;

	case MMC_WRITE_DAT_UNTIL_STOP:
	case MMC_WRITE_BLOCK:
	case MMC_WRITE_MULTIPLE_BLOCK:
	case MMC_PROGRAM_CID:
	case MMC_PROGRAM_CSD:
	case MMC_SEND_WRITE_PROT:
	case MMC_GEN_CMD:
	case MMC_LOCK_UNLOCK:
#if 0//defined(MMC_DMA_ENABLE)
		cmdat |=
		    MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE | MSC_CMDAT_DMA_EN;
#else
		cmdat |= MSC_CMDAT_DATA_EN | MSC_CMDAT_WRITE;
#endif
		events = MMC_EVENT_TX_DATA_DONE | MMC_EVENT_PROG_DONE;

		break;

	case MMC_STOP_TRANSMISSION:
		events = MMC_EVENT_PROG_DONE;
		break;

		/* ac - no data transfer */
	default:
		break;
	}

	/* Set response type */
	switch (request->rtype) {
	case RESPONSE_NONE:
		break;

	case RESPONSE_R1B:
		cmdat |= MSC_CMDAT_BUSY;
	 /*FALLTHRU*/ case RESPONSE_R1:
		cmdat |= MSC_CMDAT_RESPONSE_R1;
		break;
	case RESPONSE_R2_CID:
	case RESPONSE_R2_CSD:
		cmdat |= MSC_CMDAT_RESPONSE_R2;
		break;
	case RESPONSE_R3:
		cmdat |= MSC_CMDAT_RESPONSE_R3;
		break;
	case RESPONSE_R4:
		cmdat |= MSC_CMDAT_RESPONSE_R4;
		break;
	case RESPONSE_R5:
		cmdat |= MSC_CMDAT_RESPONSE_R5;
		break;
	case RESPONSE_R6:
		cmdat |= MSC_CMDAT_RESPONSE_R6;
		break;
	default:
		break;
	}

	/* Set command index */
	if (request->cmd == MMC_CIM_RESET) {
		REG_MSC_CMD = MMC_GO_IDLE_STATE;
	} else {
		REG_MSC_CMD = request->cmd;
	}

	/* Set argument */
	REG_MSC_ARG = request->arg;

	/* Set block length and nob */
	if (request->cmd == SEND_SCR) {	/* get SCR from DataFIFO */
		REG_MSC_BLKLEN = 8;
		REG_MSC_NOB = 1;
	} else {
		REG_MSC_BLKLEN = request->block_len;
		REG_MSC_NOB = request->nob;
	}

	/* Set command */
	REG_MSC_CMDAT = cmdat;

	DEBUG(1, "Send cmd %d cmdat: %x arg: %x resp %d\n", request->cmd,
	      cmdat, request->arg, request->rtype);

	/* Start MMC/SD clock and send command to card */
	jz_mmc_start_clock();

	/* Wait for command completion */
	__intc_unmask_irq(IRQ_MSC);
	OSSemPend(mmc_msc_irq_sem, 100, &err);
	while (timeout-- && !(REG_MSC_STAT & MSC_STAT_END_CMD_RES));


	if (timeout == 0)
		return MMC_ERROR_TIMEOUT;

	REG_MSC_IREG = MSC_IREG_END_CMD_RES;	/* clear flag */

	/* Check for status */
	retval = jz_mmc_check_status(request);
	if (retval) {
		return retval;
	}

	/* Complete command with no response */
	if (request->rtype == RESPONSE_NONE) {
		return MMC_NO_ERROR;
	}

	/* Get response */
	jz_mmc_get_response(request);

	/* Start data operation */
	if (events & (MMC_EVENT_RX_DATA_DONE | MMC_EVENT_TX_DATA_DONE)) {
		if (events & MMC_EVENT_RX_DATA_DONE) {
			if (request->cmd == SEND_SCR) {
				/* SD card returns SCR register as data. 
				   MMC core expect it in the response buffer, 
				   after normal response. */
				request->buffer =
				    (unsigned char *) ((unsigned int) request->response + 5);
			}
#if defined(MMC_DMA_ENABLE)
            if((unsigned int) request -> buffer & 0x3)
                jz_mmc_receive_data(request);
            else
    			jz_mmc_receive_data_dma(request);
#else
			jz_mmc_receive_data(request);
#endif
		}

		if (events & MMC_EVENT_TX_DATA_DONE) {
#if 0//defined(MMC_DMA_ENABLE)
            if((unsigned int) request -> buffer & 0x3)
    			jz_mmc_transmit_data(request);
            else
    			jz_mmc_transmit_data_dma(request);
#else
			jz_mmc_transmit_data(request);
#endif
		}
		__intc_unmask_irq(IRQ_MSC);
		OSSemPend(mmc_msc_irq_sem, 100, &err);
		/* Wait for Data Done */
		while (!(REG_MSC_IREG & MSC_IREG_DATA_TRAN_DONE));
		REG_MSC_IREG = MSC_IREG_DATA_TRAN_DONE;	/* clear status */
	}

	/* Wait for Prog Done event */
	if (events & MMC_EVENT_PROG_DONE) {

		__intc_unmask_irq(IRQ_MSC);
		OSSemPend(mmc_msc_irq_sem, 100, &err);
		while (!(REG_MSC_IREG & MSC_IREG_PRG_DONE));
		REG_MSC_IREG = MSC_IREG_PRG_DONE;	/* clear status */
	}

	/* Command completed */

	return MMC_NO_ERROR;	/* return successfully */
}

/*******************************************************************************************************************
** Name:	  int mmc_chkcardwp()
** Function:      check weather card is write protect
** Input:	  NULL
** Output:	  1: insert write protect	0: not write protect
********************************************************************************************************************/
int jz_mmc_chkcardwp(void)
{
	return 0;
}

/*******************************************************************************************************************
** Name:	  int mmc_chkcard()
** Function:      check weather card is insert entirely
** Input:	  NULL
** Output:	  1: insert entirely	0: not insert entirely
********************************************************************************************************************/
int jz_mmc_chkcard(void)
{
#if JZ4740_PAV_1_0
	return 1;
#endif
//printf("======================================== %d\n", MMC_INSERT_STATUS());
#if (JZ4740_PAV_1_0 == 0)
	if (MMC_INSERT_STATUS() == 0) {
		if (MMCTYPE == 1)
			return 1;	/* insert entirely */
		else
			return 0;	/* not microsd insert entirely */
	}
	else {
		if (MMCTYPE == 1)
			return 0;	/* not insert entirely */
		else
			return 1;	/* microsd insert entirely */
	}
#endif
}

/* Set the MMC clock frequency */
void jz_mmc_set_clock(int sd, unsigned int rate)
{
	int clkrt= 0;

	sd = sd ? 1 : 0;

	jz_mmc_stop_clock();

	if (sd2_0) {
        if(rate >= 48000000)
    		__cpm_select_msc_hs_clk(sd);	/* select clock source from CPM */
        else
    		__cpm_select_msc_clk(sd);	/* select clock source from CPM */
		REG_CPM_CPCCR |= CPM_CPCCR_CE;
		REG_MSC_CLKRT = 0;
	} else {
		__cpm_select_msc_clk(sd);	/* select clock source from CPM */
		REG_CPM_CPCCR |= CPM_CPCCR_CE;
		clkrt = jz_mmc_calc_clkrt(sd, rate);
		REG_MSC_CLKRT = clkrt;
	}
	dgprintf("set clock to %u Hz is_sd=%d clkrt=%d\n", rate, sd, clkrt);
}

#if MMC_DMA_INTERRUPT
void jz_mmc_tx_handler(unsigned int arg)
{
	if (__dmac_channel_address_error_detected(arg)) {
		printf("%s: DMAC address error.\n", __FUNCTION__);
		__dmac_channel_clear_address_error(arg);
	}
	if (__dmac_channel_transmit_end_detected(arg)) {
		__dmac_channel_clear_transmit_end(arg);
//        __dmac_disable_channel(arg);                //Disable DMA channel
		OSSemPost(mmc_dma_tx_sem);
	}
}
void jz_mmc_rx_handler(unsigned int arg)
{
	if (__dmac_channel_address_error_detected(arg)) {
		printf("%s: DMAC address error.\n", __FUNCTION__);
		__dmac_channel_clear_address_error(arg);
	}
	if (__dmac_channel_transmit_end_detected(arg)) {
		__dmac_channel_clear_transmit_end(arg);
//        __dmac_disable_channel(arg);                //Disable DMA channel
		OSSemPost(mmc_dma_rx_sem);
	}
}
void jz_mmc_irq_handler(unsigned int arg)
{
	__intc_mask_irq(IRQ_MSC);
	OSSemPost(mmc_msc_irq_sem);
}
#endif
/*******************************************************************************************************************
** Name:	  void mmc_hardware_init()
** Function:      initialize the hardware condiction that access sd card
** Input:	  NULL
** Output:	  NULL
********************************************************************************************************************/
static int init_jz_mmc_hardware_init = 0;
void jz_mmc_hardware_init(void)
{

	if(init_jz_mmc_hardware_init) return;
    init_jz_mmc_hardware_init = 1;
    MMC_INIT_GPIO();	/* init GPIO */
#ifdef MMC_POWER_PIN
	MMC_POWER_ON();		/* turn on power of card */
#endif
	MMC_RESET();		/* reset mmc/sd controller */
	MMC_IRQ_MASK();		/* mask all IRQs */
	jz_mmc_stop_clock();	/* stop MMC/SD clock */
#if defined(MMC_DMA_ENABLE)
	__cpm_start_dmac();
	__dmac_enable_module();
//      REG_DMAC_DMACR = DMAC_DMACR_DME;
#if MMC_DMA_INTERRUPT
	mmc_dma_rx_sem = OSSemCreate(0);
	mmc_dma_tx_sem = OSSemCreate(0);
	request_irq(IRQ_DMA_0 + RX_DMA_CHANNEL, jz_mmc_rx_handler,
		    RX_DMA_CHANNEL);
	request_irq(IRQ_DMA_0 + TX_DMA_CHANNEL, jz_mmc_tx_handler,
		    TX_DMA_CHANNEL);
	mmc_msc_irq_sem = OSSemCreate(0);
	request_irq(IRQ_MSC, jz_mmc_irq_handler, 0);
#endif
#endif
}

#if (DM==1)
void mmc_poweron(void)
{
}
void mmc_poweroff(void)
{
}
void mmc_convert(void)
{
	__cpm_select_msc_clk(1);
	//     MMCTest();
}

void mng_init_mmc(void)
{
	struct dm_jz4740_t mmc_dm;
	mmc_dm.name = "MMC driver";
	mmc_dm.init = jz_mmc_hardware_init;
	mmc_dm.poweron = mmc_poweron;
	mmc_dm.poweroff = mmc_poweroff;
	mmc_dm.convert = mmc_convert;
	dm_register(1, &mmc_dm);
}
#endif
