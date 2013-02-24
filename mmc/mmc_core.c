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
File        : mmc_core.c
Purpose     : MMC core routines.

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

/**************************************************************************
 * Debugging functions
 **************************************************************************/
int sd2_0 = 0;
static char *mmc_result_strings[] = {
	"NO_RESPONSE",
	"NO_ERROR",
	"ERROR_OUT_OF_RANGE",
	"ERROR_ADDRESS",
	"ERROR_BLOCK_LEN",
	"ERROR_ERASE_SEQ",
	"ERROR_ERASE_PARAM",
	"ERROR_WP_VIOLATION",
	"ERROR_CARD_IS_LOCKED",
	"ERROR_LOCK_UNLOCK_FAILED",
	"ERROR_COM_CRC",
	"ERROR_ILLEGAL_COMMAND",
	"ERROR_CARD_ECC_FAILED",
	"ERROR_CC",
	"ERROR_GENERAL",
	"ERROR_UNDERRUN",
	"ERROR_OVERRUN",
	"ERROR_CID_CSD_OVERWRITE",
	"ERROR_STATE_MISMATCH",
	"ERROR_HEADER_MISMATCH",
	"ERROR_TIMEOUT",
	"ERROR_CRC",
	"ERROR_DRIVER_FAILURE",
};

char *mmc_result_to_string(int i)
{
	return mmc_result_strings[i + 1];
}

static char *card_state_strings[] = {
	"empty",
	"idle",
	"ready",
	"ident",
	"stby",
	"tran",
	"data",
	"rcv",
	"prg",
	"dis",
};

static inline char *card_state_to_string(int i)
{
	return card_state_strings[i + 1];
}

/**************************************************************************
 * Utility functions
 **************************************************************************/

#define PARSE_u32(_buf,_index) \
	(((unsigned int)_buf[_index]) << 24) | (((unsigned int)_buf[_index+1]) << 16) | \
        (((unsigned int)_buf[_index+2]) << 8) | ((unsigned int)_buf[_index+3]);

#define PARSE_u16(_buf,_index) \
	(((unsigned short)_buf[_index]) << 8) | ((unsigned short)_buf[_index+1]);

