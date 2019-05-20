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

#include "common.h" 

// TODO:PSP-1000はフレームバッファは256KBあれば足りるので、VRAMを使用する
FULLY_UNINITIALIZED(uint8_t savestate_write_buffer[SAVESTATE_SIZE])
  __attribute__ ((aligned (4)));
uint8_t *g_state_buffer_ptr;

#define SAVESTATE_REWIND_SIZE (SAVESTATE_REWIND_LEN*SAVESTATE_REWIND_NUM)	//~5MB
#define SAVESTATE_REWIND_LEN (0x69040)
#define SAVESTATE_REWIND_NUM (10)
FULLY_UNINITIALIZED(uint8_t SAVESTATE_REWIND_MEM[SAVESTATE_REWIND_SIZE])
  __attribute__ ((aligned (4)));

const uint8_t SVS_HEADER_E[SVS_HEADER_SIZE] = {'N', 'G', 'B', 'A', 'R', 'T', 'S',
  '1', '.', '0', 'e'}; // 1.0e is written with sound frequency-dependent
  // variables precalculated with SOUND_FREQUENCY = 65536. 1.0f is written
  // with sound frequency-dependent variables precalculated with it at 88200.
const uint8_t SVS_HEADER_F[SVS_HEADER_SIZE] = {'N', 'G', 'B', 'A', 'R', 'T', 'S',
  '1', '.', '0', 'f'};

typedef enum
{
  FLASH_DEVICE_MACRONIX_64KB   = 0x1C,
  FLASH_DEVICE_AMTEL_64KB      = 0x3D,
  FLASH_DEVICE_SST_64K         = 0xD4,
  FLASH_DEVICE_PANASONIC_64KB  = 0x1B,
  FLASH_DEVICE_MACRONIX_128KB  = 0x09
} FLASH_DEVICE_ID_TYPE;

typedef enum
{
  FLASH_MANUFACTURER_MACRONIX  = 0xC2,
  FLASH_MANUFACTURER_AMTEL     = 0x1F,
  FLASH_MANUFACTURER_PANASONIC = 0x32,
  FLASH_MANUFACTURER_SST       = 0xBF
} FLASH_MANUFACTURER_ID_TYPE;


// 関数宣言

void memory_read_mem_savestate();
void memory_write_mem_savestate();
uint8_t read_backup(uint32_t address);
void write_eeprom(uint32_t address, uint32_t value);
uint32_t read_eeprom();
CPU_ALERT_TYPE write_io_register8(uint32_t address, uint32_t value);
CPU_ALERT_TYPE write_io_register16(uint32_t address, uint32_t value);
CPU_ALERT_TYPE write_io_register32(uint32_t address, uint32_t value);
void write_backup(uint32_t address, uint32_t value);
uint32_t encode_bcd(uint8_t value);
void write_rtc(uint32_t address, uint32_t value);
uint32_t save_backup();
int32_t parse_config_line(char *current_line, char *current_variable, char *current_value);
int32_t load_game_config(char *gamepak_title, char *gamepak_code, char *gamepak_maker);
char *skip_spaces(char *line_ptr);
static int32_t load_gamepak_raw(const char* name);
uint32_t evict_gamepak_page();
void init_memory_gamepak();

// SIO
uint32_t get_sio_mode(uint16_t io1, uint16_t io2);
uint32_t send_adhoc_multi();

uint32_t g_multi_mode;


#define SIO_NORMAL8     0
#define SIO_NORMAL32    1
#define SIO_MULTIPLAYER 2
#define SIO_UART        3
#define SIO_JOYBUS      4
#define SIO_GP          5

// This table is configured for sequential access on system defaults
#ifdef OLD_COUNT
uint32_t waitstate_cycles_sequential[16][3] =
{
  { 1, 1, 1 }, // BIOS
  { 1, 1, 1 }, // Invalid
  { 3, 3, 6 }, // EWRAM (default settings)
  { 1, 1, 1 }, // IWRAM
  { 1, 1, 1 }, // IO Registers
  { 1, 1, 2 }, // Palette RAM
  { 1, 1, 2 }, // VRAM
  { 1, 1, 2 }, // OAM
  { 3, 3, 6 }, // Gamepak (wait 0)
  { 3, 3, 6 }, // Gamepak (wait 0)
  { 5, 5, 9 }, // Gamepak (wait 1)
  { 5, 5, 9 }, // Gamepak (wait 1)
  { 9, 9, 17 }, // Gamepak (wait 2)
  { 9, 9, 17 }  // Gamepak (wait 2)
};

uint32_t gamepak_waitstate_sequential[2][3][3] =
{
  {
    { 3, 3, 6 },
    { 5, 5, 9 },
    { 9, 9, 17 }
  },
  {
    { 2, 2, 3 },
    { 2, 2, 3 },
    { 2, 2, 3 }
  }
};
#else
uint8_t waitstate_cycles_seq[2][16] =
{
 /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
  { 1, 1, 3, 1, 1, 1, 1, 1, 3, 3, 5, 5, 9, 9, 5, 1 }, /* 8,16bit */
  { 1, 1, 6, 1, 1, 2, 2, 1, 5, 5, 9, 9,17,17, 1, 1 }  /* 32bit */
};

uint8_t waitstate_cycles_non_seq[2][16] =
{
 /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
  { 1, 1, 3, 1, 1, 1, 1, 1, 5, 5, 5, 5, 5, 5, 5, 1 }, /* 8,16bit */
  { 1, 1, 6, 1, 1, 2, 2, 1, 7, 7, 9, 9,13,13, 1, 1 }  /* 32bit */
};

// Different settings for gamepak ws0-2 sequential (2nd) access

uint8_t gamepak_waitstate_seq[2][2][3] =
{
  {{ 3, 5, 9 }, { 5, 9,17 }},
  {{ 2, 2, 2 }, { 3, 3, 3 }}
};

uint8_t cpu_waitstate_cycles_seq[2][16] =
{
 /* 0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
  { 1, 1, 3, 1, 1, 1, 1, 1, 3, 3, 5, 5, 9, 9, 5, 1 }, /* 8,16bit */
  { 1, 1, 6, 1, 1, 2, 2, 1, 5, 5, 9, 9,17,17, 1, 1 }  /* 32bit */
};
#endif

uint32_t prescale_table[] = { 0, 6, 8, 10 };

bool IsNintendoBIOS = false;

// GBA memory areas.

// Palette RAM             (05000000h)      1 KiB
FULLY_UNINITIALIZED(uint16_t palette_ram   [  0x200]);
// Object Attribute Memory (07000000h)      1 KiB
FULLY_UNINITIALIZED(uint16_t oam_ram       [  0x200]);
// I/O Registers           (04000000h)     32 KiB
FULLY_UNINITIALIZED(uint16_t io_registers  [ 0x4000]);
// External Working RAM    (02000000h)    256 KiB
FULLY_UNINITIALIZED(uint8_t  ewram_data    [0x40000]);
// Internal Working RAM    (03000000h)     32 KiB
FULLY_UNINITIALIZED(uint8_t  iwram_data    [ 0x8000]);
// Video RAM               (06000000h)     96 KiB
FULLY_UNINITIALIZED(uint8_t  vram          [0x18000]);
// BIOS ROM and code tags  (00000000h)     48 KiB
FULLY_UNINITIALIZED(struct BIOS_DATA bios);
// Backup flash/EEPROM...  (0E000000h)    128 KiB
uint8_t  gamepak_backup[0x20000];
// ----------------------------------------------
// Total                                  594 KiB

#ifndef USE_C_CORE
/*
 * These are Metadata Areas corresponding to the Data Areas above. They
 * contain information about the native code compilation status of each
 * Data Word in that Data Area. For more information about these, see
 * "doc/partial flushing of RAM code.txt" in your source tree.
 */
// Internal Working RAM code metadata      64 KiB
FULLY_UNINITIALIZED(uint16_t iwram_metadata[ 0x8000]);
// External Working RAM code metadata     512 KiB
FULLY_UNINITIALIZED(uint16_t ewram_metadata[0x40000]);
// Video RAM code metadata                192 KiB
FULLY_UNINITIALIZED(uint16_t vram_metadata [0x18000]);
// ----------------------------------------------
// Total                                  768 KiB
#endif

uint32_t flash_bank_offset = 0;

uint32_t bios_read_protect;

// Keeps us knowing how much we have left.
uint8_t *gamepak_rom = NULL;
uint32_t gamepak_size;

/******************************************************************************
 * 全局变量定义
 ******************************************************************************/
TIMER_TYPE timer[4];                              // 定时器

#ifdef USE_EXT_MEM
uint8_t *gamepak_rom_resume;
#endif

DMA_TRANSFER_TYPE dma[4];

uint8_t *memory_regions[16];
uint32_t memory_limits[16];

uint32_t gamepak_next_swap;

uint32_t gamepak_ram_buffer_size;
uint32_t gamepak_ram_pages;

char gamepak_title[13];
char gamepak_code[5];
char gamepak_maker[3];
char CurrentGamePath[MAX_PATH];
bool IsGameLoaded = false;
bool IsZippedROM = false; // true if the current ROM came directly from a
                          // zip file (false if it was extracted to temp)

uint32_t mem_save_flag;

// Enough to map the gamepak RAM space.
uint16_t gamepak_memory_map[1024];

// This is global so that it can be kept open for large ROMs to swap
// pages from, so there's no slowdown with opening and closing the file
// a lot.

FILE_TAG_TYPE gamepak_file_large = FILE_TAG_INVALID;

char main_path[MAX_PATH + 1];

// Writes to these respective locations should trigger an update
// so the related subsystem may react to it.

// If OAM is written to:
uint32_t oam_update = 1;

// If GBC audio is written to:
uint32_t gbc_sound_update = 0;

// If the GBC audio waveform is modified:
uint32_t gbc_sound_wave_update = 0;

// If the backup space is written (only update once this hits 0)
uint32_t backup_update = 0;

// Write out backup file this many cycles after the most recent
// backup write.
const uint32_t write_backup_delay = 10;

typedef enum
{
  BACKUP_SRAM,
  BACKUP_FLASH,
  BACKUP_EEPROM,
  BACKUP_NONE
} backup_type_type;

typedef enum
{
  SRAM_SIZE_32KB,
  SRAM_SIZE_64KB
} sram_size_type;

// Keep it 32KB until the upper 64KB is accessed, then make it 64KB.

backup_type_type backup_type = BACKUP_NONE;
sram_size_type sram_size = SRAM_SIZE_32KB;

typedef enum
{
  FLASH_BASE_MODE,
  FLASH_ERASE_MODE,
  FLASH_ID_MODE,
  FLASH_WRITE_MODE,
  FLASH_BANKSWITCH_MODE
} flash_mode_type;

typedef enum
{
  FLASH_SIZE_64KB,
  FLASH_SIZE_128KB
} flash_size_type;

flash_mode_type flash_mode = FLASH_BASE_MODE;
uint32_t flash_command_position = 0;

FLASH_DEVICE_ID_TYPE flash_device_id = FLASH_DEVICE_MACRONIX_64KB;
FLASH_MANUFACTURER_ID_TYPE flash_manufacturer_id =
 FLASH_MANUFACTURER_MACRONIX;
flash_size_type flash_size = FLASH_SIZE_64KB;

// Tilt sensor on the GBA side. It's mapped... somewhere... in the GBA address
// space. See the read_backup function in memory.c for more information.
uint32_t tilt_sensor_x;
uint32_t tilt_sensor_y;

uint8_t read_backup(uint32_t address)
{
  uint8_t value = 0;

  if(backup_type == BACKUP_EEPROM)
  {
    switch(address & 0x8f00)
    {
      case 0x8200:
        value = tilt_sensor_x & 255;
        break;
      case 0x8300:
        value = (tilt_sensor_x >> 8) | 0x80;
        break;
      case 0x8400:
        value = tilt_sensor_y & 255;
        break;
      case 0x8500:
        value = tilt_sensor_y >> 8;
        break;
    }
    return value;
  }

  if(backup_type == BACKUP_NONE)
    backup_type = BACKUP_SRAM;

  if(backup_type == BACKUP_SRAM)
  {
    value = gamepak_backup[address];
  }
  else

  if(flash_mode == FLASH_ID_MODE)
  {
    /* ID manufacturer type */
    if(address == 0x0000)
      value = flash_manufacturer_id;
    else

    /* ID device type */
    if(address == 0x0001)
      value = flash_device_id;
  }
  else
  {
    value = gamepak_backup[flash_bank_offset + address];
  }
  return value;
}

#define read_backup8()                                                        \
  value = read_backup(address & 0xFFFF)                                       \

#define read_backup16()                                                       \
  value = 0                                                                   \

#define read_backup32()                                                       \
  value = 0                                                                   \


// EEPROM is 512 bytes by default; it is autodetecte as 8KB if
// 14bit address DMAs are made (this is done in the DMA handler).

typedef enum
{
  EEPROM_512_BYTE,
  EEPROM_8_KBYTE
} eeprom_size_type;

typedef enum
{
  EEPROM_BASE_MODE,
  EEPROM_READ_MODE,
  EEPROM_READ_HEADER_MODE,
  EEPROM_ADDRESS_MODE,
  EEPROM_WRITE_MODE,
  EEPROM_WRITE_ADDRESS_MODE,
  EEPROM_ADDRESS_FOOTER_MODE,
  EEPROM_WRITE_FOOTER_MODE
} eeprom_mode_type;


eeprom_size_type eeprom_size = EEPROM_512_BYTE;
eeprom_mode_type eeprom_mode = EEPROM_BASE_MODE;
uint32_t eeprom_address_length;
uint32_t eeprom_address = 0;
int32_t eeprom_counter = 0;
uint8_t eeprom_buffer[8];


void write_eeprom(uint32_t address, uint32_t value)
{
  if(gamepak_size > 0x1FFFF00) // eeprom_v126 ?
  {                            // ROM is restricted to 8000000h-9FFFeFFh
    gamepak_size = 0x1FFFF00;  // (max.1FFFF00h bytes = 32MB minus 256 bytes)
  }

  switch(eeprom_mode)
  {
    case EEPROM_BASE_MODE:
      backup_type = BACKUP_EEPROM;
      eeprom_buffer[0] |= (value & 0x01) << (1 - eeprom_counter);
      eeprom_counter++;
      if(eeprom_counter == 2)
      {
        if(eeprom_size == EEPROM_512_BYTE)
          eeprom_address_length = 6;
        else
          eeprom_address_length = 14;

        eeprom_counter = 0;

        switch(eeprom_buffer[0] & 0x03)
        {
          case 0x02:
            eeprom_mode = EEPROM_WRITE_ADDRESS_MODE;
            break;

          case 0x03:
            eeprom_mode = EEPROM_ADDRESS_MODE;
            break;
        }
        eeprom_buffer[0] = eeprom_buffer[1] = 0;
      }
      break;

    case EEPROM_ADDRESS_MODE:
    case EEPROM_WRITE_ADDRESS_MODE:
      eeprom_buffer[eeprom_counter / 8]
       |= (value & 0x01) << (7 - (eeprom_counter % 8));
      eeprom_counter++;
      if(eeprom_counter == eeprom_address_length)
      {
        if(eeprom_size == EEPROM_512_BYTE)
        {
          // Little endian access
          eeprom_address = (((uint32_t)eeprom_buffer[0] >> 2) |
           ((uint32_t)eeprom_buffer[1] << 6)) * 8;
        }
        else
        {
          // Big endian access
          eeprom_address = (((uint32_t)eeprom_buffer[1] >> 2) |
           ((uint32_t)eeprom_buffer[0] << 6)) * 8;
        }

        eeprom_buffer[0] = eeprom_buffer[1] = 0;
        eeprom_counter = 0;

        if(eeprom_mode == EEPROM_ADDRESS_MODE)
        {
          eeprom_mode = EEPROM_ADDRESS_FOOTER_MODE;
        }
        else
        {
          eeprom_mode = EEPROM_WRITE_MODE;
          memset(gamepak_backup + eeprom_address, 0, 8);
        }
      }
      break;

    case EEPROM_WRITE_MODE:
      gamepak_backup[eeprom_address + (eeprom_counter / 8)] |=
       (value & 0x01) << (7 - (eeprom_counter % 8));
      eeprom_counter++;
      if(eeprom_counter == 64)
      {
        backup_update = write_backup_delay;
        eeprom_counter = 0;
        eeprom_mode = EEPROM_WRITE_FOOTER_MODE;
      }
      break;

    case EEPROM_ADDRESS_FOOTER_MODE:
    case EEPROM_WRITE_FOOTER_MODE:
      eeprom_counter = 0;
      if(eeprom_mode == EEPROM_ADDRESS_FOOTER_MODE)
      {
        eeprom_mode = EEPROM_READ_HEADER_MODE;
      }
      else
      {
        eeprom_mode = EEPROM_BASE_MODE;
      }
      break;
    default:
      break;
  }
}

