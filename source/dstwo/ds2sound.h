/* ReGBA DSTwo - Audio output
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2007 takka <takka@tfact.net>
 * Copyright (C) 2013 GBAtemp user Nebuleon
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

#ifndef _DS2SOUND_H_
#define _DS2SOUND_H_

// OUTPUT_SOUND_FREQUENCY should be a power-of-2 fraction of SOUND_FREQUENCY;
// if not, ds2sound.c's ReGBA_AudioUpdate() needs to resample the output.
#define OUTPUT_SOUND_FREQUENCY 44100

/* These are in units of OUTPUT_SOUND_FREQUENCY. */
#define OUTPUT_SOUND_LEN 4096
#define OUTPUT_SOUND_LOW_LEN 1024
#define OUTPUT_SOUND_HIGH_LEN 3072
#define OUTPUT_SOUND_FFWD_LEN 2048

/* At least this many samples must be submitted at once to the Nintendo DS. */
#define SOUND_SEND_LEN 32

#define OUTPUT_FREQUENCY_DIVISOR ((int) (SOUND_FREQUENCY) / (OUTPUT_SOUND_FREQUENCY))

#endif
