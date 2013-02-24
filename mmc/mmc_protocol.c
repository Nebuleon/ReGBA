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
File        : mmc_protocol.c
Purpose     : MMC State machine functions.

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
extern int sd2_0;
int num_6 = 0;

static int mmc_sd_switch(struct mmc_request *request, int mode, int group,
			  unsigned char value, unsigned char * resp)
{
	unsigned int arg;
	struct mmc_response_r1 r1;

	mode = !!mode;
	value &= 0xF;
	arg = mode << 31 | 0x00FFFFFF;
	arg &= ~(0xF << (group * 4));
	arg |= value << (group * 4);
	mmc_send_cmd(request, 6, arg, 1, 64, RESPONSE_R1, resp);
	
	return 0;
}

/*
 * Fetches and decodes switch information
 */
static int  mmc_read_switch(struct mmc_request *request)
{
	unsigned int status[64 / 4];

	memset ((unsigned char *)status,0,64);
	mmc_sd_switch(request, 0, 0, 1, (unsigned char *) status);
	
        if (((unsigned char *)status)[13] & 0x02)
		return 0;
	else 
		 return 1;
}

/*
 * Test if the card supports high-speed mode and, if so, switch to it.
 */
static int mmc_switch_hs(struct mmc_request *request)
{
	unsigned int status[64 / 4];

	mmc_sd_switch(request, 1, 0, 1, (unsigned char *) status);
	return 0;
}

int mmc_select_card(void)
{
	struct mmc_request request;
	struct mmc_response_r1 r1;
	int retval;

	mmc_simple_cmd(&request, MMC_SELECT_CARD, mmcinfo.rca,
		       RESPONSE_R1B);
	retval = mmc_unpack_r1(&request, &r1, 0);
	if (retval) {
		return retval;
	}

	if (mmcinfo.sd) {
		if (sd2_0) {
			retval = mmc_read_switch(&request);
			if (!retval){
				mmc_switch_hs(&request);
				jz_mmc_set_clock(1,SD_CLOCK_HIGH);
			}
		}
		num_6 = 3;
		mmc_simple_cmd(&request, MMC_APP_CMD, mmcinfo.rca,
			       RESPONSE_R1);
		retval = mmc_unpack_r1(&request, &r1, 0);
		if (retval) {
			return retval;
		}
		mmc_simple_cmd(&request, SET_BUS_WIDTH, 2, RESPONSE_R1);
		retval = mmc_unpack_r1(&request, &r1, 0);
		if (retval) {
			return retval;
		}
	}
}

/******************************************************************
 *
 * Configure card
 *
 ******************************************************************/

static void mmc_configure_card(void)
{
	unsigned int rate;

	/* Get card info */

	if (sd2_0)
		mmcinfo.block_num = (mmcinfo.csd.c_size + 1) << 10;
	else
		mmcinfo.block_num =
		    (mmcinfo.csd.c_size +
		     1) * (1 << (mmcinfo.csd.c_size_mult + 2));

	mmcinfo.block_len = 1 << mmcinfo.csd.read_bl_len;
	/* Fix the clock rate */
	rate = mmc_tran_speed(mmcinfo.csd.tran_speed);
	if (rate < MMC_CLOCK_SLOW)
		rate = MMC_CLOCK_SLOW;
	if ((mmcinfo.sd == 0) && (rate > MMC_CLOCK_FAST))
		rate = MMC_CLOCK_FAST;
	if ((mmcinfo.sd) && (rate > SD_CLOCK_FAST))
		rate = SD_CLOCK_FAST;

	DEBUG(2, "mmc_configure_card: block_len=%d block_num=%d rate=%d\n",
	      mmcinfo.block_len, mmcinfo.block_num, rate);

	jz_mmc_set_clock(mmcinfo.sd, rate);
}

/******************************************************************
 *
 * State machine routines to initialize card(s)
 *
 ******************************************************************/