#define read_memory_gamepak(type)                                             \
  uint32_t gamepak_index = address >> 15;                                     \
  uint8_t *map = memory_map_read[gamepak_index];                              \
                                                                              \
  if(map == NULL)                                                             \
    map = load_gamepak_page(gamepak_index & 0x3FF);                           \
                                                                              \
  value = ADDRESS##type(map, address & 0x7FFF)                                \

#define read_open8()                                                          \
  if(reg[REG_CPSR] & 0x20)                                                    \
    value = read_memory8(reg[REG_PC] + 2 + (address & 0x01));                 \
  else                                                                        \
    value = read_memory8(reg[REG_PC] + 4 + (address & 0x03))                  \

#define read_open16()                                                         \
  if(reg[REG_CPSR] & 0x20)                                                    \
    value = read_memory16(reg[REG_PC] + 2);                                   \
  else                                                                        \
    value = read_memory16(reg[REG_PC] + 4 + (address & 0x02))                 \

#define read_open32()                                                         \
  if(reg[REG_CPSR] & 0x20)                                                    \
  {                                                                           \
    uint32_t current_instruction = read_memory16(reg[REG_PC] + 2);            \
    value = current_instruction | (current_instruction << 16);                \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    value = read_memory32(reg[REG_PC] + 4);                                   \
  }                                                                           \

uint32_t read_eeprom()
{
  uint32_t value;

  switch(eeprom_mode)
  {
    case EEPROM_BASE_MODE:
      value = 1;
      break;

    case EEPROM_READ_MODE:
      value = (gamepak_backup[eeprom_address + (eeprom_counter / 8)] >>
       (7 - (eeprom_counter % 8))) & 0x01;
      eeprom_counter++;
      if(eeprom_counter == 64)
      {
        eeprom_counter = 0;
        eeprom_mode = EEPROM_BASE_MODE;
      }
      break;

    case EEPROM_READ_HEADER_MODE:
      value = 0;
      eeprom_counter++;
      if(eeprom_counter == 4)
      {
        eeprom_mode = EEPROM_READ_MODE;
        eeprom_counter = 0;
      }
      break;

    default:
      value = 0;
      break;
  }

  return value;
}