int mmc_unpack_csd(struct mmc_request *request, struct mmc_csd *csd)
{
	unsigned char *buf = request->response;
	int num = 0;
	if (request->result)
		return request->result;
	csd->csd_structure = (buf[1] & 0xc0) >> 6;
	if (csd->csd_structure)
		sd2_0 = 1;
	else
		sd2_0 = 0;
	switch (csd->csd_structure) {
	case 0:
		csd->taac = buf[2];
		csd->nsac = buf[3];
		csd->tran_speed = buf[4];
		csd->ccc = (((unsigned short) buf[5]) << 4) | ((buf[6] & 0xf0) >> 4);
		csd->read_bl_len = buf[6] & 0x0f;
		/* for support 2GB card */
		if (csd->read_bl_len >= 10) {
			num = csd->read_bl_len - 9;
			csd->read_bl_len = 9;
		}

		csd->read_bl_partial = (buf[7] & 0x80) ? 1 : 0;
		csd->write_blk_misalign = (buf[7] & 0x40) ? 1 : 0;
		csd->read_blk_misalign = (buf[7] & 0x20) ? 1 : 0;
		csd->dsr_imp = (buf[7] & 0x10) ? 1 : 0;
		csd->c_size =
		    ((((unsigned short) buf[7]) & 0x03) << 10) | (((unsigned short) buf[8]) << 2)
		    | (((unsigned short) buf[9]) & 0xc0) >> 6;

		if (num) {
			csd->c_size = csd->c_size << num;
		}

		csd->vdd_r_curr_min = (buf[9] & 0x38) >> 3;
		csd->vdd_r_curr_max = buf[9] & 0x07;
		csd->vdd_w_curr_min = (buf[10] & 0xe0) >> 5;
		csd->vdd_w_curr_max = (buf[10] & 0x1c) >> 2;
		csd->c_size_mult =
		    ((buf[10] & 0x03) << 1) | ((buf[11] & 0x80) >> 7);
		switch (csd->csd_structure) {
		case CSD_STRUCT_VER_1_0:
		case CSD_STRUCT_VER_1_1:
			csd->erase.v22.sector_size = (buf[11] & 0x7c) >> 2;
			csd->erase.v22.erase_grp_size =
			    ((buf[11] & 0x03) << 3) | ((buf[12] & 0xe0) >>
						       5);

			break;
		case CSD_STRUCT_VER_1_2:
		default:
			csd->erase.v31.erase_grp_size =
			    (buf[11] & 0x7c) >> 2;
			csd->erase.v31.erase_grp_mult =
			    ((buf[11] & 0x03) << 3) | ((buf[12] & 0xe0) >>
						       5);
			break;
		}
		csd->wp_grp_size = buf[12] & 0x1f;
		csd->wp_grp_enable = (buf[13] & 0x80) ? 1 : 0;
		csd->default_ecc = (buf[13] & 0x60) >> 5;
		csd->r2w_factor = (buf[13] & 0x1c) >> 2;
		csd->write_bl_len =
		    ((buf[13] & 0x03) << 2) | ((buf[14] & 0xc0) >> 6);
		if (csd->write_bl_len >= 10)
			csd->write_bl_len = 9;

		csd->write_bl_partial = (buf[14] & 0x20) ? 1 : 0;
		csd->file_format_grp = (buf[15] & 0x80) ? 1 : 0;
		csd->copy = (buf[15] & 0x40) ? 1 : 0;
		csd->perm_write_protect = (buf[15] & 0x20) ? 1 : 0;
		csd->tmp_write_protect = (buf[15] & 0x10) ? 1 : 0;
		csd->file_format = (buf[15] & 0x0c) >> 2;
		csd->ecc = buf[15] & 0x03;

		DEBUG(2,
		      "  csd_structure=%d  spec_vers=%d  taac=%02x  nsac=%02x  tran_speed=%02x\n"
		      "  ccc=%04x  read_bl_len=%d  read_bl_partial=%d  write_blk_misalign=%d\n"
		      "  read_blk_misalign=%d  dsr_imp=%d  c_size=%d  vdd_r_curr_min=%d\n"
		      "  vdd_r_curr_max=%d  vdd_w_curr_min=%d  vdd_w_curr_max=%d  c_size_mult=%d\n"
		      "  wp_grp_size=%d  wp_grp_enable=%d  default_ecc=%d  r2w_factor=%d\n"
		      "  write_bl_len=%d  write_bl_partial=%d  file_format_grp=%d  copy=%d\n"
		      "  perm_write_protect=%d  tmp_write_protect=%d  file_format=%d  ecc=%d\n",
		      csd->csd_structure, csd->spec_vers, csd->taac,
		      csd->nsac, csd->tran_speed, csd->ccc,
		      csd->read_bl_len, csd->read_bl_partial,
		      csd->write_blk_misalign, csd->read_blk_misalign,
		      csd->dsr_imp, csd->c_size, csd->vdd_r_curr_min,
		      csd->vdd_r_curr_max, csd->vdd_w_curr_min,
		      csd->vdd_w_curr_max, csd->c_size_mult,
		      csd->wp_grp_size, csd->wp_grp_enable,
		      csd->default_ecc, csd->r2w_factor, csd->write_bl_len,
		      csd->write_bl_partial, csd->file_format_grp,
		      csd->copy, csd->perm_write_protect,
		      csd->tmp_write_protect, csd->file_format, csd->ecc);
		switch (csd->csd_structure) {
		case CSD_STRUCT_VER_1_0:
		case CSD_STRUCT_VER_1_1:
			DEBUG(2, " V22 sector_size=%d erase_grp_size=%d\n",
			      csd->erase.v22.sector_size,
			      csd->erase.v22.erase_grp_size);
			break;
		case CSD_STRUCT_VER_1_2:
		default:
			DEBUG(2,
			      " V31 erase_grp_size=%d erase_grp_mult=%d\n",
			      csd->erase.v31.erase_grp_size,
			      csd->erase.v31.erase_grp_mult);
			break;

		}
		break;

	case 1:
		csd->taac = 0;
		csd->nsac = 0;
		csd->tran_speed = buf[4];
		csd->ccc = (((unsigned short) buf[5]) << 4) | ((buf[6] & 0xf0) >> 4);

		csd->read_bl_len = 9;
		csd->read_bl_partial = 0;
		csd->write_blk_misalign = 0;
		csd->read_blk_misalign = 0;
		csd->dsr_imp = (buf[7] & 0x10) ? 1 : 0;
		csd->c_size =
		    ((((unsigned short) buf[8]) & 0x3f) << 16) | (((unsigned short) buf[9]) << 8)
		    | ((unsigned short) buf[10]);
		switch (csd->csd_structure) {
		case CSD_STRUCT_VER_1_0:
		case CSD_STRUCT_VER_1_1:
			csd->erase.v22.sector_size = 0x7f;
			csd->erase.v22.erase_grp_size = 0;
			break;
		case CSD_STRUCT_VER_1_2:
		default:
			csd->erase.v31.erase_grp_size = 0x7f;
			csd->erase.v31.erase_grp_mult = 0;
			break;

		}
		csd->wp_grp_size = 0;
		csd->wp_grp_enable = 0;
		csd->default_ecc = (buf[13] & 0x60) >> 5;
		csd->r2w_factor = 4;	/* Unused */
		csd->write_bl_len = 9;

		csd->write_bl_partial = 0;
		csd->file_format_grp = 0;
		csd->copy = (buf[15] & 0x40) ? 1 : 0;
		csd->perm_write_protect = (buf[15] & 0x20) ? 1 : 0;
		csd->tmp_write_protect = (buf[15] & 0x10) ? 1 : 0;
		csd->file_format = 0;
		csd->ecc = buf[15] & 0x03;

		DEBUG(2,
		      "  csd_structure=%d  spec_vers=%d  taac=%02x  nsac=%02x  tran_speed=%02x\n"
		      "  ccc=%04x  read_bl_len=%d  read_bl_partial=%d  write_blk_misalign=%d\n"
		      "  read_blk_misalign=%d  dsr_imp=%d  c_size=%d  vdd_r_curr_min=%d\n"
		      "  vdd_r_curr_max=%d  vdd_w_curr_min=%d  vdd_w_curr_max=%d  c_size_mult=%d\n"
		      "  wp_grp_size=%d  wp_grp_enable=%d  default_ecc=%d  r2w_factor=%d\n"
		      "  write_bl_len=%d  write_bl_partial=%d  file_format_grp=%d  copy=%d\n"
		      "  perm_write_protect=%d  tmp_write_protect=%d  file_format=%d  ecc=%d\n",
		      csd->csd_structure, csd->spec_vers, csd->taac,
		      csd->nsac, csd->tran_speed, csd->ccc,
		      csd->read_bl_len, csd->read_bl_partial,
		      csd->write_blk_misalign, csd->read_blk_misalign,
		      csd->dsr_imp, csd->c_size, csd->vdd_r_curr_min,
		      csd->vdd_r_curr_max, csd->vdd_w_curr_min,
		      csd->vdd_w_curr_max, csd->c_size_mult,
		      csd->wp_grp_size, csd->wp_grp_enable,
		      csd->default_ecc, csd->r2w_factor, csd->write_bl_len,
		      csd->write_bl_partial, csd->file_format_grp,
		      csd->copy, csd->perm_write_protect,
		      csd->tmp_write_protect, csd->file_format, csd->ecc);
		switch (csd->csd_structure) {
		case CSD_STRUCT_VER_1_0:
		case CSD_STRUCT_VER_1_1:
			DEBUG(2, " V22 sector_size=%d erase_grp_size=%d\n",
			      csd->erase.v22.sector_size,
			      csd->erase.v22.erase_grp_size);
			break;
		case CSD_STRUCT_VER_1_2:
		default:
			DEBUG(2,
			      " V31 erase_grp_size=%d erase_grp_mult=%d\n",
			      csd->erase.v31.erase_grp_size,
			      csd->erase.v31.erase_grp_mult);
			break;
		}
	}

	if (buf[0] != 0x3f)
		return MMC_ERROR_HEADER_MISMATCH;

	return 0;
}

