/* ds2_main.c
 *
 * Copyright (C) 2010 dking <dking024@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
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

#include "common.h"

int ds2_main(void)
{
	HighFrequencyCPU();

	DS2_SetPixelFormat(DS_ENGINE_BOTH, DS2_PIXEL_FORMAT_BGR555);
	DS2_SetScreenSwap(true);

	if (DS2_StartAudio(OUTPUT_SOUND_FREQUENCY, OUTPUT_SOUND_LEN, true, true) != 0)
		goto _failure;

	gpsp_main (0, 0);

	return EXIT_SUCCESS;

_failure:
	return EXIT_FAILURE;
}