#define read_memory(type)                                                     \
  switch(address >> 24)                                                       \
  {                                                                           \
    case 0x00:                                                                \
      /* BIOS */                                                              \
      if(reg[REG_PC] >= 0x4000)                                               \
      {                                                                       \
        if(address < 0x4000)                                                  \
        {                                                                     \
          value = ADDRESS##type(&bios_read_protect, address & 0x03);          \
        }                                                                     \
        else                                                                  \
        {                                                                     \
          read_open##type();                                                  \
        }                                                                     \
      }                                                                       \
      else                                                                    \
        value = ADDRESS##type(bios.rom, address & 0x3FFF);                   \
      break;                                                                  \
                                                                              \
    case 0x02:                                                                \
      /* external work RAM */                                                 \
      value = ADDRESS##type(ewram_data, address & 0x3FFFF);                   \
      break;                                                                  \
                                                                              \
    case 0x03:                                                                \
      /* internal work RAM */                                                 \
      value = ADDRESS##type(iwram_data, address & 0x7FFF);                    \
      break;                                                                  \
                                                                              \
    case 0x04:                                                                \
      /* I/O registers */                                                     \
      if(address < 0x04000400)                                                \
      {                                                                       \
        value = ADDRESS##type(io_registers, address & 0x3FF);                 \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        read_open##type();                                                    \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x05:                                                                \
      /* palette RAM */                                                       \
      value = ADDRESS##type(palette_ram, address & 0x3FF);                    \
      break;                                                                  \
                                                                              \
    case 0x06:                                                                \
      /* VRAM */                                                              \
      if(address & 0x10000)                                                   \
      {                                                                       \
        value = ADDRESS##type(vram, address & 0x17FFF);                       \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        value = ADDRESS##type(vram, address & 0x1FFFF);                       \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x07:                                                                \
      /* OAM RAM */                                                           \
      value = ADDRESS##type(oam_ram, address & 0x3FF);                        \
      break;                                                                  \
                                                                              \
    case 0x08 ... 0x0C:                                                       \
      /* gamepak ROM */                                                       \
      if((address & 0x1FFFFFF) < gamepak_size)                                \
      {                                                                       \
        read_memory_gamepak(type);                                            \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        value = (address / 2) & 0xFFFF;                                       \
      }                                                                       \
      break;                                                                  \
                                                                              \
    case 0x0D:                                                                \
      if((address & 0x1FFFFFF) < gamepak_size)                                \
      {                                                                       \
        read_memory_gamepak(type);                                            \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        value = read_eeprom();                                                \
      }                                                                       \
                                                                              \
      break;                                                                  \
                                                                              \
    case 0x0E:                                                                \
    case 0x0F:                                                                \
      read_backup##type();                                                    \
      break;                                                                  \
                                                                              \
    case 0x20 ... 0xFF:                                                       \
      value = 0;                                                              \
      break;                                                                  \
                                                                              \
    default:                                                                  \
      read_open##type();                                                      \
      break;                                                                  \
  }                                                                           \

#define trigger_dma(dma_number)                                               \
  if(value & 0x8000)                                                          \
  {                                                                           \
    if(dma[dma_number].start_type == DMA_INACTIVE)                            \
    {                                                                         \
      uint32_t start_type = (value >> 12) & 0x03;                             \
      uint32_t dest_address = ADDRESS32(io_registers,                         \
       ((dma_number) * 12) + 0xB4) & 0xFFFFFFF;                               \
                                                                              \
      dma[dma_number].dma_channel = dma_number;                               \
      dma[dma_number].source_address =                                        \
       ADDRESS32(io_registers, ((dma_number) * 12) + 0xB0) & 0xFFFFFFF;       \
      dma[dma_number].dest_address = dest_address;                            \
      dma[dma_number].source_direction = (value >>  7) & 0x03;                \
      dma[dma_number].repeat_type = (value >> 9) & 0x01;                      \
      dma[dma_number].start_type = start_type;                                \
      dma[dma_number].irq = (value >> 14) & 0x01;                             \
                                                                              \
      /* If it is sound FIFO DMA make sure the settings are a certain way */  \
      if(((dma_number) >= 1) && ((dma_number) <= 2) &&                        \
       (start_type == DMA_START_SPECIAL))                                     \
      {                                                                       \
        dma[dma_number].length_type = DMA_32BIT;                              \
        dma[dma_number].length = 4;                                           \
        dma[dma_number].dest_direction = DMA_FIXED;                           \
        if(dest_address == 0x40000A4)                                         \
          dma[dma_number].direct_sound_channel = DMA_DIRECT_SOUND_B;          \
        else                                                                  \
          dma[dma_number].direct_sound_channel = DMA_DIRECT_SOUND_A;          \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        uint32_t length =                                                     \
         ADDRESS16(io_registers, ((dma_number) * 12) + 0xB8);                 \
                                                                              \
        if(((dma_number) == 3) && ((dest_address >> 24) == 0x0D) &&           \
         ((length & 0x1F) == 17))                                             \
        {                                                                     \
          eeprom_size = EEPROM_8_KBYTE;                                       \
        }                                                                     \
                                                                              \
        if((dma_number) < 3)                                                  \
          length &= 0x3FFF;                                                   \
                                                                              \
        if(length == 0)                                                       \
        {                                                                     \
          if((dma_number) == 3)                                               \
            length = 0x10000;                                                 \
          else                                                                \
            length = 0x04000;                                                 \
        }                                                                     \
                                                                              \
        dma[dma_number].length = length;                                      \
        dma[dma_number].length_type = (value >> 10) & 0x01;                   \
        dma[dma_number].dest_direction = (value >> 5) & 0x03;                 \
      }                                                                       \
                                                                              \
      ADDRESS16(io_registers, ((dma_number) * 12) + 0xBA) = value;            \
      if(start_type == DMA_START_IMMEDIATELY)                                 \
        return dma_transfer(dma + (dma_number));                              \
    }                                                                         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    dma[dma_number].start_type = DMA_INACTIVE;                                \
    dma[dma_number].direct_sound_channel = DMA_NO_DIRECT_SOUND;               \
    ADDRESS16(io_registers, ((dma_number) * 12) + 0xBA) = value;              \
  }                                                                           \

// TODO:タイマーカウンタ周りの処理は再検討

// タイマーリロード時のカウンタの設定(この時点ではタイマーにセットされない)
// タイマースタート時にカウンタに設定される
// ただし、32bitアクセス時には即座にタイマーにセットされる
// 実機では0~0xFFFFだが、gpSP内部では (0x10000~1)<<prescale(0,6,8,10)の値をとる
// 各カウンターにリロードする際にprescale分シフトされる
// TODO:32bitアクセスと8/16bitアクセスで処理を分ける必要がある
// 8/16ビットアクセス時には呼び出す必要がない？
#define COUNT_TIMER(timer_number)                                             \
  timer[timer_number].reload = 0x10000 - value;                               \
  if(timer_number < 2)                                                        \
  {                                                                           \
    uint32_t timer_reload =                                                   \
     timer[timer_number].reload;              \
    SOUND_UPDATE_FREQUENCY_STEP(timer_number);                                \
  }                                                                           \

// タイマーのアクセスとカウント開始処理
#define TRIGGER_TIMER(timer_number)                                           \
  if(value & 0x80)                                                            \
  {                                                                           \
    /* スタートビットが”1”だった場合 */                                     \
    if(timer[timer_number].status == TIMER_INACTIVE)                          \
    {                                                                         \
      /* タイマーが停止していた場合 */                                        \
      /* 各種設定をして、タイマー作動 */                                      \
                                                                              \
      /* リロード値を読み込む */                                              \
      uint32_t timer_reload = timer[timer_number].reload;                     \
                                                                              \
      /* カスケードモードか判別(タイマー0以外)*/                              \
      if(((value >> 2) & 0x01) && ((timer_number) != 0))                      \
      {                                                                       \
        /* カスケードモード */                                                \
        timer[timer_number].status = TIMER_CASCADE;                           \
        /* プリスケールの設定 */                                              \
        timer[timer_number].prescale = 0;                                     \
      }                                                                       \
      else                                                                    \
      {                                                                       \
        /* プリスケールモード */                                              \
        timer[timer_number].status = TIMER_PRESCALE;                          \
        uint32_t prescale = prescale_table[value & 0x03];                     \
        /* プリスケールの設定 */                                              \
        timer[timer_number].prescale = prescale;                              \
      }                                                                       \
                                                                              \
      /* IRQの設定 */                                                         \
      timer[timer_number].irq = (value >> 6) & 0x01;                          \
                                                                              \
      /* カウンタを設定 */                                                    \
      timer[timer_number].count = timer_reload << timer[timer_number].prescale; \
      ADDRESS16(io_registers, 0x100 + ((timer_number) * 4)) =                 \
      0x10000 - timer_reload;                                                 \
                                                                              \
      if(timer[timer_number].count < execute_cycles)                          \
        execute_cycles = timer[timer_number].count;                           \
                                                                              \
      if((timer_number) < 2)                                                  \
      {                                                                       \
        /* 小数点以下を切り捨てていたので、GBCサウンドと同様の処理にした*/   \
        SOUND_UPDATE_FREQUENCY_STEP(timer_number);                            \
        ADJUST_SOUND_BUFFER(timer_number, 0);                                 \
        ADJUST_SOUND_BUFFER(timer_number, 1);                                 \
      }                                                                       \
    }                                                                         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    if(timer[timer_number].status != TIMER_INACTIVE)                          \
    {                                                                         \
      timer[timer_number].status = TIMER_INACTIVE;                            \
    }                                                                         \
  }                                                                           \
  ADDRESS16(io_registers, 0x102 + ((timer_number) * 4)) = value;              \

// configure game pak access timings
#define waitstate_control()                                                   \
{                                                                             \
  uint8_t i;                                                                  \
  uint8_t waitstate_table[4] = { 5, 4, 3, 9 };                                \
                                                                              \
  waitstate_cycles_non_seq[0][0x0e] = waitstate_cycles_seq[0][0x0e]           \
   = waitstate_table[value & 0x03];                                           \
                                                                              \
  for(i = 0; i < 2; i++)                                                      \
  {                                                                           \
    waitstate_cycles_seq[i][0x08] = waitstate_cycles_seq[i][0x09]             \
     = gamepak_waitstate_seq[(value >> 4) & 0x01][i][0];                      \
    waitstate_cycles_seq[i][0x0A] = waitstate_cycles_seq[i][0x0B]             \
     = gamepak_waitstate_seq[(value >> 7) & 0x01][i][1];                      \
    waitstate_cycles_seq[i][0x0C] = waitstate_cycles_seq[i][0x0D]             \
     = gamepak_waitstate_seq[(value >> 10) & 0x01][i][2];                     \
  }                                                                           \
                                                                              \
  waitstate_cycles_non_seq[0][0x08] = waitstate_cycles_non_seq[0][0x09]       \
   = waitstate_table[(value >> 2) & 0x03];                                    \
  waitstate_cycles_non_seq[0][0x0A] = waitstate_cycles_non_seq[0][0x0B]       \
   = waitstate_table[(value >> 5) & 0x03];                                    \
  waitstate_cycles_non_seq[0][0x0C] = waitstate_cycles_non_seq[0][0x0D]       \
   = waitstate_table[(value >> 8) & 0x03];                                    \
                                                                              \
  /* 32bit access ( split into two 16bit accsess ) */                         \
  waitstate_cycles_non_seq[1][0x08] = waitstate_cycles_non_seq[1][0x09]       \
   = (waitstate_cycles_non_seq[0][0x08] + waitstate_cycles_seq[0][0x08] - 1); \
  waitstate_cycles_non_seq[1][0x0A] = waitstate_cycles_non_seq[1][0x0B]       \
   = (waitstate_cycles_non_seq[0][0x0A] + waitstate_cycles_seq[0][0x0A] - 1); \
  waitstate_cycles_non_seq[1][0x0C] = waitstate_cycles_non_seq[1][0x0D]       \
  =  (waitstate_cycles_non_seq[0][0x0C] + waitstate_cycles_seq[0][0x0C] - 1); \
                                                                              \
  /* gamepak prefetch */                                                      \
  if(value & 0x4000)                                                          \
  {                                                                           \
    for(i = 0x08; i <= 0x0D; i++)                                             \
    {                                                                         \
      cpu_waitstate_cycles_seq[0][i] = 1;                                     \
      cpu_waitstate_cycles_seq[1][i] = 2;                                     \
    }                                                                         \
  }                                                                           \
  else                                                                        \
  {                                                                           \
    for(i = 0; i < 2; i++)                                                    \
    {                                                                         \
      cpu_waitstate_cycles_seq[i][0x08] = cpu_waitstate_cycles_seq[i][0x09]   \
       = waitstate_cycles_seq[i][0x08];                                       \
      cpu_waitstate_cycles_seq[i][0x0A] = cpu_waitstate_cycles_seq[i][0x0B]   \
       = waitstate_cycles_seq[i][0x0A];                                       \
      cpu_waitstate_cycles_seq[i][0x0C] = cpu_waitstate_cycles_seq[i][0x0D]   \
       = waitstate_cycles_seq[i][0x0C];                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  ADDRESS16(io_registers, 0x204) =                                            \
   (ADDRESS16(io_registers, 0x204) & 0x8000) | (value & 0x7FFF);              \
}                                                                             \

#define access_register8_high(address)                                        \
  value = (value << 8) | (ADDRESS8(io_registers, address))                    \

#define access_register8_low(address)                                         \
  value = ((ADDRESS8(io_registers, address + 1)) << 8) | value                \

#define access_register16_high(address)                                       \
  value = (value << 16) | (ADDRESS16(io_registers, address))                  \

#define access_register16_low(address)                                        \
  value = ((ADDRESS16(io_registers, address + 2)) << 16) | value              \

CPU_ALERT_TYPE write_io_register8(uint32_t address, uint32_t value)
{
  switch(address)
  {
    case 0x00:
      {
        uint32_t dispcnt = io_registers[REG_DISPCNT];

        if((value & 0x07) != (dispcnt & 0x07))
          oam_update = 1;

        ADDRESS8(io_registers, 0x00) = value;
      }
      break;

    // DISPSTAT (lower byte)
    case 0x04:
      ADDRESS8(io_registers, 0x04) =
       (ADDRESS8(io_registers, 0x04) & 0x07) | (value & ~0x07);
      break;

    // VCOUNT
    case 0x06:
    case 0x07:
      break;

    // BG2 reference X
    case 0x28:
      access_register8_low(0x28);
      access_register16_low(0x28);
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    case 0x29:
      access_register8_high(0x28);
      access_register16_low(0x28);
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    case 0x2A:
      access_register8_low(0x2A);
      access_register16_high(0x28);
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    case 0x2B:
      access_register8_high(0x2A);
      access_register16_high(0x28);
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    // BG2 reference Y
    case 0x2C:
      access_register8_low(0x2C);
      access_register16_low(0x2C);
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    case 0x2D:
      access_register8_high(0x2C);
      access_register16_low(0x2C);
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    case 0x2E:
      access_register8_low(0x2E);
      access_register16_high(0x2C);
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    case 0x2F:
      access_register8_high(0x2E);
      access_register16_high(0x2C);
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    // BG3 reference X
    case 0x38:
      access_register8_low(0x38);
      access_register16_low(0x38);
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    case 0x39:
      access_register8_high(0x38);
      access_register16_low(0x38);
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    case 0x3A:
      access_register8_low(0x3A);
      access_register16_high(0x38);
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    case 0x3B:
      access_register8_high(0x3A);
      access_register16_high(0x38);
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    // BG3 reference Y
    case 0x3C:
      access_register8_low(0x3C);
      access_register16_low(0x3C);
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    case 0x3D:
      access_register8_high(0x3C);
      access_register16_low(0x3C);
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    case 0x3E:
      access_register8_low(0x3E);
      access_register16_high(0x3C);
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    case 0x3F:
      access_register8_high(0x3E);
      access_register16_high(0x3C);
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    // Sound 1 control sweep
    case 0x60:
      access_register8_low(0x60);
      GBC_SOUND_TONE_CONTROL_SWEEP();
      break;

    case 0x61:
      access_register8_low(0x60);
      GBC_SOUND_TONE_CONTROL_SWEEP();
      break;

    // Sound 1 control duty/length/envelope
    case 0x62:
      access_register8_low(0x62);
      GBC_SOUND_TONE_CONTROL_LOW(0, 0x62);
      break;

    case 0x63:
      access_register8_high(0x62);
      GBC_SOUND_TONE_CONTROL_LOW(0, 0x62);
      break;

    // Sound 1 control frequency
    case 0x64:
      access_register8_low(0x64);
      GBC_SOUND_TONE_CONTROL_HIGH(0, 0x64);
      break;

    case 0x65:
      access_register8_high(0x64);
      GBC_SOUND_TONE_CONTROL_HIGH(0, 0x64);
      break;

    // Sound 2 control duty/length/envelope
    case 0x68:
      access_register8_low(0x68);
      GBC_SOUND_TONE_CONTROL_LOW(1, 0x68);
      break;

    case 0x69:
      access_register8_high(0x68);
      GBC_SOUND_TONE_CONTROL_LOW(1, 0x68);
      break;

    // Sound 2 control frequency
    case 0x6C:
      access_register8_low(0x6C);
      GBC_SOUND_TONE_CONTROL_HIGH(1, 0x6C);
      break;

    case 0x6D:
      access_register8_high(0x6C);
      GBC_SOUND_TONE_CONTROL_HIGH(1, 0x6C);
      break;

    // Sound 3 control wave
    case 0x70:
      access_register8_low(0x70);
      GBC_SOUND_WAVE_CONTROL();
      break;

    case 0x71:
      access_register8_high(0x70);
      GBC_SOUND_WAVE_CONTROL();
      break;

    // Sound 3 control length/volume
    case 0x72:
      access_register8_low(0x72);
      GBC_SOUND_TONE_CONTROL_LOW_WAVE();
      break;

    case 0x73:
      access_register8_high(0x72);
      GBC_SOUND_TONE_CONTROL_LOW_WAVE();
      break;

    // Sound 3 control frequency
    case 0x74:
      access_register8_low(0x74);
      GBC_SOUND_TONE_CONTROL_HIGH_WAVE();
      break;

    case 0x75:
      access_register8_high(0x74);
      GBC_SOUND_TONE_CONTROL_HIGH_WAVE();
      break;

    // Sound 4 control length/envelope
    case 0x78:
      access_register8_low(0x78);
      GBC_SOUND_TONE_CONTROL_LOW(3, 0x78);
      break;

    case 0x79:
      access_register8_high(0x78);
      GBC_SOUND_TONE_CONTROL_LOW(3, 0x78);
      break;

    // Sound 4 control frequency
    case 0x7C:
      access_register8_low(0x7C);
      GBC_SOUND_NOISE_CONTROL();
      break;

    case 0x7D:
      access_register8_high(0x7C);
      GBC_SOUND_NOISE_CONTROL();
      break;

    // Sound control L
    case 0x80:
      access_register8_low(0x80);
      GBC_TRIGGER_SOUND();
      break;

    case 0x81:
      access_register8_high(0x80);
      GBC_TRIGGER_SOUND();
      break;

    // Sound control H
    case 0x82:
      access_register8_low(0x82);
      TRIGGER_SOUND();
      break;

    case 0x83:
      access_register8_high(0x82);
      TRIGGER_SOUND();
      break;

    // Sound control X
    case 0x84:
      SOUND_ON();
      break;

    // Sound wave RAM
    case 0x90 ... 0x9F:
      gbc_sound_wave_update = 1;
      ADDRESS8(io_registers, address) = value;
      break;

    // Sound FIFO A
    case 0xA0 ... 0xA3:
      ADDRESS8(io_registers, address) = value;
      sound_timer_queue32(0);
      break;

    // Sound FIFO B
    case 0xA4 ... 0xA7:
      ADDRESS8(io_registers, address) = value;
      sound_timer_queue32(1);
      break;

    // DMA control (trigger byte)
    case 0xBB:  // DMA channel 0
    case 0xC7:  // DMA channel 1
    case 0xD3:  // DMA channel 2
    case 0xDF:  // DMA channel 3
      access_register8_high(address - 1);
      trigger_dma((address - 0xBB) / 12);
      break;

    // Timer counts
    case 0x100:
      access_register8_low(0x100);
      COUNT_TIMER(0);
      break;

    case 0x101:
      access_register8_high(0x100);
      COUNT_TIMER(0);
      break;

    case 0x104:
      access_register8_low(0x104);
      COUNT_TIMER(1);
      break;

    case 0x105:
      access_register8_high(0x104);
      COUNT_TIMER(1);
      break;

    case 0x108:
      access_register8_low(0x108);
      COUNT_TIMER(2);
      break;

    case 0x109:
      access_register8_high(0x108);
      COUNT_TIMER(2);
      break;

    case 0x10C:
      access_register8_low(0x10C);
      COUNT_TIMER(3);
      break;

    case 0x10D:
      access_register8_high(0x10C);
      COUNT_TIMER(3);
      break;

    // Timer control (trigger byte)
    case 0x103:  // Timer 0
    case 0x107:  // Timer 1
    case 0x10B:  // Timer 2
    case 0x10F:  // Timer 3
      access_register8_high(address - 1);
      TRIGGER_TIMER((address - 0x103) / 4);
      break;

    case 0x128:
#ifdef USE_ADHOC
      if(g_adhoc_link_flag == NO)
        ADDRESS8(io_registers, 0x128) |= 0x0C;
#endif
      break;

    case 0x129:
    case 0x134:
    case 0x135:
    // P1
    case 0x130:
    case 0x131:
      /* Read only */
      break;

      // IE
      case 0x200:
        ADDRESS8(io_registers, 0x200) = value;
        break;

      case 0x201:
        ADDRESS8(io_registers, 0x201) = value;
        break;

    // IF
    case 0x202:
      ADDRESS8(io_registers, 0x202) &= ~value;
      break;

    case 0x203:
      ADDRESS8(io_registers, 0x203) &= ~value;
      break;

    // WAITCNT
    case 0x204:
#ifndef OLD_COUNT
      access_register8_low(0x204);
      waitstate_control();
#endif
      break;

    case 0x205:
#ifndef OLD_COUNT
      access_register8_high(0x204);
      waitstate_control();
#endif
      break;

    // Halt
    case 0x301:
      if(value & 0x80)
        reg[CPU_HALT_STATE] = CPU_STOP;
      else
        reg[CPU_HALT_STATE] = CPU_HALT;
      return CPU_ALERT_HALT;

    default:
      ADDRESS8(io_registers, address) = value;
      break;
  }

  return CPU_ALERT_NONE;
}

CPU_ALERT_TYPE write_io_register16(uint32_t address, uint32_t value)
{
  switch(address)
  {
    case 0x00:
    {
      uint32_t dispcnt = io_registers[REG_DISPCNT];
      if((value & 0x07) != (dispcnt & 0x07))
        oam_update = 1;

      ADDRESS16(io_registers, 0x00) = value;
      break;
    }

    // DISPSTAT
    case 0x04:
      ADDRESS16(io_registers, 0x04) =
       (ADDRESS16(io_registers, 0x04) & 0x07) | (value & ~0x07);
      break;

    // VCOUNT
    case 0x06:
      break;

    // BG2 reference X
    case 0x28:
      access_register16_low(0x28);
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    case 0x2A:
      access_register16_high(0x28);
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    // BG2 reference Y
    case 0x2C:
      access_register16_low(0x2C);
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    case 0x2E:
      access_register16_high(0x2C);
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    // BG3 reference X

    case 0x38:
      access_register16_low(0x38);
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    case 0x3A:
      access_register16_high(0x38);
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    // BG3 reference Y
    case 0x3C:
      access_register16_low(0x3C);
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    case 0x3E:
      access_register16_high(0x3C);
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    // Sound 1 control sweep
    case 0x60:
      GBC_SOUND_TONE_CONTROL_SWEEP();
      break;

    // Sound 1 control duty/length/envelope
    case 0x62:
      GBC_SOUND_TONE_CONTROL_LOW(0, 0x62);
      break;

    // Sound 1 control frequency
    case 0x64:
      GBC_SOUND_TONE_CONTROL_HIGH(0, 0x64);
      break;

    // Sound 2 control duty/length/envelope
    case 0x68:
      GBC_SOUND_TONE_CONTROL_LOW(1, 0x68);
      break;

    // Sound 2 control frequency
    case 0x6C:
      GBC_SOUND_TONE_CONTROL_HIGH(1, 0x6C);
      break;

    // Sound 3 control wave
    case 0x70:
      GBC_SOUND_WAVE_CONTROL();
      break;

    // Sound 3 control length/volume
    case 0x72:
      GBC_SOUND_TONE_CONTROL_LOW_WAVE();
      break;

    // Sound 3 control frequency
    case 0x74:
      GBC_SOUND_TONE_CONTROL_HIGH_WAVE();
      break;

    // Sound 4 control length/envelope
    case 0x78:
      GBC_SOUND_TONE_CONTROL_LOW(3, 0x78);
      break;

    // Sound 4 control frequency
    case 0x7C:
      GBC_SOUND_NOISE_CONTROL();
      break;

    // Sound control L
    case 0x80:
      GBC_TRIGGER_SOUND();
      break;

    // Sound control H
    case 0x82:
      TRIGGER_SOUND();
      break;

    // Sound control X
    case 0x84:
      SOUND_ON();
      break;

    // Sound wave RAM
    case 0x90 ... 0x9E:
      gbc_sound_wave_update = 1;
      ADDRESS16(io_registers, address) = value;
      break;

    // Sound FIFO A
    case 0xA0:
    case 0xA2:
      ADDRESS16(io_registers, address) = value;
      sound_timer_queue32(0);
      break;

    // Sound FIFO B
    case 0xA4:
    case 0xA6:
      ADDRESS16(io_registers, address) = value;
      sound_timer_queue32(1);
      break;

    // DMA control
    case 0xBA:  // DMA channel 0
    case 0xC6:  // DMA channel 1
    case 0xD2:  // DMA channel 2
    case 0xDE:  // DMA channel 3
      trigger_dma((address - 0xBA) / 12);
      break;

    // Timer counts
    case 0x100:
    case 0x104:
    case 0x108:
    case 0x10C:
      COUNT_TIMER((address - 0x100) / 4);
      break;

    // Timer control
    case 0x102:  // Timer 0
    case 0x106:  // Timer 1
    case 0x10A:  // Timer 2
    case 0x10E:  // Timer 3
      TRIGGER_TIMER((address - 0x102) / 4);
      break;

#ifdef USE_ADHOC
// SIOCNT
//      Bit   Expl.
//      0-1   Baud Rate          (0-3: 9600,38400,57600,115200 bps)
//      2     SI-Terminal        (0=Parent, 1=Child)                  (Read Only)
//      3     SD-Terminal        (0=Bad connection, 1=All GBAs Ready) (Read Only)

//      4-5   Multi-Player ID    (0=Parent, 1-3=1st-3rd child)        (Read Only)
//      6     Multi-Player Error (0=Normal, 1=Error)                  (Read Only)
//      7     Start/Busy Bit     (0=Inactive, 1=Start/Busy) (Read Only for Slaves)

//      8-11  Not used           (R/W, should be 0)

//      12    Must be "0" for Multi-Player mode
//      13    Must be "1" for Multi-Player mode
//      14    IRQ Enable         (0=Disable, 1=Want IRQ upon completion)
//      15    Not used           (Read only, always 0)
    case 0x128:
      if(g_adhoc_link_flag == NO)
        ADDRESS16(io_registers, 0x128) |= 0x0C;
      else
      {
        switch(get_sio_mode(value, ADDRESS16(io_registers, 0x134)))
        {
          case SIO_MULTIPLAYER: // マルチプレイヤーモード
            if(value & 0x80) // bit7(start bit)が1の時 転送開始命令
            {
              if(!g_multi_id)  // 親モードの時 g_multi_id = 0
              {
                if(!g_adhoc_transfer_flag) // g_adhoc_transfer_flag == 0 転送中で無いとき
                {
                  g_multi_mode = MULTI_START; // データの送信
                } // 転送中の時
                value |= (g_adhoc_transfer_flag != 0)<<7;
              }
            }
            if(g_multi_id)
            {
              value &= 0xf00b;
              value |= 0x8;
              value |= g_multi_id << 4;
              ADDRESS16(io_registers, 0x128) = value;
              ADDRESS16(io_registers, 0x134) = 7; // 親と子で0x134の設定値を変える
            }
            else
            {
              value &= 0xf00f;
              value |= 0xc;
              value |= g_multi_id << 4;
              ADDRESS16(io_registers, 0x128) = value;
              ADDRESS16(io_registers, 0x134) = 3;
            }
            break;
        }
      }

    // SIOMLT_SEND
    case 0x12A:
      ADDRESS16(io_registers, 0x12A) = value;
      break;

    // RCNT
    case 0x134:
      if(!value) // 0の場合
      {
        ADDRESS16(io_registers, 0x134) = 0;
        return CPU_ALERT_NONE;
      }
      switch(get_sio_mode(ADDRESS16(io_registers, 0x128), value))
      {
        case SIO_MULTIPLAYER:
          value &= 0xc0f0;
          value |= 3;
          if(g_multi_id) value |= 4;
          ADDRESS16(io_registers, 0x134) = value;
          ADDRESS16(io_registers, 0x128) = ((ADDRESS16(io_registers, 0x128)&0xff8b)|(g_multi_id ? 0xc : 8)|(g_multi_id<<4));
          break;

        default:
          ADDRESS16(io_registers, 0x134) = value;
          break;
      }
      break;
#else
    case 0x128:
      ADDRESS16(io_registers, 0x128) |= 0x0C;
      break;
#endif

    // IE
    case 0x200:
      ADDRESS16(io_registers, 0x200) = value;
      break;

    // Interrupt flag
    case 0x202:
      ADDRESS16(io_registers, 0x202) &= ~value;
      break;

    // WAITCNT
    case 0x204:
#ifndef OLD_COUNT
      waitstate_control();
#endif
      break;

    // Halt
    case 0x300:
      if(value & 0x8000)
        reg[CPU_HALT_STATE] = CPU_STOP;
      else
        reg[CPU_HALT_STATE] = CPU_HALT;

      return CPU_ALERT_HALT;

    default:
      ADDRESS16(io_registers, address) = value;
      break;
  }

  return CPU_ALERT_NONE;
}


CPU_ALERT_TYPE write_io_register32(uint32_t address, uint32_t value)
{
  switch(address)
  {
    // BG2 reference X
    case 0x28:
      affine_reference_x[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x28) = value;
      break;

    // BG2 reference Y
    case 0x2C:
      affine_reference_y[0] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x2C) = value;
      break;

    // BG3 reference X
    case 0x38:
      affine_reference_x[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x38) = value;
      break;

    // BG3 reference Y
    case 0x3C:
      affine_reference_y[1] = (int32_t)(value << 4) >> 4;
      ADDRESS32(io_registers, 0x3C) = value;
      break;

    // Sound FIFO A
    case 0xA0:
      ADDRESS32(io_registers, 0xA0) = value;
      sound_timer_queue32(0);
      break;

    // Sound FIFO B
    case 0xA4:
      ADDRESS32(io_registers, 0xA4) = value;
      sound_timer_queue32(1);
      break;

    default:
    {
      CPU_ALERT_TYPE alert_low =
        write_io_register16(address, value & 0xFFFF);

      CPU_ALERT_TYPE alert_high =
        write_io_register16(address + 2, value >> 16);

//      if(alert_high)
//        return alert_high;

      return alert_high | alert_low;
    }
  }

  return CPU_ALERT_NONE;
}

#define write_palette8(address, value)                                        \
  ADDRESS16(palette_ram, (address & ~0x01)) = ((value << 8) | value)          \

#define write_palette16(address, value)                                       \
  ADDRESS16(palette_ram, address) = value                                     \

#define write_palette32(address, value)                                       \
  ADDRESS32(palette_ram, address) = value                                     \


void write_backup(uint32_t address, uint32_t value)
{
  value &= 0xFF;

  if(backup_type == BACKUP_NONE)
    backup_type = BACKUP_SRAM;


  // gamepak SRAM or Flash ROM
  if((address == 0x5555) && (flash_mode != FLASH_WRITE_MODE))
  {
    if((flash_command_position == 0) && (value == 0xAA))
    {
      backup_type = BACKUP_FLASH;
      flash_command_position = 1;
    }

    if(flash_command_position == 2)
    {
      switch(value)
      {
        case 0x90:
          // Enter ID mode, this also tells the emulator that we're using
          // flash, not SRAM

          if(flash_mode == FLASH_BASE_MODE)
            flash_mode = FLASH_ID_MODE;

          break;

        case 0x80:
          // Enter erase mode
          if(flash_mode == FLASH_BASE_MODE)
            flash_mode = FLASH_ERASE_MODE;
          break;

        case 0xF0:
          // Terminate ID mode
          if(flash_mode == FLASH_ID_MODE)
            flash_mode = FLASH_BASE_MODE;
          break;

        case 0xA0:
          // Write mode
          if(flash_mode == FLASH_BASE_MODE)
            flash_mode = FLASH_WRITE_MODE;
          break;

        case 0xB0:
          // Bank switch
          // Here the chip is now officially 128KB.
          flash_size = FLASH_SIZE_128KB;
          if(flash_mode == FLASH_BASE_MODE)
            flash_mode = FLASH_BANKSWITCH_MODE;
          break;

        case 0x10:
          // Erase chip
          if(flash_mode == FLASH_ERASE_MODE)
          {
            if(flash_size == FLASH_SIZE_64KB)
              memset(gamepak_backup, 0xFF, 1024 * 64);
            else
              memset(gamepak_backup, 0xFF, 1024 * 128);
            backup_update = write_backup_delay;
            flash_mode = FLASH_BASE_MODE;
          }
          break;

        default:
          break;
      }
      flash_command_position = 0;
    }
    if(backup_type == BACKUP_SRAM)
      gamepak_backup[0x5555] = value;
  }
  else

  if((address == 0x2AAA) && (value == 0x55) &&
   (flash_command_position == 1))
  {
    flash_command_position = 2;
  }
  else
  {
    if((flash_command_position == 2) &&
     (flash_mode == FLASH_ERASE_MODE) && (value == 0x30))
    {
      // Erase sector
      memset(gamepak_backup + flash_bank_offset + (address & 0xF000), 0xFF, 1024 * 4);
      backup_update = write_backup_delay;
      flash_mode = FLASH_BASE_MODE;
      flash_command_position = 0;
    }
    else

    if((flash_command_position == 0) &&
     (flash_mode == FLASH_BANKSWITCH_MODE) && (address == 0x0000) &&
     (flash_size == FLASH_SIZE_128KB))
    {
      flash_bank_offset = ((value & 0x01) * (1024 * 64));
      flash_mode = FLASH_BASE_MODE;
    }
    else

    if((flash_command_position == 0) && (flash_mode == FLASH_WRITE_MODE))
    {
      // Write value to flash ROM
      backup_update = write_backup_delay;
      gamepak_backup[flash_bank_offset + address] = value;
      flash_mode = FLASH_BASE_MODE;
    }
    else

    if(backup_type == BACKUP_SRAM)
    {
      // Write value to SRAM
      backup_update = write_backup_delay;
      // Hit 64KB territory?
      if(address >= 0x8000)
        sram_size = SRAM_SIZE_64KB;
      gamepak_backup[address] = value;
    }
  }
}

#define write_backup8()                                                       \
  write_backup(address & 0xFFFF, value)                                       \

#define write_backup16()                                                      \

#define write_backup32()                                                      \

// TODO bitmapモードにより動作に違いがある
#define write_vram8()                                                         \
  address &= 0x1FFFE;                                                         \
  if(address >= 0x18000)                                                      \
    address -= 0x8000;                                                        \
  ADDRESS16(vram, address) = ((value << 8) | value)                           \

#define write_vram16()                                                        \
  address &= 0x1FFFF;                                                         \
  if(address >= 0x18000)                                                      \
    address -= 0x8000;                                                        \
  ADDRESS16(vram, address) = value                                            \

#define write_vram32()                                                        \
  address &= 0x1FFFF;                                                         \
  if(address >= 0x18000)                                                      \
    address -= 0x8000;                                                        \
  ADDRESS32(vram, address) = value                                            \

#define write_oam_ram8()                                                      \

#define write_oam_ram16()                                                     \
  oam_update = 1;                                                             \
  ADDRESS16(oam_ram, address & 0x3FF) = value                                 \

#define write_oam_ram32()                                                     \
  oam_update = 1;                                                             \
  ADDRESS32(oam_ram, address & 0x3FF) = value                                 \

// RTC code derived from VBA's (due to lack of any real publically available
// documentation...)

typedef enum
{
  RTC_DISABLED,
  RTC_IDLE,
  RTC_COMMAND,
  RTC_OUTPUT_DATA,
  RTC_INPUT_DATA
} rtc_state_type;

typedef enum
{
  RTC_COMMAND_RESET            = 0x60,
  RTC_COMMAND_WRITE_STATUS     = 0x62,
  RTC_COMMAND_READ_STATUS      = 0x63,
  RTC_COMMAND_OUTPUT_TIME_FULL = 0x65,
  RTC_COMMAND_OUTPUT_TIME      = 0x67
} rtc_command_type;

typedef enum
{
  RTC_WRITE_TIME,
  RTC_WRITE_TIME_FULL,
  RTC_WRITE_STATUS
} rtc_write_mode_type;

rtc_state_type rtc_state = RTC_DISABLED;
rtc_write_mode_type rtc_write_mode;
uint8_t rtc_registers[3];
uint32_t rtc_command;
uint32_t rtc_data[12];
uint32_t rtc_status = 0x40;
uint32_t rtc_data_bytes;
int32_t rtc_bit_count;

uint32_t encode_bcd(uint8_t value)
{
  return ((value / 10) << 4) | (value % 10);
}

#define write_rtc_register(index, _value)                                     \
  update_address = 0x80000C4 + (index * 2);                                   \
  rtc_registers[index] = _value;                                              \
  rtc_page_index = update_address >> 15;                                      \
  map = memory_map_read[rtc_page_index];                                      \
                                                                              \
  if(map == NULL)                                                             \
    map = load_gamepak_page(rtc_page_index & 0x3FF);                          \
                                                                              \
  ADDRESS16(map, update_address & 0x7FFF) = _value                            \

void write_rtc(uint32_t address, uint32_t value)
{
  uint32_t rtc_page_index;
  uint32_t update_address;
  uint8_t *map;

  value &= 0xFFFF;

  switch(address)
  {
    // RTC command
    // Bit 0: SCHK, perform action
    // Bit 1: IO, input/output command data
    // Bit 2: CS, select input/output? If high make I/O write only
    case 0xC4:
      if(rtc_state == RTC_DISABLED)
        rtc_state = RTC_IDLE;
      if(!(rtc_registers[0] & 0x04))
        value = (rtc_registers[0] & 0x02) | (value & ~0x02);
      if(rtc_registers[2] & 0x01)
      {
        // To begin writing a command 1, 5 must be written to the command
        // registers.
        if((rtc_state == RTC_IDLE) && (rtc_registers[0] == 0x01) &&
         (value == 0x05))
        {
          // We're now ready to begin receiving a command.
          write_rtc_register(0, value);
          rtc_state = RTC_COMMAND;
          rtc_command = 0;
          rtc_bit_count = 7;
        }
        else
        {
          write_rtc_register(0, value);
          switch(rtc_state)
          {
            // Accumulate RTC command by receiving the next bit, and if we
            // have accumulated enough bits to form a complete command
            // execute it.
            case RTC_COMMAND:
              if(rtc_registers[0] & 0x01)
              {
                rtc_command |= ((value & 0x02) >> 1) << rtc_bit_count;
                rtc_bit_count--;
              }

              // Have we received a full RTC command? If so execute it.
              if(rtc_bit_count < 0)
              {
                switch(rtc_command)
                {
                  // Resets RTC
                  case RTC_COMMAND_RESET:
                    rtc_state = RTC_IDLE;
                    memset(rtc_registers, 0, sizeof(rtc_registers));
                    break;

                  // Sets status of RTC
                  case RTC_COMMAND_WRITE_STATUS:
                    rtc_state = RTC_INPUT_DATA;
                    rtc_data_bytes = 1;
                    rtc_write_mode = RTC_WRITE_STATUS;
                    break;

                  // Outputs current status of RTC
                  case RTC_COMMAND_READ_STATUS:
                    rtc_state = RTC_OUTPUT_DATA;
                    rtc_data_bytes = 1;
                    rtc_data[0] = rtc_status;
                    break;

                  // Actually outputs the time, all of it
                  // 0x65
                  case RTC_COMMAND_OUTPUT_TIME_FULL:
                  {
                    rtc_state = RTC_OUTPUT_DATA;
                    rtc_data_bytes = 7;

                    struct ReGBA_RTC Time;
                    ReGBA_LoadRTCTime(&Time);
                    rtc_data[0] = encode_bcd(Time.year % 100);
                    rtc_data[1] = encode_bcd(Time.month);
                    rtc_data[2] = encode_bcd(Time.day);
                    rtc_data[3] = encode_bcd(Time.weekday);
                    rtc_data[4] = encode_bcd(Time.hours);
                    rtc_data[5] = encode_bcd(Time.minutes);
                    rtc_data[6] = encode_bcd(Time.seconds);

                    break;
                  }

                  // Only outputs the current time of day.
                  // 0x67
                  case RTC_COMMAND_OUTPUT_TIME:
                  {
                    rtc_state = RTC_OUTPUT_DATA;
                    rtc_data_bytes = 3;

                    struct ReGBA_RTC Time;
                    ReGBA_LoadRTCTime(&Time);
                    rtc_data[0] = encode_bcd(Time.hours);
                    rtc_data[1] = encode_bcd(Time.minutes);
                    rtc_data[2] = encode_bcd(Time.seconds);

                    break;
                  }
                }
                rtc_bit_count = 0;
              }
              break;

            // Receive parameters from the game as input to the RTC
            // for a given command. Read one bit at a time.
            case RTC_INPUT_DATA:
              // Bit 1 of parameter A must be high for input
              if(rtc_registers[1] & 0x02)
              {
                // Read next bit for input
                if(!(value & 0x01))
                {
                  rtc_data[rtc_bit_count >> 3] |=
                   ((value & 0x01) << (7 - (rtc_bit_count & 0x07)));
                }
                else
                {
                  rtc_bit_count++;

                  if(rtc_bit_count == (rtc_data_bytes * 8))
                  {
                    rtc_state = RTC_IDLE;
                    switch(rtc_write_mode)
                    {
                      case RTC_WRITE_STATUS:
                        rtc_status = rtc_data[0];
                        break;

                      default:
                        break;
                    }
                  }
                }
              }
              break;

            case RTC_OUTPUT_DATA:
              // Bit 1 of parameter A must be low for output
              if(!(rtc_registers[1] & 0x02))
              {
                // Write next bit to output, on bit 1 of parameter B
                if(!(value & 0x01))
                {
                  uint8_t current_output_byte = rtc_registers[2];

                  current_output_byte =
                   (current_output_byte & ~0x02) |
                   (((rtc_data[rtc_bit_count >> 3] >>
                   (rtc_bit_count & 0x07)) & 0x01) << 1);

                  write_rtc_register(0, current_output_byte);

                }
                else
                {
                  rtc_bit_count++;

                  if(rtc_bit_count == (rtc_data_bytes * 8))
                  {
                    rtc_state = RTC_IDLE;
                    memset(rtc_registers, 0, sizeof(rtc_registers));
                  }
                }
              }
              break;

            default:
              ;
              break;
          }
        }
      }
      else
      {
        write_rtc_register(2, value);
      }
      break;

    // Write parameter A
    case 0xC6:
      write_rtc_register(1, value);
      break;

    // Write parameter B
    case 0xC8:
      write_rtc_register(2, value);
      break;
  }
}

#define write_rtc8()                                                          \

#define write_rtc16()                                                         \
  write_rtc(address & 0xFF, value)                                            \

#define write_rtc32()                                                         \

// type = 8 / 16 / 32
#define write_memory(type)                                                    \
  switch(address >> 24)                                                       \
  {                                                                           \
    case 0x02:                                                                \
      /* external work RAM */                                                 \
      ADDRESS##type(ewram_data, address & 0x3FFFF) = value;                   \
      break;                                                                  \
                                                                              \
    case 0x03:                                                                \
      /* internal work RAM */                                                 \
      ADDRESS##type(iwram_data, address & 0x7FFF) = value;                    \
      break;                                                                  \
                                                                              \
    case 0x04:                                                                \
      /* I/O registers */                                                     \
      if(address < 0x04000400)                                                \
        return write_io_register##type(address & 0x3FF, value);                 /* IOは0x803まで存在 */ \
      break;                                                                    /* 0x800は0x800ごとにループしている:TODO */ \
                                                                              \
    case 0x05:                                                                \
      /* palette RAM */                                                       \
      write_palette##type(address & 0x3FF, value);                            \
      break;                                                                  \
                                                                              \
    case 0x06:                                                                \
      /* VRAM */                                                              \
      write_vram##type();                                                     \
      break;                                                                  \
                                                                              \
    case 0x07:                                                                \
      /* OAM RAM */                                                           \
      write_oam_ram##type();                                                  \
      break;                                                                  \
                                                                              \
    case 0x08:                                                                \
      /* gamepak ROM or RTC */                                                \
      write_rtc##type();                                                      \
      break;                                                                  \
                                                                              \
    case 0x09 ... 0x0C:                                                       \
      /* gamepak ROM space */                                                 \
      break;                                                                  \
                                                                              \
    case 0x0D:                                                                \
      write_eeprom(address, value);                                           \
      break;                                                                  \
                                                                              \
    case 0x0E:                                                                \
      write_backup##type();                                                   \
      break;                                                                  \
                                                                              \
    default:                                                                  \
      /* unwritable */                                                        \
      break;                                                                  \
  }                                                                           \

uint8_t read_memory8(uint32_t address)
{
  uint8_t value;
  read_memory(8);
  return value;
}

uint16_t read_memory16_signed(uint32_t address)
{
  uint16_t value;

  if(address & 0x01)
  {
    return (int8_t)read_memory8(address);
  }
  else
  {
    read_memory(16);
  }

  return value;
}

// unaligned reads are actually 32bit

uint32_t read_memory16(uint32_t address)
{
  uint32_t value;

  if(address & 0x01)
  {
    address &= ~0x01;
    read_memory(16);
    ROR(value, value, 8);
  }
  else
  {
    read_memory(16);
  }

  return value;
}


uint32_t read_memory32(uint32_t address)
{
  uint32_t value;
  if(address & 0x03)
  {
    uint32_t rotate = (address & 0x03) * 8;
    address &= ~0x03;
    read_memory(32);
    ROR(value, value, rotate);
  }
  else
  {
    read_memory(32);
  }

  return value;
}

CPU_ALERT_TYPE write_memory8(uint32_t address, uint8_t value)
{
  write_memory(8);
  return CPU_ALERT_NONE;
}

CPU_ALERT_TYPE write_memory16(uint32_t address, uint16_t value)
{
  write_memory(16);
  return CPU_ALERT_NONE;
}

CPU_ALERT_TYPE write_memory32(uint32_t address, uint32_t value)
{
  write_memory(32);
  return CPU_ALERT_NONE;
}

char backup_filename[MAX_FILE];

uint32_t load_backup()
{
	char BackupFilename[MAX_PATH + 1];
	if (!ReGBA_GetBackupFilename(BackupFilename, CurrentGamePath))
	{
		ReGBA_Trace("W: Failed to get the name of the saved data file for '%s'", CurrentGamePath);
		return 0;
	}
	ReGBA_ProgressInitialise(FILE_ACTION_LOAD_BATTERY);

  FILE_TAG_TYPE backup_file;

  FILE_OPEN(backup_file, BackupFilename, READ);

  if(FILE_CHECK_VALID(backup_file))
  {
    uint32_t backup_size = FILE_LENGTH(backup_file);

    FILE_READ(backup_file, gamepak_backup, backup_size);
    FILE_CLOSE(backup_file);
	ReGBA_ProgressUpdate(1, 1);
	ReGBA_ProgressFinalise();

    // The size might give away what kind of backup it is.
    switch(backup_size)
    {
      case 0x200:
        backup_type = BACKUP_EEPROM;
        eeprom_size = EEPROM_512_BYTE;
        break;

      case 0x2000:
        backup_type = BACKUP_EEPROM;
        eeprom_size = EEPROM_8_KBYTE;
        break;

      case 0x8000:
        backup_type = BACKUP_SRAM;
        sram_size = SRAM_SIZE_32KB;
        break;

      // Could be either flash or SRAM, go with flash
      case 0x10000:
        backup_type = BACKUP_FLASH;
        flash_size = FLASH_SIZE_64KB;
        sram_size = SRAM_SIZE_64KB;
        break;

      case 0x20000:
        backup_type = BACKUP_FLASH;
        flash_size = FLASH_SIZE_128KB;
        break;
    }
    return 1;
  }
  else
  {
	ReGBA_ProgressFinalise();
    backup_type = BACKUP_NONE;
    memset(gamepak_backup, 0xFF, 1024 * 128);
  }

  return 0;
}

uint32_t save_backup()
{
	char BackupFilename[MAX_PATH + 1];
	if (!ReGBA_GetBackupFilename(BackupFilename, CurrentGamePath))
	{
		ReGBA_Trace("W: Failed to get the name of the saved data file for '%s'", CurrentGamePath);
		return 0;
	}

  FILE_TAG_TYPE backup_file;

  if(backup_type != BACKUP_NONE)
  {
	ReGBA_ProgressInitialise(FILE_ACTION_SAVE_BATTERY);
    FILE_OPEN(backup_file, BackupFilename, WRITE);

    if(FILE_CHECK_VALID(backup_file))
    {
      uint32_t backup_size = 0x8000;

      switch(backup_type)
      {
        case BACKUP_SRAM:
          if(sram_size == SRAM_SIZE_32KB)
            backup_size = 0x8000;
          else
            backup_size = 0x10000;
          break;

        case BACKUP_FLASH:
          if(flash_size == FLASH_SIZE_64KB)
            backup_size = 0x10000;
          else
            backup_size = 0x20000;
          break;

        case BACKUP_EEPROM:
          if(eeprom_size == EEPROM_512_BYTE)
            backup_size = 0x200;
          else
            backup_size = 0x2000;
          break;

        default:
          break;
      }

      FILE_WRITE(backup_file, gamepak_backup, backup_size);
      FILE_CLOSE(backup_file);
	  ReGBA_ProgressUpdate(1, 1);
	  ReGBA_ProgressFinalise();
      return 1;
    }
  }

  return 0;
}

void update_backup()
{
  if(backup_update != (write_backup_delay + 1))
    backup_update--;

  if(backup_update == 0)
  {
    save_backup();
    backup_update = write_backup_delay + 1;
  }
}

void update_backup_force()
{
  save_backup();
  backup_update = write_backup_delay + 1;
}

char *skip_spaces(char *line_ptr)
{
  while(*line_ptr == ' ')
    line_ptr++;

  return line_ptr;
}

int32_t parse_config_line(char *current_line, char *current_variable, char *current_value)
{
  char *line_ptr = current_line;
  char *line_ptr_new;

  if((current_line[0] == 0) || (current_line[0] == '#'))
    return -1;

  line_ptr_new = strchr(line_ptr, ' ');
  if(line_ptr_new == NULL)
    return -1;

  *line_ptr_new = 0;
  strcpy(current_variable, line_ptr);
  line_ptr_new = skip_spaces(line_ptr_new + 1);

  if(*line_ptr_new != '=')
    return -1;

  line_ptr_new = skip_spaces(line_ptr_new + 1);
  strcpy(current_value, line_ptr_new);
  line_ptr_new = current_value + strlen(current_value) - 1;
  if(*line_ptr_new == '\n')
  {
    line_ptr_new--;
    *line_ptr_new = 0;
  }

  if(*line_ptr_new == '\r')
    *line_ptr_new = 0;

  return 0;
}

static bool lookup_game_config(char *gamepak_title, char *gamepak_code, char *gamepak_maker, FILE_TAG_TYPE config_file);

int32_t load_game_config(char *gamepak_title, char *gamepak_code, char *gamepak_maker)
{
	char config_path[MAX_PATH];
	FILE_TAG_TYPE config_file;

	idle_loop_targets = 0;
	idle_loop_target_pc[0] = 0xFFFFFFFF;
	iwram_stack_optimize = 1;
	if (IsNintendoBIOS)
	{
		bios.rom[0x39] = 0x00; // Only Nintendo's BIOS requires this.
		bios.rom[0x2C] = 0x00; // Normmatt's open-source replacement doesn't.
	}
	flash_device_id = FLASH_DEVICE_MACRONIX_64KB;
	backup_type = BACKUP_NONE;

	ReGBA_ProgressInitialise(FILE_ACTION_APPLY_GAME_COMPATIBILITY);

	sprintf(config_path, "%s/%s", main_path, CONFIG_FILENAME);

	FILE_OPEN(config_file, config_path, READ);

	if(FILE_CHECK_VALID(config_file))
	{
		if (lookup_game_config(gamepak_title, gamepak_code, gamepak_maker, config_file))
		{
			ReGBA_ProgressUpdate(2, 2);
			ReGBA_ProgressFinalise();
			return 0;
		}
		else
			ReGBA_ProgressUpdate(1, 2);
	}

	if (ReGBA_GetBundledGameConfig(config_path))
	{
		FILE_OPEN(config_file, config_path, READ);

		if(FILE_CHECK_VALID(config_file))
		{
			if (lookup_game_config(gamepak_title, gamepak_code, gamepak_maker, config_file))
			{
				ReGBA_ProgressUpdate(2, 2);
				ReGBA_ProgressFinalise();
				return 0;
			}
		}
	}

	ReGBA_ProgressUpdate(2, 2);
	ReGBA_ProgressFinalise();

	return -1;
}

static bool lookup_game_config(char *gamepak_title, char *gamepak_code, char *gamepak_maker, FILE_TAG_TYPE config_file)
{
	char current_line[256];
	char current_variable[256];
	char current_value[256];

	while(fgets(current_line, 256, config_file))
	{
		if(parse_config_line(current_line, current_variable, current_value) != -1)
		{
			if(strcasecmp(current_variable, "game_name") != 0 || strcasecmp(current_value, gamepak_title) != 0)
				continue;

			if(!fgets(current_line, 256, config_file) || (parse_config_line(current_line, current_variable, current_value) == -1) ||
			   strcasecmp(current_variable, "game_code") != 0 || strcasecmp(current_value, gamepak_code) != 0)
				continue;

			if(!fgets(current_line, 256, config_file) || (parse_config_line(current_line, current_variable, current_value) == -1) ||
			   strcasecmp(current_variable, "vender_code") != 0 || strcasecmp(current_value, gamepak_maker) != 0)
				continue;

			while(fgets(current_line, 256, config_file))
			{
				if(parse_config_line(current_line, current_variable, current_value) != -1)
				{
					if(!strcasecmp(current_variable, "game_name"))
					{
						fclose(config_file);
						return 0;
					}

					if(!strcasecmp(current_variable, "idle_loop_eliminate_target"))
					{
						if(idle_loop_targets < MAX_IDLE_LOOPS)
						{
							idle_loop_target_pc[idle_loop_targets] =
							strtol(current_value, NULL, 16);
							idle_loop_targets++;
						}
					}

					if(!strcasecmp(current_variable, "iwram_stack_optimize") && !strcasecmp(current_value, "no"))
					{
						iwram_stack_optimize = 0;
					}

					if(!strcasecmp(current_variable, "flash_rom_type") && !strcasecmp(current_value, "128KB"))
					{
						flash_device_id = FLASH_DEVICE_MACRONIX_128KB;
					}

					// DBZLGCYGOKU2 のプロテクト回避
					// EEPROM_V124で特殊な物(現在判別不可) で指定すれば動作可
					if(!strcasecmp(current_variable, "save_type"))
					{
						if(!strcasecmp(current_value, "sram"))
							backup_type = BACKUP_SRAM;
						else if(!strcasecmp(current_value, "flash"))
							backup_type = BACKUP_FLASH;
						else if(!strcasecmp(current_value, "eeprom"))
							backup_type = BACKUP_EEPROM;
					}

					if(!strcasecmp(current_variable, "bios_rom_hack_39") &&
					   !strcasecmp(current_value, "yes") &&
					   IsNintendoBIOS)
					{
						bios.rom[0x39] = 0xC0;
					}

					if(!strcasecmp(current_variable, "bios_rom_hack_2C") &&
					   !strcasecmp(current_value, "yes") &&
					   IsNintendoBIOS)
					{
						bios.rom[0x2C] = 0x02;
					}
			}
		}

		FILE_CLOSE(config_file);
		return true;
		}
	}

	FILE_CLOSE(config_file);
	return false;
}

#define LOAD_ON_MEMORY FILE_TAG_INVALID

static ssize_t load_gamepak_raw(const char* name_path)
{
	FILE_TAG_TYPE gamepak_file;

	FILE_OPEN(gamepak_file, name_path, READ);

	if(FILE_CHECK_VALID(gamepak_file))
	{
		size_t gamepak_size = FILE_LENGTH(gamepak_file);
		uint8_t* EntireROM = ReGBA_MapEntireROM(gamepak_file, gamepak_size);
		if (EntireROM == NULL)
		{
			// Read in just enough for the header
			gamepak_file_large = gamepak_file;
			gamepak_ram_buffer_size = ReGBA_AllocateOnDemandBuffer((void**) &gamepak_rom);
			FILE_READ(gamepak_file, gamepak_rom, 0x100);
		}
		else
		{
			gamepak_ram_buffer_size = gamepak_size;
			gamepak_rom = EntireROM;
			// Do not close the file, because it is required by the mapping.
			// However, do not preserve it as gamepak_file_large either.
		}

		return gamepak_size;
	}

	return -1;
}

/*
 * Loads a GBA ROM from a file whose full path is in the first parameter.
 * Returns 0 on success and -1 on failure.
 */
ssize_t load_gamepak(const char* file_path)
{
	errno = 0;
	uint8_t magicbit[4];
	FILE_TAG_TYPE fd;
	if (IsGameLoaded) {
		update_backup_force();
		if(FILE_CHECK_VALID(gamepak_file_large))
		{
			FILE_CLOSE(gamepak_file_large);
			gamepak_file_large = FILE_TAG_INVALID;
			ReGBA_DeallocateROM(gamepak_rom);
		}
		else if (IsZippedROM)
		{
			ReGBA_DeallocateROM(gamepak_rom);
		}
		else
		{
			ReGBA_UnmapEntireROM(gamepak_rom);
		}
		IsGameLoaded = false;
		IsZippedROM = false;
		gamepak_size = 0;
		gamepak_rom = NULL;
		gamepak_ram_buffer_size = gamepak_ram_pages = 0;
	}

	ssize_t file_size;

	FILE_OPEN(fd, file_path, READ);
	if (fd)
	{
		FILE_READ(fd, &magicbit, 4);
		FILE_CLOSE(fd);

		if ((magicbit[0] == 0x50) && (magicbit[1] == 0x4B) && (magicbit[2] == 0x03) && (magicbit[3] == 0x04))
		{
			uint8_t* ROMBuffer = NULL;
			file_size = load_file_zip(file_path, &ROMBuffer);
			if(file_size == -2)
			{
				char extracted_file[MAX_FILE];
				sprintf(extracted_file, "%s/%s", main_path, ZIP_TMP);
				file_size = load_gamepak_raw(extracted_file);
			}
			else
			{
				gamepak_rom = ROMBuffer;
				gamepak_ram_buffer_size = file_size;
				IsZippedROM = true;
			}
		}
		else if (magicbit[3] == 0xEA)
		{
			file_size = load_gamepak_raw(file_path);
		}
		else
		{
			ReGBA_Trace("E: Unsupported file type; first 4 bytes are <%02X %02X %02X %02X>\n", magicbit[0], magicbit[1], magicbit[2], magicbit[3]);
			return -1;
		}
	}
	else
	{
		ReGBA_Trace("E: Failed to open %s\n", file_path);
		return -1;
	}

	if(file_size != -1)
	{
		gamepak_ram_pages = gamepak_ram_buffer_size / (32 * 1024);
		strcpy(CurrentGamePath, file_path);
		IsGameLoaded = true;

		frame_ticks = 0;
		init_rewind(); // Initialise rewinds for this game

		gamepak_size = (file_size + 0x7FFF) & ~0x7FFF;

		sram_size = SRAM_SIZE_32KB;
		flash_size = FLASH_SIZE_64KB;
		eeprom_size = EEPROM_512_BYTE;

		load_backup();

		memcpy(gamepak_title, gamepak_rom + 0xA0, 12);
		memcpy(gamepak_code, gamepak_rom + 0xAC, 4);
		memcpy(gamepak_maker, gamepak_rom + 0xB0, 2);
		gamepak_title[12] = '\0';
		gamepak_code[4] = '\0';
		gamepak_maker[2] = '\0';

		mem_save_flag = 0;

		load_game_config(gamepak_title, gamepak_code, gamepak_maker);

		reset_gba();
		reg[CHANGED_PC_STATUS] = 1;

		ReGBA_OnGameLoaded(file_path);

		return 0;
	}

	return -1;
}

uint8_t nintendo_bios_sha1[] = {
	0x30, 0x0c, 0x20, 0xdf, 0x67, 0x31, 0xa3, 0x39, 0x52, 0xde,
	0xd8, 0xc4, 0x36, 0xf7, 0xf1, 0x86, 0xd2, 0x5d, 0x34, 0x92,
};

int32_t load_bios(const char* name)
{
	FILE_TAG_TYPE bios_file;
	FILE_OPEN(bios_file, name, READ);

	if(FILE_CHECK_VALID(bios_file)) {
		FILE_READ(bios_file, bios.rom, 0x4000);
		FILE_CLOSE(bios_file);

		sha1nfo sha1;
		sha1_init(&sha1);
		sha1_write(&sha1, bios.rom, 0x4000);
		uint8_t* digest = sha1_result(&sha1);
		IsNintendoBIOS = memcmp(digest, nintendo_bios_sha1, SHA1_HASH_LENGTH) == 0;

		return 0;
	}

	return -1;
}

// DMA memory regions can be one of the following:
// IWRAM - 32kb offset from the contiguous iwram region.
// EWRAM - like segmented but with self modifying code check.
// VRAM - 96kb offset from the contiguous vram region, should take care
// Palette RAM - Converts palette entries when written to.
// OAM RAM - Sets OAM modified flag to true.
// I/O registers - Uses the I/O register function.
// of mirroring properly.
// Segmented RAM/ROM - a region >= 32kb, the translated address has to
//  be reloaded if it wraps around the limit (cartride ROM)
// Ext - should be handled by the memory read/write function.

// The following map determines the region of each (assumes DMA access
// is not done out of bounds)

typedef enum
{
  DMA_REGION_IWRAM,
  DMA_REGION_EWRAM,
  DMA_REGION_VRAM,
  DMA_REGION_PALETTE_RAM,
  DMA_REGION_OAM_RAM,
  DMA_REGION_IO,
  DMA_REGION_GAMEPAK,
  DMA_REGION_EXT,
  DMA_REGION_BIOS,
  DMA_REGION_NULL
} dma_region_type;

dma_region_type dma_region_map[16] =
{
  DMA_REGION_BIOS,          // 0x00 - BIOS
  DMA_REGION_NULL,          // 0x01 - Nothing
  DMA_REGION_EWRAM,         // 0x02 - EWRAM
  DMA_REGION_IWRAM,         // 0x03 - IWRAM
  DMA_REGION_IO,            // 0x04 - I/O registers
  DMA_REGION_PALETTE_RAM,   // 0x05 - palette RAM
  DMA_REGION_VRAM,          // 0x06 - VRAM
  DMA_REGION_OAM_RAM,       // 0x07 - OAM RAM
  DMA_REGION_GAMEPAK,       // 0x08 - gamepak ROM
  DMA_REGION_GAMEPAK,       // 0x09 - gamepak ROM
  DMA_REGION_GAMEPAK,       // 0x0A - gamepak ROM
  DMA_REGION_GAMEPAK,       // 0x0B - gamepak ROM
  DMA_REGION_GAMEPAK,       // 0x0C - gamepak ROM
  DMA_REGION_EXT,           // 0x0D - EEPROM
  DMA_REGION_EXT,           // 0x0E - gamepak SRAM/flash ROM
  DMA_REGION_EXT            // 0x0F - gamepak SRAM/flash ROM
};

static void dma_adjust_ptr_inc(uint32_t* GBAAddress, uint_fast8_t TransferBytes)
{
	*GBAAddress += TransferBytes;
}

static void dma_adjust_ptr_dec(uint32_t* GBAAddress, uint_fast8_t TransferBytes)
{
	*GBAAddress -= TransferBytes;
}

static void dma_adjust_ptr_fix(uint32_t* GBAAddress, uint_fast8_t TransferBytes)
{
}

static void dma_adjust_reg_writeback(DMA_TRANSFER_TYPE* DMA, uint32_t GBAAddress)
{
	DMA->dest_address = GBAAddress;
}

static void dma_adjust_reg_reload(DMA_TRANSFER_TYPE* DMA, uint32_t GBAAddress)
{
}

#define dma_smc_vars_src()                                                    \

#define dma_smc_vars_dest()                                                   \
  uint32_t smc_trigger = 0                                                    \

#define dma_vars_iwram(type)                                                  \
  dma_smc_vars_##type()                                                       \

#define dma_vars_vram(type)                                                   \
  dma_smc_vars_##type();                                                      \

#define dma_vars_palette_ram(type)                                            \

#define dma_oam_ram_src()                                                     \

#define dma_oam_ram_dest()                                                    \
  oam_update = 1                                                              \

#define dma_vars_oam_ram(type)                                                \
  dma_oam_ram_##type()                                                        \

#define dma_vars_io(type)                                                     \

#define dma_segmented_load_src()                                              \
  memory_map_read[src_current_region]                                         \

#define dma_segmented_load_dest()                                             \
  memory_map_write[dest_current_region]                                       \

#define dma_vars_gamepak(type)                                                \
  uint32_t type##_new_region;                                                 \
  uint32_t type##_current_region = type##_ptr >> 15;                          \
  uint8_t *type##_address_block = dma_segmented_load_##type();                \
  if(type##_address_block == NULL)                                            \
  {                                                                           \
    if((type##_ptr & 0x1FFFFFF) >= gamepak_size)                              \
      return return_value;                                                    \
    type##_address_block = load_gamepak_page(type##_current_region & 0x3FF);  \
  }                                                                           \

#define dma_vars_ewram(type)                                                  \
  dma_smc_vars_##type();                                                      \

#define dma_vars_bios(type)                                                   \

#define dma_vars_ext(type)                                                    \

#define dma_gamepak_check_region(type)                                        \
  type##_new_region = (type##_ptr >> 15);                                     \
  if(type##_new_region != type##_current_region)                              \
  {                                                                           \
    type##_current_region = type##_new_region;                                \
    type##_address_block = dma_segmented_load_##type();                       \
    if(type##_address_block == NULL)                                          \
    {                                                                         \
      type##_address_block =                                                  \
       load_gamepak_page(type##_current_region & 0x3FF);                      \
    }                                                                         \
  }                                                                           \

#define dma_read_iwram(type, transfer_size)                                   \
  read_value = ADDRESS##transfer_size(iwram_data, type##_ptr & 0x7FFF)        \

#define dma_read_vram(type, transfer_size)                                    \
  if (type##_ptr & 0x10000)                                                   \
    read_value = ADDRESS##transfer_size(vram, type##_ptr & 0x17FFF);          \
  else                                                                        \
    read_value = ADDRESS##transfer_size(vram, type##_ptr & 0xFFFF)            \

#define dma_read_io(type, transfer_size)                                      \
  read_value = ADDRESS##transfer_size(io_registers, type##_ptr & 0x7FFF)      \

#define dma_read_oam_ram(type, transfer_size)                                 \
  read_value = ADDRESS##transfer_size(oam_ram, type##_ptr & 0x3FF)            \

#define dma_read_palette_ram(type, transfer_size)                             \
  read_value = ADDRESS##transfer_size(palette_ram, type##_ptr & 0x3FF)        \

#define dma_read_ewram(type, transfer_size)                                   \
  read_value = ADDRESS##transfer_size(ewram_data, type##_ptr & 0x3FFFF)       \

#define dma_read_gamepak(type, transfer_size)                                 \
  dma_gamepak_check_region(type);                                             \
  read_value = ADDRESS##transfer_size(type##_address_block,                   \
   type##_ptr & 0x7FFF)                                                       \

// DMAing from the BIOS is funny, just returns 0..

#define dma_read_bios(type, transfer_size)                                    \
  read_value = 0                                                              \

#define dma_read_ext(type, transfer_size)                                     \
  read_value = read_memory##transfer_size(type##_ptr)                         \

#define dma_write_iwram(type, transfer_size)                                  \
  ADDRESS##transfer_size(iwram_data, type##_ptr & 0x7FFF) = read_value;       \
  {                                                                           \
    /* Get the Metadata Entry's [3], bits 0-1, to see if there's code at this \
     * location. See "doc/partial flushing of RAM code.txt" for more info. */ \
    uint16_t smc = iwram_metadata[(type##_ptr & 0x7FFC) | 3] & 0x3;           \
    if (smc) {                                                                \
      partial_clear_metadata(type##_ptr);                                     \
    }                                                                         \
    smc_trigger |= smc;                                                       \
  }                                                                           \

#define dma_write_vram(type, transfer_size)                                   \
  if (type##_ptr & 0x10000)                                                   \
    ADDRESS##transfer_size(vram, type##_ptr & 0x17FFF) = read_value;          \
  else                                                                        \
    ADDRESS##transfer_size(vram, type##_ptr & 0xFFFF) = read_value;           \
  {                                                                           \
    /* Get the Metadata Entry's [3], bits 0-1, to see if there's code at this \
     * location. See "doc/partial flushing of RAM code.txt" for more info. */ \
    uint16_t smc;                                                             \
    if (type##_ptr & 0x10000)                                                 \
      smc = vram_metadata[(type##_ptr & 0x17FFC) | 3] & 0x3;                  \
    else                                                                      \
      smc = vram_metadata[(type##_ptr & 0xFFFC) | 3] & 0x3;                   \
    if (smc) {                                                                \
      partial_clear_metadata(type##_ptr);                                     \
    }                                                                         \
    smc_trigger |= smc;                                                       \
  }                                                                           \

#define dma_write_io(type, transfer_size)                                     \
  write_io_register##transfer_size(type##_ptr & 0x3FF, read_value)            \

#define dma_write_oam_ram(type, transfer_size)                                \
  ADDRESS##transfer_size(oam_ram, type##_ptr & 0x3FF) = read_value            \

#define dma_write_palette_ram(type, transfer_size)                            \
  write_palette##transfer_size(type##_ptr & 0x3FF, read_value)                \

#define dma_write_ext(type, transfer_size)                                    \
  write_memory##transfer_size(type##_ptr, read_value)                         \

#define dma_write_ewram(type, transfer_size)                                  \
  ADDRESS##transfer_size(ewram_data, type##_ptr & 0x3FFFF) = read_value;      \
  {                                                                           \
    /* Get the Metadata Entry's [3], bits 0-1, to see if there's code at this \
     * location. See "doc/partial flushing of RAM code.txt" for more info. */ \
    uint16_t smc = ewram_metadata[(type##_ptr & 0x3FFFC) | 3] & 0x3;          \
    if (smc) {                                                                \
      partial_clear_metadata(type##_ptr);                                     \
    }                                                                         \
    smc_trigger |= smc;                                                       \
  }                                                                           \

#define dma_epilogue_iwram()                                                  \
  if(smc_trigger)                                                             \
  {                                                                           \
    /* Special return code indicating to retranslate to the CPU code */       \
    return_value |= CPU_ALERT_SMC;                                             \
  }                                                                           \

#define dma_epilogue_ewram()                                                  \
  if(smc_trigger)                                                             \
  {                                                                           \
    /* Special return code indicating to retranslate to the CPU code */       \
    return_value |= CPU_ALERT_SMC;                                             \
  }                                                                           \

#define dma_epilogue_vram()                                                   \
  if(smc_trigger)                                                             \
  {                                                                           \
    /* Special return code indicating to retranslate to the CPU code */       \
    return_value |= CPU_ALERT_SMC;                                             \
  }                                                                           \

#define dma_epilogue_io()                                                     \

#define dma_epilogue_oam_ram()                                                \

#define dma_epilogue_palette_ram()                                            \

#define dma_epilogue_GAMEPAK()                                                \

#define dma_epilogue_ext()                                                    \

#define print_line()                                                          \
  dma_print(src_op, dest_op, transfer_size, wb)                               \
  
#define dma_transfer_loop_region(src_region_type, dest_region_type, src_op,   \
 dest_op, transfer_size, wb)                                                  \
dma_transfer_##src_region_type##_##dest_region_type##_##transfer_size(        \
  dma, src_ptr, dest_ptr, length, src_op, dest_op, wb);                       \
break;                                                                        \

#define dma_transfer_loop_region_builder(src_region_type, dest_region_type,   \
 transfer_size)                                                               \
static CPU_ALERT_TYPE dma_transfer_##src_region_type##_##dest_region_type##_##transfer_size \
(                                                                             \
  DMA_TRANSFER_TYPE* dma,                                                     \
  uint32_t src_ptr,                                                           \
  uint32_t dest_ptr,                                                          \
  uint32_t length,                                                            \
  void (*src_op) (uint32_t*, uint_fast8_t),                                   \
  void (*dest_op) (uint32_t*, uint_fast8_t),                                  \
  void (*writeback_op) (DMA_TRANSFER_TYPE*, uint32_t)                         \
)                                                                             \
{                                                                             \
  uint_fast##transfer_size##_t read_value;                                    \
  uint32_t i;                                                                 \
  CPU_ALERT_TYPE return_value = CPU_ALERT_NONE;                               \
  dma_vars_##src_region_type(src);                                            \
  dma_vars_##dest_region_type(dest);                                          \
                                                                              \
  for(i = 0; i < length; i++)                                                 \
  {                                                                           \
    dma_read_##src_region_type(src, transfer_size);                           \
    dma_write_##dest_region_type(dest, transfer_size);                        \
    src_op(&src_ptr, transfer_size / 8);                                      \
    dest_op(&dest_ptr, transfer_size / 8);                                    \
  }                                                                           \
  dma->source_address = src_ptr;                                              \
  writeback_op(dma, dest_ptr);                                                \
  dma_epilogue_##dest_region_type();                                          \
  return return_value;                                                        \
}                                                                             \

// To IWRAM
dma_transfer_loop_region_builder(bios, iwram, 16)
dma_transfer_loop_region_builder(bios, iwram, 32)
dma_transfer_loop_region_builder(iwram, iwram, 16)
dma_transfer_loop_region_builder(iwram, iwram, 32)
dma_transfer_loop_region_builder(ewram, iwram, 16)
dma_transfer_loop_region_builder(ewram, iwram, 32)
dma_transfer_loop_region_builder(vram, iwram, 16)
dma_transfer_loop_region_builder(vram, iwram, 32)
dma_transfer_loop_region_builder(palette_ram, iwram, 16)
dma_transfer_loop_region_builder(palette_ram, iwram, 32)
dma_transfer_loop_region_builder(oam_ram, iwram, 16)
dma_transfer_loop_region_builder(oam_ram, iwram, 32)
dma_transfer_loop_region_builder(io, iwram, 16)
dma_transfer_loop_region_builder(io, iwram, 32)
dma_transfer_loop_region_builder(gamepak, iwram, 16)
dma_transfer_loop_region_builder(gamepak, iwram, 32)
dma_transfer_loop_region_builder(ext, iwram, 16)
dma_transfer_loop_region_builder(ext, iwram, 32)

// To EWRAM
dma_transfer_loop_region_builder(bios, ewram, 16)
dma_transfer_loop_region_builder(bios, ewram, 32)
dma_transfer_loop_region_builder(iwram, ewram, 16)
dma_transfer_loop_region_builder(iwram, ewram, 32)
dma_transfer_loop_region_builder(ewram, ewram, 16)
dma_transfer_loop_region_builder(ewram, ewram, 32)
dma_transfer_loop_region_builder(vram, ewram, 16)
dma_transfer_loop_region_builder(vram, ewram, 32)
dma_transfer_loop_region_builder(palette_ram, ewram, 16)
dma_transfer_loop_region_builder(palette_ram, ewram, 32)
dma_transfer_loop_region_builder(oam_ram, ewram, 16)
dma_transfer_loop_region_builder(oam_ram, ewram, 32)
dma_transfer_loop_region_builder(io, ewram, 16)
dma_transfer_loop_region_builder(io, ewram, 32)
dma_transfer_loop_region_builder(gamepak, ewram, 16)
dma_transfer_loop_region_builder(gamepak, ewram, 32)
dma_transfer_loop_region_builder(ext, ewram, 16)
dma_transfer_loop_region_builder(ext, ewram, 32)

// To VRAM
dma_transfer_loop_region_builder(bios, vram, 16)
dma_transfer_loop_region_builder(bios, vram, 32)
dma_transfer_loop_region_builder(iwram, vram, 16)
dma_transfer_loop_region_builder(iwram, vram, 32)
dma_transfer_loop_region_builder(ewram, vram, 16)
dma_transfer_loop_region_builder(ewram, vram, 32)
dma_transfer_loop_region_builder(vram, vram, 16)
dma_transfer_loop_region_builder(vram, vram, 32)
dma_transfer_loop_region_builder(palette_ram, vram, 16)
dma_transfer_loop_region_builder(palette_ram, vram, 32)
dma_transfer_loop_region_builder(oam_ram, vram, 16)
dma_transfer_loop_region_builder(oam_ram, vram, 32)
dma_transfer_loop_region_builder(io, vram, 16)
dma_transfer_loop_region_builder(io, vram, 32)
dma_transfer_loop_region_builder(gamepak, vram, 16)
dma_transfer_loop_region_builder(gamepak, vram, 32)
dma_transfer_loop_region_builder(ext, vram, 16)
dma_transfer_loop_region_builder(ext, vram, 32)

// To Palette RAM
dma_transfer_loop_region_builder(bios, palette_ram, 16)
dma_transfer_loop_region_builder(bios, palette_ram, 32)
dma_transfer_loop_region_builder(iwram, palette_ram, 16)
dma_transfer_loop_region_builder(iwram, palette_ram, 32)
dma_transfer_loop_region_builder(ewram, palette_ram, 16)
dma_transfer_loop_region_builder(ewram, palette_ram, 32)
dma_transfer_loop_region_builder(vram, palette_ram, 16)
dma_transfer_loop_region_builder(vram, palette_ram, 32)
dma_transfer_loop_region_builder(palette_ram, palette_ram, 16)
dma_transfer_loop_region_builder(palette_ram, palette_ram, 32)
dma_transfer_loop_region_builder(oam_ram, palette_ram, 16)
dma_transfer_loop_region_builder(oam_ram, palette_ram, 32)
dma_transfer_loop_region_builder(io, palette_ram, 16)
dma_transfer_loop_region_builder(io, palette_ram, 32)
dma_transfer_loop_region_builder(gamepak, palette_ram, 16)
dma_transfer_loop_region_builder(gamepak, palette_ram, 32)
dma_transfer_loop_region_builder(ext, palette_ram, 16)
dma_transfer_loop_region_builder(ext, palette_ram, 32)

// To OAM RAM
dma_transfer_loop_region_builder(bios, oam_ram, 16)
dma_transfer_loop_region_builder(bios, oam_ram, 32)
dma_transfer_loop_region_builder(iwram, oam_ram, 16)
dma_transfer_loop_region_builder(iwram, oam_ram, 32)
dma_transfer_loop_region_builder(ewram, oam_ram, 16)
dma_transfer_loop_region_builder(ewram, oam_ram, 32)
dma_transfer_loop_region_builder(vram, oam_ram, 16)
dma_transfer_loop_region_builder(vram, oam_ram, 32)
dma_transfer_loop_region_builder(palette_ram, oam_ram, 16)
dma_transfer_loop_region_builder(palette_ram, oam_ram, 32)
dma_transfer_loop_region_builder(oam_ram, oam_ram, 16)
dma_transfer_loop_region_builder(oam_ram, oam_ram, 32)
dma_transfer_loop_region_builder(io, oam_ram, 16)
dma_transfer_loop_region_builder(io, oam_ram, 32)
dma_transfer_loop_region_builder(gamepak, oam_ram, 16)
dma_transfer_loop_region_builder(gamepak, oam_ram, 32)
dma_transfer_loop_region_builder(ext, oam_ram, 16)
dma_transfer_loop_region_builder(ext, oam_ram, 32)

// To I/O RAM
dma_transfer_loop_region_builder(bios, io, 16)
dma_transfer_loop_region_builder(bios, io, 32)
dma_transfer_loop_region_builder(iwram, io, 16)
dma_transfer_loop_region_builder(iwram, io, 32)
dma_transfer_loop_region_builder(ewram, io, 16)
dma_transfer_loop_region_builder(ewram, io, 32)
dma_transfer_loop_region_builder(vram, io, 16)
dma_transfer_loop_region_builder(vram, io, 32)
dma_transfer_loop_region_builder(palette_ram, io, 16)
dma_transfer_loop_region_builder(palette_ram, io, 32)
dma_transfer_loop_region_builder(oam_ram, io, 16)
dma_transfer_loop_region_builder(oam_ram, io, 32)
dma_transfer_loop_region_builder(io, io, 16)
dma_transfer_loop_region_builder(io, io, 32)
dma_transfer_loop_region_builder(gamepak, io, 16)
dma_transfer_loop_region_builder(gamepak, io, 32)
dma_transfer_loop_region_builder(ext, io, 16)
dma_transfer_loop_region_builder(ext, io, 32)

// To extended RAM
dma_transfer_loop_region_builder(bios, ext, 16)
dma_transfer_loop_region_builder(bios, ext, 32)
dma_transfer_loop_region_builder(iwram, ext, 16)
dma_transfer_loop_region_builder(iwram, ext, 32)
dma_transfer_loop_region_builder(ewram, ext, 16)
dma_transfer_loop_region_builder(ewram, ext, 32)
dma_transfer_loop_region_builder(vram, ext, 16)
dma_transfer_loop_region_builder(vram, ext, 32)
dma_transfer_loop_region_builder(palette_ram, ext, 16)
dma_transfer_loop_region_builder(palette_ram, ext, 32)
dma_transfer_loop_region_builder(oam_ram, ext, 16)
dma_transfer_loop_region_builder(oam_ram, ext, 32)
dma_transfer_loop_region_builder(io, ext, 16)
dma_transfer_loop_region_builder(io, ext, 32)
dma_transfer_loop_region_builder(gamepak, ext, 16)
dma_transfer_loop_region_builder(gamepak, ext, 32)
dma_transfer_loop_region_builder(ext, ext, 16)
dma_transfer_loop_region_builder(ext, ext, 32)

#define dma_transfer_loop(src_op, dest_op, transfer_size, wb)                 \
{                                                                             \
  uint32_t src_region = src_ptr >> 24;                                        \
  uint32_t dest_region = dest_ptr >> 24;                                      \
  dma_region_type src_region_type = dma_region_map[src_region];               \
  dma_region_type dest_region_type = dma_region_map[dest_region];             \
                                                                              \
  switch(src_region_type | (dest_region_type << 4))                           \
  {                                                                           \
    case (DMA_REGION_BIOS | (DMA_REGION_IWRAM << 4)):                         \
      dma_transfer_loop_region(bios, iwram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_IWRAM << 4)):                        \
      dma_transfer_loop_region(iwram, iwram, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_IWRAM << 4)):                        \
      dma_transfer_loop_region(ewram, iwram, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_IWRAM << 4)):                         \
      dma_transfer_loop_region(vram, iwram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_IWRAM << 4)):                  \
      dma_transfer_loop_region(palette_ram, iwram, src_op, dest_op,           \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_IWRAM << 4)):                      \
      dma_transfer_loop_region(oam_ram, iwram, src_op, dest_op,               \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_IWRAM << 4)):                           \
      dma_transfer_loop_region(io, iwram, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_IWRAM << 4)):                      \
      dma_transfer_loop_region(gamepak, iwram, src_op, dest_op,               \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_IWRAM << 4)):                          \
      dma_transfer_loop_region(ext, iwram, src_op, dest_op,                   \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_BIOS | (DMA_REGION_EWRAM << 4)):                         \
      dma_transfer_loop_region(bios, ewram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_EWRAM << 4)):                        \
      dma_transfer_loop_region(iwram, ewram, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_EWRAM << 4)):                        \
      dma_transfer_loop_region(ewram, ewram, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_EWRAM << 4)):                         \
      dma_transfer_loop_region(vram, ewram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_EWRAM << 4)):                  \
      dma_transfer_loop_region(palette_ram, ewram, src_op, dest_op,           \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_EWRAM << 4)):                      \
      dma_transfer_loop_region(oam_ram, ewram, src_op, dest_op,               \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_EWRAM << 4)):                           \
      dma_transfer_loop_region(io, ewram, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_EWRAM << 4)):                      \
      dma_transfer_loop_region(gamepak, ewram, src_op, dest_op,               \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_EWRAM << 4)):                          \
      dma_transfer_loop_region(ext, ewram, src_op, dest_op,                   \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_BIOS | (DMA_REGION_VRAM << 4)):                          \
      dma_transfer_loop_region(bios, vram, src_op, dest_op,                   \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_VRAM << 4)):                         \
      dma_transfer_loop_region(iwram, vram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_VRAM << 4)):                         \
      dma_transfer_loop_region(ewram, vram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_VRAM << 4)):                          \
      dma_transfer_loop_region(vram, vram, src_op, dest_op,                   \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_VRAM << 4)):                   \
      dma_transfer_loop_region(palette_ram, vram, src_op, dest_op,            \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_VRAM << 4)):                       \
      dma_transfer_loop_region(oam_ram, vram, src_op, dest_op,                \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_VRAM << 4)):                            \
      dma_transfer_loop_region(io, vram, src_op, dest_op,                     \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_VRAM << 4)):                       \
      dma_transfer_loop_region(gamepak, vram, src_op, dest_op,                \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_VRAM << 4)):                           \
      dma_transfer_loop_region(ext, vram, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_BIOS | (DMA_REGION_PALETTE_RAM << 4)):                   \
      dma_transfer_loop_region(bios, palette_ram, src_op, dest_op,            \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_PALETTE_RAM << 4)):                  \
      dma_transfer_loop_region(iwram, palette_ram, src_op, dest_op,           \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_PALETTE_RAM << 4)):                  \
      dma_transfer_loop_region(ewram, palette_ram, src_op, dest_op,           \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_PALETTE_RAM << 4)):                   \
      dma_transfer_loop_region(vram, palette_ram, src_op, dest_op,            \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_PALETTE_RAM << 4)):            \
      dma_transfer_loop_region(palette_ram, palette_ram, src_op, dest_op,     \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_PALETTE_RAM << 4)):                \
      dma_transfer_loop_region(oam_ram, palette_ram, src_op, dest_op,         \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_PALETTE_RAM << 4)):                     \
      dma_transfer_loop_region(io, palette_ram, src_op, dest_op,              \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_PALETTE_RAM << 4)):                \
      dma_transfer_loop_region(gamepak, palette_ram, src_op, dest_op,         \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_PALETTE_RAM << 4)):                    \
      dma_transfer_loop_region(ext, palette_ram, src_op, dest_op,             \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_BIOS | (DMA_REGION_OAM_RAM << 4)):                       \
      dma_transfer_loop_region(bios, oam_ram, src_op, dest_op,                \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_OAM_RAM << 4)):                      \
      dma_transfer_loop_region(iwram, oam_ram, src_op, dest_op,               \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_OAM_RAM << 4)):                      \
      dma_transfer_loop_region(ewram, oam_ram, src_op, dest_op,               \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_OAM_RAM << 4)):                       \
      dma_transfer_loop_region(vram, oam_ram, src_op, dest_op,                \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_OAM_RAM << 4)):                \
      dma_transfer_loop_region(palette_ram, oam_ram, src_op, dest_op,         \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_OAM_RAM << 4)):                    \
      dma_transfer_loop_region(oam_ram, oam_ram, src_op, dest_op,             \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_OAM_RAM << 4)):                         \
      dma_transfer_loop_region(io, oam_ram, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_OAM_RAM << 4)):                    \
      dma_transfer_loop_region(gamepak, oam_ram, src_op, dest_op,             \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_OAM_RAM << 4)):                        \
      dma_transfer_loop_region(ext, oam_ram, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_BIOS | (DMA_REGION_IO << 4)):                            \
      dma_transfer_loop_region(bios, io, src_op, dest_op,                     \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_IO << 4)):                           \
      dma_transfer_loop_region(iwram, io, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_IO << 4)):                           \
      dma_transfer_loop_region(ewram, io, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_IO << 4)):                            \
      dma_transfer_loop_region(vram, io, src_op, dest_op,                     \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_IO << 4)):                     \
      dma_transfer_loop_region(palette_ram, io, src_op, dest_op,              \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_IO << 4)):                         \
      dma_transfer_loop_region(oam_ram, io, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_IO << 4)):                              \
      dma_transfer_loop_region(io, io, src_op, dest_op,                       \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_IO << 4)):                         \
      dma_transfer_loop_region(gamepak, io, src_op, dest_op,                  \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_IO << 4)):                             \
      dma_transfer_loop_region(ext, io, src_op, dest_op,                      \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_BIOS | (DMA_REGION_EXT << 4)):                           \
      dma_transfer_loop_region(bios, ext, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IWRAM | (DMA_REGION_EXT << 4)):                          \
      dma_transfer_loop_region(iwram, ext, src_op, dest_op,                   \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EWRAM | (DMA_REGION_EXT << 4)):                          \
      dma_transfer_loop_region(ewram, ext, src_op, dest_op,                   \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_VRAM | (DMA_REGION_EXT << 4)):                           \
      dma_transfer_loop_region(vram, ext, src_op, dest_op,                    \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_PALETTE_RAM | (DMA_REGION_EXT << 4)):                    \
      dma_transfer_loop_region(palette_ram, ext, src_op, dest_op,             \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_OAM_RAM | (DMA_REGION_EXT << 4)):                        \
      dma_transfer_loop_region(oam_ram, ext, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_IO | (DMA_REGION_EXT << 4)):                             \
      dma_transfer_loop_region(io, ext, src_op, dest_op,                      \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_GAMEPAK | (DMA_REGION_EXT << 4)):                        \
      dma_transfer_loop_region(gamepak, ext, src_op, dest_op,                 \
       transfer_size, wb);                                                    \
                                                                              \
    case (DMA_REGION_EXT | (DMA_REGION_EXT << 4)):                            \
      dma_transfer_loop_region(ext, ext, src_op, dest_op,                     \
       transfer_size, wb);                                                    \
  }                                                                           \
}                                                                             \


