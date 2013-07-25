/* Serial port output utilities - gpSP on DSTwo
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
#include <stdarg.h>

void serial_printf(const char* fmt, ...)
{
	char* line = malloc(82);
	va_list args;
	int linelen;

	va_start(args, fmt);
	if ((linelen = vsnprintf(line, 82, fmt, args)) >= 82)
	{
		va_end(args);
		va_start(args, fmt);
		free(line);
		line = malloc(linelen + 1);
		vsnprintf(line, linelen + 1, fmt, args);
	}
	serial_puts(line);
	va_end(args);
	free(line);
	serial_putc('\r');
	serial_putc('\n');
}

void serial_timestamp_printf(const char* fmt, ...)
{
	{
		char timestamp[14];
		unsigned int Now = getSysTime();
		sprintf(timestamp, "%6u.%03u", Now / 23437, (Now % 23437) * 16 / 375);
		serial_puts(timestamp);
	}
	serial_putc(':');
	serial_putc(' ');

	char* line = malloc(82);
	va_list args;
	int linelen;

	va_start(args, fmt);
	if ((linelen = vsnprintf(line, 82, fmt, args)) >= 82)
	{
		va_end(args);
		va_start(args, fmt);
		free(line);
		line = malloc(linelen + 1);
		vsnprintf(line, linelen + 1, fmt, args);
	}
	serial_puts(line);
	va_end(args);
	free(line);
	serial_putc('\r');
	serial_putc('\n');
}
