/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
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

/******************************************************************************
 * main.h
 * メインヘッダ
 ******************************************************************************/
#ifndef MAIN_H
#define MAIN_H

#include "message.h"

#define LANGUAGE_PACK   "SYSTEM/language.msg"

extern uint32_t execute_cycles;
extern uint32_t to_skip;
extern uint32_t skip_next_frame_flag;
extern uint32_t prescale_table[];
extern uint32_t frame_ticks;
extern uint32_t frame_interval; // For in-memory saved states used in rewinding

extern uint32_t fast_backward;

uint32_t update_gba();
void reset_gba();
void quit();
void main_read_mem_savestate();
void main_write_mem_savestate();
extern int gpsp_main(int argc, char **argv);

#endif /* MAIN_H */