#define dma_transfer_expand(transfer_size)                                    \
  if (dma->source_direction != 3)                                             \
  {                                                                           \
    void (*src_op) (uint32_t*, uint_fast8_t);                                 \
    void (*dest_op) (uint32_t*, uint_fast8_t);                                \
    void (*writeback_op) (DMA_TRANSFER_TYPE*, uint32_t);                      \
    switch (dma->dest_direction)                                              \
    {                                                                         \
      case 0:                                                                 \
        dest_op = dma_adjust_ptr_inc;                                         \
        writeback_op = dma_adjust_reg_writeback;                              \
        break;                                                                \
                                                                              \
      case 1:                                                                 \
        dest_op = dma_adjust_ptr_dec;                                         \
        writeback_op = dma_adjust_reg_writeback;                              \
        break;                                                                \
                                                                              \
      case 2:                                                                 \
        dest_op = dma_adjust_ptr_fix;                                         \
        writeback_op = dma_adjust_reg_writeback;                              \
        break;                                                                \
                                                                              \
      case 3:                                                                 \
        dest_op = dma_adjust_ptr_inc;                                         \
        writeback_op = dma_adjust_reg_reload;                                 \
        break;                                                                \
                                                                              \
      default: /* To satisfy the compiler */                                  \
        dest_op = NULL;                                                       \
        writeback_op = NULL;                                                  \
        break;                                                                \
    }                                                                         \
    switch (dma->source_direction)                                            \
    {                                                                         \
      case 0:  src_op = dma_adjust_ptr_inc;  break;                           \
      case 1:  src_op = dma_adjust_ptr_dec;  break;                           \
      case 2:  src_op = dma_adjust_ptr_fix;  break;                           \
      default: src_op = NULL; /* compiler */ break;                           \
    }                                                                         \
    dma_transfer_loop(src_op, dest_op, transfer_size, writeback_op);          \
  }                                                                           \