int mmc_unpack_r1(struct mmc_request *request, struct mmc_response_r1 *r1,
		  enum card_state state)
{
	unsigned char *buf = request->response;

	if (request->result)
		return request->result;

	r1->cmd = buf[0];
	r1->status = PARSE_u32(buf, 1);

	DEBUG(2, "mmc_unpack_r1: cmd=%d status=%08x\n", r1->cmd,
	      r1->status);
//printf("mmc_unpack_r1: cmd=%d status=%08x\n", r1->cmd,
//	      r1->status);

	if (R1_STATUS(r1->status)) {
		if (r1->status & R1_OUT_OF_RANGE)
			return MMC_ERROR_OUT_OF_RANGE;
		if (r1->status & R1_ADDRESS_ERROR)
			return MMC_ERROR_ADDRESS;
		if (r1->status & R1_BLOCK_LEN_ERROR)
			return MMC_ERROR_BLOCK_LEN;
		if (r1->status & R1_ERASE_SEQ_ERROR)
			return MMC_ERROR_ERASE_SEQ;
		if (r1->status & R1_ERASE_PARAM)
			return MMC_ERROR_ERASE_PARAM;
		if (r1->status & R1_WP_VIOLATION)
			return MMC_ERROR_WP_VIOLATION;
		//if ( r1->status & R1_CARD_IS_LOCKED )     return MMC_ERROR_CARD_IS_LOCKED;
		if (r1->status & R1_LOCK_UNLOCK_FAILED)
			return MMC_ERROR_LOCK_UNLOCK_FAILED;
		if (r1->status & R1_COM_CRC_ERROR)
			return MMC_ERROR_COM_CRC;
		if (r1->status & R1_ILLEGAL_COMMAND)
			return MMC_ERROR_ILLEGAL_COMMAND;
		if (r1->status & R1_CARD_ECC_FAILED)
			return MMC_ERROR_CARD_ECC_FAILED;
		if (r1->status & R1_CC_ERROR)
			return MMC_ERROR_CC;
		if (r1->status & R1_ERROR)
			return MMC_ERROR_GENERAL;
		if (r1->status & R1_UNDERRUN)
			return MMC_ERROR_UNDERRUN;
		if (r1->status & R1_OVERRUN)
			return MMC_ERROR_OVERRUN;
		if (r1->status & R1_CID_CSD_OVERWRITE)
			return MMC_ERROR_CID_CSD_OVERWRITE;
	}

