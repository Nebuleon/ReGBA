/* unofficial gameplaySP kai
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * Copyright (C) 2006 SiberianSTAR
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

#include "common.h"

#define ZIP_BUFFER_SIZE (64 * 1024) // 64KB
#define FILE_BUFFER_SIZE (32 * 1024) // 32 KiB, one ReGBA page's worth
//unsigned char zip_buff[ZIP_BUFFER_SIZE];

struct SZIPFileDataDescriptor
{
  int32_t CRC32;
  int32_t CompressedSize;
  int32_t UncompressedSize;
} __attribute__((packed));

struct SZIPFileHeader
{
  uint8_t Sig[4]; // EDIT: Used to be s32 Sig;
  int16_t VersionToExtract;
  int16_t GeneralBitFlag;
  int16_t CompressionMethod;
  int16_t LastModFileTime;
  int16_t LastModFileDate;
  struct SZIPFileDataDescriptor DataDescriptor;
  int16_t FilenameLength;
  int16_t ExtraFieldLength;
}  __attribute__((packed));


void* zip_malloc_func(void* opaque, unsigned int items, unsigned int size)
{
    return calloc(size, items);
}

void zip_free_func(void* opaque, void* address)
{
    free(address);
}

// ZIPで圧縮されたRONのロード
// 返り値:-2=一時ファイルを作成/-1=エラー/その他=ROMのサイズ
// もし、ROMのサイズ>ROMバッファのサイズ の場合は一時ファイルを作成

// TODO Support big-endian systems.
// The ZIP file header contains little-endian fields, and byte swapping is
// needed on big-endian systems.
ssize_t load_file_zip(const char *filename, uint8_t** ROMBuffer)
{
	struct SZIPFileHeader data;
	char tmp[MAX_PATH];
	ssize_t retval = -1;
	uint8_t *cbuffer;
	char *ext;
	FILE_TAG_TYPE fd;
	FILE_TAG_TYPE tmp_fd = FILE_TAG_INVALID;
	bool WriteExternalFile = false;
	uint8_t* Buffer = NULL;

	cbuffer = (uint8_t*) malloc(ZIP_BUFFER_SIZE);
	if(cbuffer == NULL)
		return -1;

	FILE_OPEN(fd, filename, READ);

	if (!FILE_CHECK_VALID(fd))
	{
		free(cbuffer);
		return -1;
	}

	{
		FILE_READ(fd, &data, sizeof(struct SZIPFileHeader));

		FILE_READ(fd, tmp, data.FilenameLength);
		tmp[data.FilenameLength] = 0; // end string

		if (data.ExtraFieldLength)
			FILE_SEEK(fd, data.ExtraFieldLength, SEEK_CUR);

		if (data.GeneralBitFlag & 0x0008)
		{
			FILE_READ(fd, &data.DataDescriptor, sizeof(struct SZIPFileDataDescriptor));
		}

		ext = strrchr(tmp, '.');

		Buffer = ReGBA_AllocateROM(data.DataDescriptor.UncompressedSize);
		// Is the decompressed file too big to fit in a buffer, according to
		// the port?
		if (Buffer == NULL)
		{
			WriteExternalFile = true;
			sprintf(tmp, "%s/%s", main_path, ZIP_TMP);
			FILE_OPEN(tmp_fd, tmp, WRITE);
			// Allocate a small buffer to write the external file with.
			Buffer = (uint8_t*) malloc(FILE_BUFFER_SIZE);
			if (Buffer == NULL)
			{
				retval = -1;
				goto outcode;
			}
		}
		else
		{
			*ROMBuffer = Buffer;
		}

		ReGBA_ProgressInitialise(WriteExternalFile
			? FILE_ACTION_DECOMPRESS_ROM_TO_FILE
			: FILE_ACTION_DECOMPRESS_ROM_TO_RAM
			);

		if(ext && (strcasecmp(ext, ".bin") == 0 || strcasecmp(ext, ".gba") == 0))
		{
			switch (data.CompressionMethod)
			{
				case 0: // No compression
					retval = data.DataDescriptor.UncompressedSize;
					if (!WriteExternalFile)
					{
						FILE_READ(fd, Buffer, retval);
						ReGBA_ProgressUpdate(data.DataDescriptor.UncompressedSize, data.DataDescriptor.UncompressedSize);
					}
					else
					{
						while (retval > 0)
						{
							int bytes_to_read = ZIP_BUFFER_SIZE;
							if (retval < bytes_to_read) bytes_to_read = retval;
							FILE_READ(fd, cbuffer, ZIP_BUFFER_SIZE);
							FILE_WRITE(tmp_fd, cbuffer, ZIP_BUFFER_SIZE);
							retval -= bytes_to_read;
							ReGBA_ProgressUpdate(data.DataDescriptor.UncompressedSize - retval, data.DataDescriptor.UncompressedSize);
						}
						retval = -2;
					}

					goto outcode;

				case 8: // Deflate compression
				{
					z_stream stream = {0};
					int32_t err;

					/*
					 * z.next_in   = Input pointer
					 * z.avail_in  = Remaining bytes of buffered input data
					 * z.next_out  = Output pointer
					 * z.avail_out = Remaining bytes of bufferable output data
					 */

					stream.next_in = (Bytef*) cbuffer;
					stream.avail_in = (uint32_t) ZIP_BUFFER_SIZE;
					stream.next_out = (Bytef*) Buffer;

					if (!WriteExternalFile)
					{
						stream.avail_out = data.DataDescriptor.UncompressedSize;
						retval = (uint32_t)data.DataDescriptor.UncompressedSize;
					}
					else
					{
						stream.avail_out = FILE_BUFFER_SIZE;
						retval = -2;
					}

					stream.opaque = (voidpf) 0;

					stream.zalloc = zip_malloc_func;
					stream.zfree = zip_free_func;

					err = inflateInit2(&stream, -MAX_WBITS);

					size_t Done = 0;

					FILE_READ(fd, cbuffer, ZIP_BUFFER_SIZE);

					if (err == Z_OK)
					{
						while (err != Z_STREAM_END)
						{
							err = inflate(&stream, Z_SYNC_FLUSH);
							if (err == Z_BUF_ERROR)
							{
								stream.avail_in = (uint32_t) ZIP_BUFFER_SIZE;
								stream.next_in = (Bytef*) cbuffer;
								FILE_READ(fd, cbuffer, ZIP_BUFFER_SIZE);
							}
							else if (err < 0)
							{
								retval = -1;
								goto outcode;
							}

							if (WriteExternalFile && stream.avail_out == 0)
							{
								FILE_WRITE(tmp_fd, Buffer, FILE_BUFFER_SIZE);
								Done += FILE_BUFFER_SIZE;
								ReGBA_ProgressUpdate(Done, data.DataDescriptor.UncompressedSize);
								stream.next_out = Buffer;
								stream.avail_out = FILE_BUFFER_SIZE;
							}
							else if (!WriteExternalFile)
							{
								ReGBA_ProgressUpdate(data.DataDescriptor.UncompressedSize - stream.avail_out, data.DataDescriptor.UncompressedSize);
							}
						}

						if (WriteExternalFile && FILE_BUFFER_SIZE - stream.avail_out != 0)
							FILE_WRITE(tmp_fd, Buffer, FILE_BUFFER_SIZE - stream.avail_out);
						ReGBA_ProgressUpdate(data.DataDescriptor.UncompressedSize, data.DataDescriptor.UncompressedSize);

						err = Z_OK;
						inflateEnd(&stream);
					}
					goto outcode;
				}
			}
		}
	}
   
outcode:
	FILE_CLOSE(fd);

	if(WriteExternalFile)
	{
		FILE_CLOSE(tmp_fd);
		free(Buffer);
	}

	free(cbuffer);
	ReGBA_ProgressFinalise();

	return retval;
}