CPU_ALERT_TYPE dma_transfer(DMA_TRANSFER_TYPE *dma)
{
  uint32_t length = dma->length;
  uint32_t src_ptr = dma->source_address;
  uint32_t dest_ptr = dma->dest_address;
  CPU_ALERT_TYPE return_value = CPU_ALERT_NONE;

  // Technically this should be done for source and destination, but
  // chances are this is only ever used (probably mistakingly!) for dest.
  // The only game I know of that requires this is Lucky Luke.
  if((dest_ptr >> 24) != ((dest_ptr + length - 1) >> 24))
  {
    uint32_t first_length = ((dest_ptr & 0xFF000000) + 0x1000000) - dest_ptr;
    uint32_t second_length = length - first_length;
    dma->length = first_length;

    dma_transfer(dma);

    dma->length = length;

    length = second_length;
    dest_ptr += first_length;
    src_ptr += first_length;
  }

  if(dma->length_type == DMA_16BIT)
  {
    src_ptr &= ~0x01;
    dest_ptr &= ~0x01;
    dma_transfer_expand(16);
  }
  else
  {
    src_ptr &= ~0x03;
    dest_ptr &= ~0x03;
    dma_transfer_expand(32);
  }

  if((dma->repeat_type == DMA_NO_REPEAT) ||
   (dma->start_type == DMA_START_IMMEDIATELY))
  {
    dma->start_type = DMA_INACTIVE;
    ADDRESS16(io_registers, (dma->dma_channel * 12) + 0xBA) &= (~0x8000);
  }

  if(dma->irq)
  {
    raise_interrupt(IRQ_DMA0 << dma->dma_channel);
    return_value |= CPU_ALERT_IRQ;
  }

  return return_value;
}