/*
  CIM_SINGLE_CARD_ACQ  (frequency at 400 kHz)
  --- Must enter from GO_IDLE_STATE ---
  1. SD_SEND_OP_COND (SD Card) [CMD55] + [CMD41]
  2. SEND_OP_COND (Full Range) [CMD1]   {optional}
  3. SEND_OP_COND (Set Range ) [CMD1]
     If busy, delay and repeat step 2
  4. ALL_SEND_CID              [CMD2]
     If timeout, set an error (no cards found)
  5. SET_RELATIVE_ADDR         [CMD3]
  6. SEND_CSD                  [CMD9]
  7. SET_DSR                   [CMD4]    Only call this if (csd.dsr_imp).
  8. Set clock frequency (check available in csd.tran_speed)
 */

#define MMC_INIT_DOING   0
#define MMC_INIT_PASSED  1
#define MMC_INIT_FAILED  2
//static int limit_41 = 0;
int limit_41 = 0;
static int mmc_init_card_state(struct mmc_request *request)
{
	struct mmc_response_r1 r1;
	struct mmc_response_r3 r3;
	int retval;
	int ocr = 0x40300000;

	DEBUG(2, "mmc_init_card_state\n");

	switch (request->cmd) {
	case MMC_GO_IDLE_STATE:	/* No response to parse */
		if (mmcinfo.sd)
			mmc_simple_cmd(request, 8, 0x1aa, RESPONSE_R1);
		else
			mmc_simple_cmd(request, MMC_SEND_OP_COND,
				       MMC_OCR_ARG, RESPONSE_R3);
		break;

	case 8:
		retval = mmc_unpack_r1(request, &r1, mmcinfo.state);
		mmc_simple_cmd(request, MMC_APP_CMD, 0, RESPONSE_R1);
		break;

	case MMC_APP_CMD:
		retval = mmc_unpack_r1(request, &r1, mmcinfo.state);
		if (retval & (limit_41 < 100)) {
			DEBUG(0,
			      "mmc_init_card_state: unable to MMC_APP_CMD error=%d (%s)\n",
			      retval, mmc_result_to_string(retval));
			limit_41++;
			mmc_simple_cmd(request, SD_SEND_OP_COND, ocr,
				       RESPONSE_R3);
		} else if (limit_41 < 100) {
			limit_41++;
			mmc_simple_cmd(request, SD_SEND_OP_COND, ocr,
				       RESPONSE_R3);
		} else {
			/* reset the card to idle */
			mmc_simple_cmd(request, MMC_GO_IDLE_STATE, 0,
				       RESPONSE_NONE);
			mmcinfo.sd = 0;
		}
		break;

	case SD_SEND_OP_COND:
		retval = mmc_unpack_r3(request, &r3);
		if (retval) {

			/* Try MMC card */
			mmc_simple_cmd(request, MMC_SEND_OP_COND,
				       MMC_OCR_ARG, RESPONSE_R3);
			break;
		}

		DEBUG(2, "mmc_init_card_state: read ocr value = 0x%08x\n",
		      r3.ocr);

		if (!(r3.ocr & MMC_CARD_BUSY || ocr == 0)) {
			mdelay(10);
			mmc_simple_cmd(request, MMC_APP_CMD, 0,
				       RESPONSE_R1);
		} else {
			/* Set the data bus width to 4 bits */
			mmcinfo.sd = 1;	/* SD Card ready */
			mmcinfo.state = CARD_STATE_READY;
			mmc_simple_cmd(request, MMC_ALL_SEND_CID, 0,
				       RESPONSE_R2_CID);
		}
		break;

	case MMC_SEND_OP_COND:
		retval = mmc_unpack_r3(request, &r3);
		if (retval) {
			DEBUG(0,
			      "mmc_init_card_state: failed SEND_OP_COND error=%d (%s)\n",
			      retval, mmc_result_to_string(retval));
			return MMC_INIT_FAILED;
		}

		DEBUG(2, "mmc_init_card_state: read ocr value = 0x%08x\n",
		      r3.ocr);
		if (!(r3.ocr & MMC_CARD_BUSY)) {
			mmc_simple_cmd(request, MMC_SEND_OP_COND,
				       MMC_OCR_ARG, RESPONSE_R3);
		} else {
			mmcinfo.sd = 0;	/* MMC Card ready */
			mmcinfo.state = CARD_STATE_READY;
			mmc_simple_cmd(request, MMC_ALL_SEND_CID, 0,
				       RESPONSE_R2_CID);
		}
		break;

	case MMC_ALL_SEND_CID:
		retval = mmc_unpack_cid(request, &mmcinfo.cid);

		/*FIXME:ignore CRC error for CMD2/CMD9/CMD10 */
		if (retval && (retval != MMC_ERROR_CRC)) {
			DEBUG(0,
			      "mmc_init_card_state: unable to ALL_SEND_CID error=%d (%s)\n",
			      retval, mmc_result_to_string(retval));
			return MMC_INIT_FAILED;
		}
		mmcinfo.state = CARD_STATE_IDENT;
		if (mmcinfo.sd)
			mmc_simple_cmd(request, MMC_SET_RELATIVE_ADDR, 0,
				       RESPONSE_R6);
		else
			mmc_simple_cmd(request, MMC_SET_RELATIVE_ADDR,
				       ID_TO_RCA(mmcinfo.id) << 16,
				       RESPONSE_R1);
		break;

	case MMC_SET_RELATIVE_ADDR:
		if (mmcinfo.sd) {
			retval =
			    mmc_unpack_r6(request, &r1, mmcinfo.state,
					  &mmcinfo.rca);
			mmcinfo.rca = mmcinfo.rca << 16;
			DEBUG(2,
			      "mmc_init_card_state: Get RCA from SD: 0x%04x Status: %x\n",
			      mmcinfo.rca, r1.status);
		} else {
			retval =
			    mmc_unpack_r1(request, &r1, mmcinfo.state);
			mmcinfo.rca = ID_TO_RCA(mmcinfo.id) << 16;
		}
		if (retval) {
			DEBUG(0,
			      "mmc_init_card_state: unable to SET_RELATIVE_ADDR error=%d (%s)\n",
			      retval, mmc_result_to_string(retval));
			return MMC_INIT_FAILED;
		}

		mmcinfo.state = CARD_STATE_STBY;
		mmc_simple_cmd(request, MMC_SEND_CSD, mmcinfo.rca,
			       RESPONSE_R2_CSD);

		break;
	case MMC_SEND_CSD:
		retval = mmc_unpack_csd(request, &mmcinfo.csd);

		/*FIXME:ignore CRC error for CMD2/CMD9/CMD10 */
		if (retval && (retval != MMC_ERROR_CRC)) {
			DEBUG(0,
			      "mmc_init_card_state: unable to SEND_CSD error=%d (%s)\n",
			      retval, mmc_result_to_string(retval));
			return MMC_INIT_FAILED;
		}
		if (mmcinfo.csd.dsr_imp) {
			DEBUG(0,
			      "mmc_init_card_state: driver doesn't support setting DSR\n");
		}
		mmc_configure_card();
		return MMC_INIT_PASSED;

		break;
	default:
		DEBUG(0,
		      "mmc_init_card_state: error!  Illegal last cmd %d\n",
		      request->cmd);
		return MMC_INIT_FAILED;
	}

	return MMC_INIT_DOING;
}

int mmc_init_card(void)
{
	struct mmc_request request;
	struct mmc_response_r1 r1;
	int retval;

	mmc_simple_cmd(&request, MMC_CIM_RESET, 0, RESPONSE_NONE);	/* reset card */
	mmc_simple_cmd(&request, MMC_GO_IDLE_STATE, 0, RESPONSE_NONE);
	mmcinfo.sd = 1;		/* assuming a SD card */

	while ((retval = mmc_init_card_state(&request)) == MMC_INIT_DOING);

	if (retval == MMC_INIT_PASSED)
		return MMC_NO_ERROR;
	else
		return MMC_NO_RESPONSE;
}
