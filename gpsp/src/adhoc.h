/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 NJ
 * Copyright (C) 2007 takka <takka@tfact.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ADHOC_H
#define ADHOC_H

#define ADHOC_DATATYPE_ANY    0xff
#define ADHOC_DATATYPE_ACK    0x01
#define ADHOC_DATATYPE_SYNC   0x02
#define ADHOC_DATATYPE_INPUT  0x04
#define ADHOC_DATATYPE_STATE  0x08
#define ADHOC_DATATYPE_USER1  0x10
#define ADHOC_DATATYPE_USER2  0x20
#define ADHOC_DATATYPE_USER3  0x40
#define ADHOC_DATATYPE_USER4  0x80

// データの最小単位
// 各データのサイズは異なるサイズになるように設定した方が安全
// データサイズが0x400を超える場合は0x400に設定
#define ADHOC_DATASIZE_ACK    (1 + sizeof(int))
#define ADHOC_DATASIZE_SYNC   (1 + sizeof(unsigned char))
#define ADHOC_DATASIZE_INPUT  (1 + sizeof(ADHOC_DATA))
#define ADHOC_DATASIZE_STATE  (1 + 0x3ff)
#define ADHOC_DATASIZE_USER1  (1)
#define ADHOC_DATASIZE_USER2  (1)
#define ADHOC_DATASIZE_USER3  (1)
#define ADHOC_DATASIZE_USER4  (1)

#define MULTI_DATASIZE        (4)
#define MULTI_NOP   0x00
#define MULTI_START 0x01
#define MULTI_END   0x02
#define MULTI_SEND  0x10
#define MULTI_RECV  0x20
#define MULTI_KILL  0xFF

/******************************************************************************
 * グローバル関数の宣言
 ******************************************************************************/
u32 load_adhoc_modules(void);

u32 adhoc_init(const char *matchingData);
u32 adhoc_term(void);
u32 adhoc_select(void);
u32 adhocReconnect(char *ssid);

u32 adhocSend(void *buffer, u32 length, u32 type);
u32 adhocRecv(void *buffer, u32 length, u32 type);

u32 adhocSendBlocking(void *buffer, u32 length);
u32 adhocRecvBlocking(void *buffer, u32 length);
u32 adhocRecvBlockingTimeout(void *buffer, u32 length, u32 timeout);

int adhocSendRecvAck(void *buffer, int length, int timeout, int type);
int adhocRecvSendAck(void *buffer, int length, int timeout, int type);

void adhoc_exit();

extern u32 g_multi_id;
extern u32 g_adhoc_transfer_flag;
extern u32 g_adhoc_link_flag;

#endif