// Be sure to do this after loading ROMs.

#define map_region(type, start, end, mirror_blocks, region)                   \
  for(map_offset = (start) / 0x8000; map_offset <                             \
   ((end) / 0x8000); map_offset++)                                            \
  {                                                                           \
    memory_map_##type[map_offset] =                                           \
     ((uint8_t *)region) + ((map_offset % mirror_blocks) * 0x8000);           \
  }                                                                           \

#define map_null(type, start, end)                                            \
  for(map_offset = start / 0x8000; map_offset < (end / 0x8000);               \
   map_offset++)                                                              \
  {                                                                           \
    memory_map_##type[map_offset] = NULL;                                     \
  }                                                                           \

#define map_ram_region(type, start, end, mirror_blocks, region)               \
  for(map_offset = (start) / 0x8000; map_offset <                             \
   ((end) / 0x8000); map_offset++)                                            \
  {                                                                           \
    memory_map_##type[map_offset] =                                           \
     ((uint8_t *)region) + ((map_offset % mirror_blocks) * 0x10000) + 0x8000; \
  }                                                                           \

#define map_vram(type)                                                        \
  for(map_offset = 0x6000000 / 0x8000; map_offset < (0x7000000 / 0x8000);     \
   map_offset += 4)                                                           \
  {                                                                           \
    memory_map_##type[map_offset] = vram;                                     \
    memory_map_##type[map_offset + 1] = vram + 0x8000;                        \
    memory_map_##type[map_offset + 2] = vram + (0x8000 * 2);                  \
    memory_map_##type[map_offset + 3] = vram + (0x8000 * 2);                  \
  }                                                                           \

