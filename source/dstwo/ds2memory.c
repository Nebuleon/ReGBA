/* Per-platform code - ReGBA on DSTwo
 *
 * Copyright (C) 2013 GBATemp user Nebuleon
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
#include <sys/mman.h>

static FILE_TAG_TYPE MappedFile = FILE_TAG_INVALID;
static size_t MappedFileSize;

uint8_t* ReGBA_MapEntireROM(FILE_TAG_TYPE File, size_t Size)
{
	// The DSTwo cannot do this.
	return NULL;
}

void ReGBA_UnmapEntireROM(void* Mapping)
{
}

uint8_t* ReGBA_AllocateROM(size_t Size)
{
	uint8_t* Result = malloc(Size);
#if TRACE_MEMORY
	if (Result != NULL)
		ReGBA_Trace("I: Allocated space for a %u-byte ROM buffer", Size);
#endif
	return Result;
}

size_t ReGBA_AllocateOnDemandBuffer(void** Buffer)
{
	/* At the end of this function, the ROM buffer will have been sized so
	 * that at least 2 MiB are left for other operations.
	 */
	void* Result = NULL;

	/* Start with trying to get 10 MiB. Let go in 1 MiB increments. */
	size_t Size = 10 * 1024 * 1024;
	Result = malloc(Size);
	while (Result == NULL)
	{
		Size -= 1024 * 1024;
		Result = malloc(Size);
	}

	/* Now free the allocation and allocate it again, minus 2 MiB. */
	free(Result);
	Size -= 2 * 1024 * 1024;
	Result = malloc(Size);

#if TRACE_MEMORY
	ReGBA_Trace("I: Allocated space for a %u-byte on-demand buffer",
		Size);
#endif

	*Buffer = Result;
	return Size;
}

void ReGBA_DeallocateROM(void* Buffer)
{
	free(Buffer);
#if TRACE_MEMORY
	ReGBA_Trace("I: Deallocated space for the previous buffer");
#endif
}