	if (buf[0] != request->cmd)
		return MMC_ERROR_HEADER_MISMATCH;

	/* This should be last - it's the least dangerous error */
//      if ( R1_CURRENT_STATE(r1->status) != state ) return MMC_ERROR_STATE_MISMATCH;

	return 0;
}


int mmc_unpack_scr(struct mmc_request *request, struct mmc_response_r1 *r1,
		   enum card_state state, unsigned int * scr)
{
	unsigned char *buf = request->response;
	if (request->result)
		return request->result;

	*scr = PARSE_u32(buf, 5);	/* Save SCR returned by the SD Card */
	return mmc_unpack_r1(request, r1, state);

}

int mmc_unpack_r6(struct mmc_request *request, struct mmc_response_r1 *r1,
		  enum card_state state, int *rca)
{
	unsigned char *buf = request->response;

	if (request->result)
		return request->result;

	*rca = PARSE_u16(buf, 1);	/* Save RCA returned by the SD Card */

	*(buf + 1) = 0;
	*(buf + 2) = 0;

	return mmc_unpack_r1(request, r1, state);
}

int mmc_unpack_cid(struct mmc_request *request, struct mmc_cid *cid)
{
	unsigned char *buf = request->response;
	int i;

	if (request->result)
		return request->result;

	cid->mid = buf[1];
	cid->oid = PARSE_u16(buf, 2);
	for (i = 0; i < 6; i++)
		cid->pnm[i] = buf[4 + i];
	cid->pnm[6] = 0;
	cid->prv = buf[10];
	cid->psn = PARSE_u32(buf, 11);
	cid->mdt = buf[15];

	DEBUG(2,
	      "mmc_unpack_cid: mid=%d oid=%d pnm=%s prv=%d.%d psn=%08x mdt=%d/%d\n",
	      cid->mid, cid->oid, cid->pnm, (cid->prv >> 4),
	      (cid->prv & 0xf), cid->psn, (cid->mdt >> 4),
	      (cid->mdt & 0xf) + 1997);

	if (buf[0] != 0x3f)
		return MMC_ERROR_HEADER_MISMATCH;
	return 0;
}

int mmc_unpack_r3(struct mmc_request *request, struct mmc_response_r3 *r3)
{
	unsigned char *buf = request->response;

	if (request->result)
		return request->result;

	r3->ocr = PARSE_u32(buf, 1);
	DEBUG(2, "mmc_unpack_r3: ocr=%08x\n", r3->ocr);

	if (buf[0] != 0x3f)
		return MMC_ERROR_HEADER_MISMATCH;
	return 0;
}

/**************************************************************************/

#define KBPS 1
#define MBPS 1000

static unsigned int ts_exp[] =
    { 100 * KBPS, 1 * MBPS, 10 * MBPS, 100 * MBPS, 0, 0, 0, 0 };
static unsigned int ts_mul[] = { 0, 1000, 1200, 1300, 1500, 2000, 2500, 3000,
	3500, 4000, 4500, 5000, 5500, 6000, 7000, 8000
};

unsigned int mmc_tran_speed(unsigned char ts)
{
	unsigned int rate = ts_exp[(ts & 0x7)] * ts_mul[(ts & 0x78) >> 3];

	if (rate <= 0) {
		DEBUG(0,
		      "mmc_tran_speed: error - unrecognized speed 0x%02x\n",
		      ts);
		return 1;
	}

	return rate;
}

/**************************************************************************/

void mmc_send_cmd(struct mmc_request *request, int cmd, unsigned int arg,
		  unsigned short nob, unsigned short block_len, enum mmc_rsp_t rtype,
		  unsigned char * buffer)
{
	request->cmd = cmd;
	request->arg = arg;
	request->rtype = rtype;
	request->nob = nob;
	request->block_len = block_len;
	request->buffer = buffer;
	request->cnt = nob * block_len;
//printf("mmc_send_cmd: command = %d \r\n",cmd);
	jz_mmc_exec_cmd(request);
}