uint32_t evict_gamepak_page()
{
	// We will evict the page with index gamepak_next_swap, a bit like a ring
	// buffer.
	uint32_t page_index = gamepak_next_swap;
	gamepak_next_swap++;
	if (gamepak_next_swap >= gamepak_ram_pages)
		gamepak_next_swap = 0;
	uint16_t physical_index = gamepak_memory_map[page_index];

	memory_map_read[(0x8000000 / (32 * 1024)) + physical_index] = NULL;
	memory_map_read[(0xA000000 / (32 * 1024)) + physical_index] = NULL;
	memory_map_read[(0xC000000 / (32 * 1024)) + physical_index] = NULL;

#if TRACE_MEMORY
	ReGBA_Trace("T: Evicting virtual page %u", page_index);
#endif
	
	return page_index;
}

uint8_t *load_gamepak_page(uint16_t physical_index)
{
	if (memory_map_read[(0x08000000 / (32 * 1024)) + (uint32_t) physical_index] != NULL)
	{
#if TRACE_MEMORY
		ReGBA_Trace("T: Not reloading already loaded Game Pak page %u (%08X..%08X)", (uint32_t) physical_index, 0x08000000 + physical_index * (32 * 1024), 0x08000000 + (uint32_t) physical_index * (32 * 1024) + 0x7FFF);
#endif
		return memory_map_read[(0x08000000 / (32 * 1024)) + (uint32_t) physical_index];
	}
#if TRACE_MEMORY
	ReGBA_Trace("T: Loading Game Pak page %u (%08X..%08X)", (uint32_t) physical_index, 0x08000000 + (uint32_t) physical_index * (32 * 1024), 0x08000000 + (uint32_t) physical_index * (32 * 1024) + 0x7FFF);
#endif
	if((uint32_t) physical_index >= (gamepak_size >> 15))
		return gamepak_rom;

	uint16_t page_index = evict_gamepak_page();
	uint32_t page_offset = (uint32_t) page_index * (32 * 1024);
	uint8_t *swap_location = gamepak_rom + page_offset;

	gamepak_memory_map[page_index] = physical_index;

	FILE_SEEK(gamepak_file_large, (off_t) physical_index * (32 * 1024), SEEK_SET);
	FILE_READ(gamepak_file_large, swap_location, (32 * 1024));

	memory_map_read[(0x8000000 / (32 * 1024)) + physical_index] =
		memory_map_read[(0xA000000 / (32 * 1024)) + physical_index] =
		memory_map_read[(0xC000000 / (32 * 1024)) + physical_index] = swap_location;

	// If RTC is active page the RTC register bytes so they can be read
	if((rtc_state != RTC_DISABLED) && (physical_index == 0))
	{
		memcpy(swap_location + 0xC4, rtc_registers, sizeof(rtc_registers));
	}

	return swap_location;
}

void init_memory_gamepak()
{
  uint32_t map_offset = 0;

  if (FILE_CHECK_VALID(gamepak_file_large))
  {
    // Large ROMs get special treatment because they
    // can't fit into the ROM buffer.
    // The size of this buffer varies per platform, and may actually
    // fit all of the ROM, in which case this is dead code.
    memset(gamepak_memory_map, 0, sizeof(gamepak_memory_map));
    gamepak_next_swap = 0;

    map_null(read, 0x8000000, 0xE000000);
  }
  else
  {
    map_region(read, 0x8000000, 0x8000000 + gamepak_size, 1024, gamepak_rom);
    map_null(read, 0x8000000 + gamepak_size, 0xA000000);
    map_region(read, 0xA000000, 0xA000000 + gamepak_size, 1024, gamepak_rom);
    map_null(read, 0xA000000 + gamepak_size, 0xC000000);
    map_region(read, 0xC000000, 0xC000000 + gamepak_size, 1024, gamepak_rom);
    map_null(read, 0xC000000 + gamepak_size, 0xE000000);
  }
}

void init_memory()
{
  uint32_t map_offset = 0;

  memory_regions[0x00] = (uint8_t *)bios.rom;
  memory_regions[0x01] = (uint8_t *)bios.rom;
  memory_regions[0x02] = (uint8_t *)ewram_data;
  memory_regions[0x03] = (uint8_t *)iwram_data;
  memory_regions[0x04] = (uint8_t *)io_registers;
  memory_regions[0x05] = (uint8_t *)palette_ram;
  memory_regions[0x06] = (uint8_t *)vram;
  memory_regions[0x07] = (uint8_t *)oam_ram;
  memory_regions[0x08] = (uint8_t *)gamepak_rom;
  memory_regions[0x09] = (uint8_t *)(gamepak_rom + 0xFFFFFF);
  memory_regions[0x0A] = (uint8_t *)gamepak_rom;
  memory_regions[0x0B] = (uint8_t *)(gamepak_rom + 0xFFFFFF);
  memory_regions[0x0C] = (uint8_t *)gamepak_rom;
  memory_regions[0x0D] = (uint8_t *)(gamepak_rom + 0xFFFFFF);
  memory_regions[0x0E] = (uint8_t *)gamepak_backup;

  memory_limits[0x00] = 0x3FFF;
  memory_limits[0x01] = 0x3FFF;
  memory_limits[0x02] = 0x3FFFF;
  memory_limits[0x03] = 0x7FFF;
  memory_limits[0x04] = 0x7FFF;
  memory_limits[0x05] = 0x3FF;
  memory_limits[0x06] = 0x17FFF;
  memory_limits[0x07] = 0x3FF;
  memory_limits[0x08] = 0x1FFFFFF;
  memory_limits[0x09] = 0x1FFFFFF;
  memory_limits[0x0A] = 0x1FFFFFF;
  memory_limits[0x0B] = 0x1FFFFFF;
  memory_limits[0x0C] = 0x1FFFFFF;
  memory_limits[0x0D] = 0x1FFFFFF;
  memory_limits[0x0E] = 0xFFFF;

  // Fill memory map regions, areas marked as NULL must be checked directly
  map_region(read, 0x0000000, 0x1000000, 1, bios.rom);
  map_null(read, 0x1000000, 0x2000000);
  map_region(read, 0x2000000, 0x3000000, 8, ewram_data);
  map_region(read, 0x3000000, 0x4000000, 1, iwram_data);
  map_region(read, 0x4000000, 0x5000000, 1, io_registers);
  map_null(read, 0x5000000, 0x6000000);
  map_vram(read);
  map_null(read, 0x7000000, 0x8000000);
  init_memory_gamepak();
  map_null(read, 0xE000000, 0x10000000);

  // Fill memory map regions, areas marked as NULL must be checked directly
  map_null(write, 0x0000000, 0x2000000);
  map_region(write, 0x2000000, 0x3000000, 8, ewram_data);
  map_region(write, 0x3000000, 0x4000000, 1, iwram_data);
  map_null(write, 0x4000000, 0x5000000);
  map_null(write, 0x5000000, 0x6000000);

  map_vram(write);

  map_null(write, 0x7000000, 0x8000000);
  map_null(write, 0x8000000, 0xE000000);
  map_null(write, 0xE000000, 0x10000000);

  memset(io_registers, 0, sizeof(io_registers));
  memset(oam_ram, 0, sizeof(oam_ram));
  memset(palette_ram, 0, sizeof(palette_ram));
  memset(iwram_data, 0, sizeof(iwram_data));
  memset(ewram_data, 0, sizeof(ewram_data));
  memset(vram, 0, sizeof(vram));

  io_registers[REG_DISPCNT] = 0x80;
  io_registers[REG_P1] = 0x3FF;
  io_registers[REG_BG2PA] = 0x100;
  io_registers[REG_BG2PD] = 0x100;
  io_registers[REG_BG3PA] = 0x100;
  io_registers[REG_BG3PD] = 0x100;
  //io_registers[REG_RCNT] = 0x8000;
  //io_registers[REG_SIOCNT] = 0x000C;

  io_registers[REG_RCNT] = 0x0000;
  io_registers[REG_SIOCNT] = 0x0004;

  flash_bank_offset = 0;
  flash_command_position = 0;
  eeprom_mode = EEPROM_BASE_MODE;
  eeprom_address = 0;
  eeprom_counter = 0;

  flash_mode = FLASH_BASE_MODE;

  rtc_state = RTC_DISABLED;
  memset(rtc_registers, 0, sizeof(rtc_registers));
  bios_read_protect = 0xe129f000;

  mem_save_flag = 0;
}

void bios_region_read_allow()
{
  memory_map_read[0] = bios.rom;
}

void bios_region_read_protect()
{
  memory_map_read[0] = NULL;
}

// TODO:savestate_fileは必要ないので削除する
// type = read_mem / write_mem
#define savestate_block(type)                                                 \
  cpu_##type##_savestate();                                                   \
  input_##type##_savestate();                                                 \
  main_##type##_savestate();                                                  \
  memory_##type##_savestate();                                                \
  sound_##type##_savestate();                                                 \
  video_##type##_savestate();                                                 \

static unsigned int rewind_queue_wr_len;
unsigned int rewind_queue_len;
void init_rewind(void)
{
	rewind_queue_wr_len = 0;
	rewind_queue_len = 0;
}

void savestate_rewind(void)
{
	g_state_buffer_ptr = SAVESTATE_REWIND_MEM + rewind_queue_wr_len * SAVESTATE_REWIND_LEN;
	savestate_block(write_mem);

	rewind_queue_wr_len += 1;
	if(rewind_queue_wr_len >= SAVESTATE_REWIND_NUM)
		rewind_queue_wr_len = 0;

	if(rewind_queue_len < SAVESTATE_REWIND_NUM)
		rewind_queue_len += 1;
}

void loadstate_rewind(void)
{
	int i;

	if(rewind_queue_len == 0)  // There's no rewind data
		return;

	//Load latest recently
	rewind_queue_len--;
	if(rewind_queue_wr_len == 0)
		rewind_queue_wr_len = SAVESTATE_REWIND_NUM - 1;
	else
		rewind_queue_wr_len--;

	g_state_buffer_ptr = SAVESTATE_REWIND_MEM + rewind_queue_wr_len * SAVESTATE_REWIND_LEN;
	savestate_block(read_mem);

	clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_LOADING_STATE);
	clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_LOADING_STATE);
	clear_metadata_area(METADATA_AREA_VRAM, CLEAR_REASON_LOADING_STATE);

	oam_update = 1;
	gbc_sound_update = 1;

	// Oops, these contain raw pointers
	for(i = 0; i < 4; i++)
	{
		gbc_sound_channel[i].sample_data = square_pattern_duty[2];
	}

	reg[CHANGED_PC_STATUS] = 1;
}

/*
 * Loads a saved state, given its slot number.
 * Returns 0 on success, non-zero on failure.
 */
uint32_t load_state(uint32_t SlotNumber)
{
	FILE_TAG_TYPE savestate_file;
	size_t i;

	char SavedStateFilename[MAX_PATH + 1];
	if (!ReGBA_GetSavedStateFilename(SavedStateFilename, CurrentGamePath, SlotNumber))
	{
		ReGBA_Trace("W: Failed to get the name of saved state #%d for '%s'", SlotNumber, CurrentGamePath);
		return 0;
	}

	ReGBA_ProgressInitialise(FILE_ACTION_LOAD_STATE);

	FILE_OPEN(savestate_file, SavedStateFilename, READ);
	if(FILE_CHECK_VALID(savestate_file))
    {
		uint8_t header[SVS_HEADER_SIZE];
		errno = 0;
		i = FILE_READ(savestate_file, header, SVS_HEADER_SIZE);
		ReGBA_ProgressUpdate(SVS_HEADER_SIZE, SAVESTATE_SIZE);
		if (i < SVS_HEADER_SIZE) {
			FILE_CLOSE(savestate_file);
			ReGBA_ProgressFinalise();
			return 1; // Failed to fully read the file
		}
		if (!(
			memcmp(header, SVS_HEADER_E, SVS_HEADER_SIZE) == 0
		||	memcmp(header, SVS_HEADER_F, SVS_HEADER_SIZE) == 0
		)) {
			FILE_CLOSE(savestate_file);
			ReGBA_ProgressFinalise();
			return 2; // Bad saved state format
		}

		i = FILE_READ(savestate_file, savestate_write_buffer, SAVESTATE_SIZE);
		ReGBA_ProgressUpdate(SAVESTATE_SIZE, SAVESTATE_SIZE);
		ReGBA_ProgressFinalise();
		FILE_CLOSE(savestate_file);
		if (i < SAVESTATE_SIZE)
			return 1; // Failed to fully read the file

		g_state_buffer_ptr = savestate_write_buffer + sizeof(struct ReGBA_RTC) + (240 * 160 * sizeof(uint16_t)) + 2;

		savestate_block(read_mem);

		// Perform fixups by saved-state version.

		// 1.0e: Uses precalculated variables with SOUND_FREQUENCY equal to
		// 65536. Port these values forward to 1.0f where it's 88200.
		if (memcmp(header, SVS_HEADER_E, SVS_HEADER_SIZE) == 0)
		{
			unsigned int n;
			for (n = 0; n < 4; n++) {
				gbc_sound_channel[n].frequency_step = FLOAT_TO_FP16_16(FP16_16_TO_FLOAT(gbc_sound_channel[n].frequency_step) * 65536.0f / SOUND_FREQUENCY);
			}
			for (n = 0; n < 2; n++) {
				timer[n].frequency_step = FLOAT_TO_FP16_16(FP16_16_TO_FLOAT(timer[n].frequency_step) * 65536.0f / SOUND_FREQUENCY);
			}
		}

		// End fixups.

		clear_metadata_area(METADATA_AREA_IWRAM, CLEAR_REASON_LOADING_STATE);
		clear_metadata_area(METADATA_AREA_EWRAM, CLEAR_REASON_LOADING_STATE);
		clear_metadata_area(METADATA_AREA_VRAM, CLEAR_REASON_LOADING_STATE);

		oam_update = 1;
		gbc_sound_update = 1;

		// Oops, these contain raw pointers
		for(i = 0; i < 4; i++)
		{
			gbc_sound_channel[i].sample_data = square_pattern_duty[2];
		}

		reg[CHANGED_PC_STATUS] = 1;

		return 0;
	}
	else
	{
		ReGBA_ProgressFinalise();
		return 1;
	}
}


/*--------------------------------------------------------
  保存即时存档
  input
    uint32_t SlotNumber           存档槽
    uint16_t *screen_capture      存档索引画面
  return
    0 失败
    1 成功
--------------------------------------------------------*/
uint32_t save_state(uint32_t SlotNumber, const uint16_t *screen_capture)
{
  FILE_TAG_TYPE savestate_file;
  struct ReGBA_RTC Time;
  uint32_t ret = 1;

	char SavedStateFilename[MAX_PATH + 1];
	if (!ReGBA_GetSavedStateFilename(SavedStateFilename, CurrentGamePath, SlotNumber))
	{
		ReGBA_Trace("W: Failed to get the name of saved state #%d for '%s'", SlotNumber, CurrentGamePath);
		return 0;
	}
	
	ReGBA_ProgressInitialise(FILE_ACTION_SAVE_STATE);

  g_state_buffer_ptr = savestate_write_buffer;

  ReGBA_LoadRTCTime(&Time);
  FILE_WRITE_MEM_VARIABLE(g_state_buffer_ptr, Time);

  FILE_WRITE_MEM(g_state_buffer_ptr, screen_capture, 240 * 160 * 2);
  //Identify ID
  *(g_state_buffer_ptr++)= 0x5A;
  *(g_state_buffer_ptr++)= 0x3C;

  savestate_block(write_mem);

  FILE_OPEN(savestate_file, SavedStateFilename, WRITE);
  if(FILE_CHECK_VALID(savestate_file))
  {
    if (FILE_WRITE(savestate_file, SVS_HEADER_F, SVS_HEADER_SIZE) < SVS_HEADER_SIZE)
	{
		ret = 0;
		goto fail;
	}
	ReGBA_ProgressUpdate(SVS_HEADER_SIZE, SAVESTATE_SIZE);
    if (FILE_WRITE(savestate_file, savestate_write_buffer, sizeof(savestate_write_buffer)) < sizeof(savestate_write_buffer))
	{
		ret = 0;
		goto fail;
	}
	ReGBA_ProgressUpdate(SAVESTATE_SIZE, SAVESTATE_SIZE);
fail:
    FILE_CLOSE(savestate_file);
  }
  else
  {
    ret = 0;
  }
  ReGBA_ProgressFinalise();

  mem_save_flag = 1;

  return ret;
}

#define SAVESTATE_READ_MEM_FILENAME(name)                                     \
    FILE_READ_MEM_ARRAY(g_state_buffer_ptr, name);                            \

#define SAVESTATE_WRITE_MEM_FILENAME(name)                                    \
    FILE_WRITE_MEM_ARRAY(g_state_buffer_ptr, name);                       \

#define memory_savestate_body(type)                                           \
{                                                                             \
  uint32_t i;                                                                 \
  char fullname[512];                                                         \
  memset(fullname, 0, sizeof(fullname));                                      \
                                                                              \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, backup_type);                    \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, sram_size);                      \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, flash_mode);                     \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, flash_command_position);         \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, flash_bank_offset);              \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, flash_device_id);                \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, flash_manufacturer_id);          \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, flash_size);                     \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, eeprom_size);                    \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, eeprom_mode);                    \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, eeprom_address_length);          \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, eeprom_address);                 \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, eeprom_counter);                 \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, rtc_state);                      \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, rtc_write_mode);                 \
  FILE_##type##_ARRAY(g_state_buffer_ptr, rtc_registers);                     \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, rtc_command);                    \
  FILE_##type##_ARRAY(g_state_buffer_ptr, rtc_data);                          \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, rtc_status);                     \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, rtc_data_bytes);                 \
  FILE_##type##_VARIABLE(g_state_buffer_ptr, rtc_bit_count);                  \
  FILE_##type##_ARRAY(g_state_buffer_ptr, eeprom_buffer);                     \
  SAVESTATE_##type##_FILENAME(fullname);                                      \
  FILE_##type##_ARRAY(g_state_buffer_ptr, dma);                               \
                                                                              \
  FILE_##type##_ARRAY(g_state_buffer_ptr, iwram_data);                        \
  FILE_##type##_ARRAY(g_state_buffer_ptr, ewram_data);                        \
  FILE_##type(g_state_buffer_ptr, vram, 0x18000);                             \
  FILE_##type(g_state_buffer_ptr, oam_ram, 0x400);                            \
  FILE_##type(g_state_buffer_ptr, palette_ram, 0x400);                        \
  FILE_##type(g_state_buffer_ptr, io_registers, 0x8000);                      \
}                                                                             \

void memory_read_mem_savestate()
memory_savestate_body(READ_MEM)

void memory_write_mem_savestate()
memory_savestate_body(WRITE_MEM)

uint32_t get_sio_mode(uint16_t io1, uint16_t io2)
{
  if(!(io2 & 0x8000))
  {
    switch(io1 & 0x3000)
    {
      case 0x0000:
        return SIO_NORMAL8;
      case 0x1000:
        return SIO_NORMAL32;
      case 0x2000:
        return SIO_MULTIPLAYER;
      case 0x3000:
        return SIO_UART;
    }
  }
  if(io2 & 0x4000)
  {
    return SIO_JOYBUS;
  }
  return SIO_GP;
}

// multiモードのデータを送る
uint32_t send_adhoc_multi()
{
  return 0;
}
