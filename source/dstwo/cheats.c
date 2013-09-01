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
 * cheat.c
 * チートの処理
 ******************************************************************************/

/******************************************************************************
 * ヘッダファイルの読込み
 ******************************************************************************/
#include "common.h"

u32 g_num_cheats;

static void decrypt_gsa_code(int *address_ptr, int *value_ptr, CHEAT_VARIANT_ENUM cheat_variant);
static void process_cheat_gs1(CHEAT_TYPE *cheat);
static void process_cheat_gs3(CHEAT_TYPE *cheat);

static void decrypt_gsa_code(int *address_ptr, int *value_ptr, CHEAT_VARIANT_ENUM cheat_variant)
{
  u32 i;
  u32 address = *address_ptr;
  u32 value = *value_ptr;
  u32 r = 0xc6ef3720;
  const u32 seeds_v1[4] = {0x09f4fbbd, 0x9681884a, 0x352027e9, 0xf3dee5a7};
  const u32 seeds_v3[4] = {0x7aa9648f, 0x7fae6994, 0xc0efaad5, 0x42712c57};

  u32 *seeds;

  if(cheat_variant == CHEAT_TYPE_GAMESHARK_V1)
    seeds = (u32 *)seeds_v1;
  else
    seeds = (u32 *)seeds_v3;

  for(i = 0; i < 32; i++)
  {
    value   -= ((address << 4) + seeds[2]) ^ (address + r) ^ ((address >> 5) + seeds[3]);
    address -= ((value   << 4) + seeds[0]) ^ (value   + r) ^ ((value   >> 5) + seeds[1]);
    r -= 0x9e3779b9;
  }

  *address_ptr = address;
  *value_ptr = value;
}

static unsigned char* check_is_cht(unsigned char *str)
{
	unsigned char *pt, *pt1;

	if(*str == '\0') return NULL;

	pt = str;
	while(*pt == ' ') pt++;			//Skip leading space
	if(*pt != '[') return NULL;		//valid entry should be:[string]

	pt1 = strrchr(str, ']');
	if(pt1 == NULL) return NULL;

	while(*(--pt1) == ' ');
	*(pt1+1) = '\0';					//Cut trailing space between string and ']'

	while(*(++pt) == ' ');			//Cut space between '[' and string

	return pt;
}

static unsigned char* sscanf_cht_value(unsigned char* str, unsigned int *value)
{
	unsigned char *pt;
	unsigned int tmp;
	unsigned char ch;

	pt = str;
	while(*pt == ' ') pt++;
	
	if(*pt == '\0') return NULL;
	if(*pt == 0x0D || *pt == 0x0A) return NULL;

	tmp = 0;
	while(*pt)
	{
		ch = *pt;
		if(ch >= 'a' && ch <= 'f') ch = ch - 'a' + 0xa;
		else if(ch >= 'A' && ch <= 'F') ch = ch - 'A' + 0xa;
		else if(ch >= '0' && ch <= '9') ch = ch - '0';
		else break;

		tmp = (tmp << 4) | ch;
		pt++;
	}

	*value = tmp;
	return pt;
}

int string2utf8(unsigned char *src, unsigned char* dst)
{
	unsigned char *pt;
	unsigned char ch;
	unsigned short ucode;
	unsigned int type;
	unsigned int len;

	len = 0;
	type = 0;
	pt = src;
	while(*pt)
	{
		pt = utf8decode(pt, &ucode);
		if(ucode < 0x4e00)
		{
			if(ucode == 0 || ucode > 0x7F)
			{
				type = 1;
				break;
			}
		}
		else if(ucode > 0x9FCF)
		{
			type = 1;
			break;
		}
		else
			len++;

		if(len >= 5) break;	//There is enough UTF8, so it is, to save time(>_*)
	}

	//UTF8
	if(type == 0)	return 0;

	//GBK to UTF8
	while(*src)
	{
		ch = *src;
		if(ch < 0x80)
		{
			*dst++= ch;
			src ++;
		} 
		else
		{
			ucode = charsets_gbk_to_ucs(src);

			if (ucode < 0x800) //two byte
			{
				*dst++ = 0xC0 | ((ucode >> 6) & 0x1F);
				*dst++ = 0x80 | (ucode & 0x3F);
			}
			else /* if(ucode < 0x10000) */ //3 bytes
			{
				*dst++ = 0xE0 | (ucode >> 12);
				*dst++ = 0x80 | ((ucode >>6) & 0x3F);
				*dst++ = 0x80 | (ucode & 0x3F);
			}

			src += 2;
		}
	}
	*dst = '\0';

	return 1;
}

char cheat_path_loaded[MAX_PATH];
int load_cheats_name(char* cheats_msg, char** cheats_msg_pt, unsigned int num)
{
	FILE *cheats_file;
	char current_line[256];
	char current_line_tmp[256];
	int len, m;
	unsigned char* pt;
	unsigned char* name_ptr;

	cheats_file = fopen(cheat_path_loaded, "r");
	if(cheats_file == NULL)
		return -1;

	len = 0;
	m= 0;
	cheats_msg_pt[m] = cheats_msg;

	while(fgets(current_line, 256, cheats_file))
    {
		if((name_ptr = check_is_cht(current_line)) != NULL)
		if(strcasecmp(name_ptr, "gameinfo"))
		{
			pt = name_ptr;
			if(string2utf8(pt, current_line_tmp) != 0)
				pt = current_line_tmp;

			len += strlen(pt) +1;
			strcpy(cheats_msg_pt[m++], pt);
			if(m >= num) break;
			cheats_msg_pt[m] = cheats_msg + len;

			while(fgets(current_line, 256, cheats_file))
			{
				if(strlen(current_line) < 3)
					break;

				pt = strchr(current_line, '=');
				if(pt == NULL) continue;

				while(*(--pt) == ' ');
				*(pt+1) = '\0';

				pt = current_line;
				while(*pt == ' ') pt++;

				if(string2utf8(pt, current_line_tmp) != 0)
					pt = current_line_tmp;

				len += strlen(pt) +1;
				strcpy(cheats_msg_pt[m++], pt);

				if(m >= num) break;
				cheats_msg_pt[m] = cheats_msg + len;
	        }

			if(m >= num) break;
		}
    }

    fclose(cheats_file);
	return 0;
}


void add_cheats(char *cheats_filename)
{
  FILE *cheats_file;
  char current_line[256];
  char current_line_tmp[256];
  char *name_ptr;
  unsigned int *cheat_code_ptr;
  int address = 0;
  int value = 0;
  u32 num_cheat_lines;
  u32 cheat_name_length;
  CHEAT_VARIANT_ENUM current_cheat_variant;
  char cheat_path[MAX_PATH];

  g_num_cheats = 0;

  if (DEFAULT_CHEAT_DIR != NULL) {
    sprintf(cheat_path, "%s/%s", DEFAULT_CHEAT_DIR, cheats_filename);
  }
  else
  {
    strcpy(cheat_path, cheats_filename);
  }

  cheats_file = fopen(cheat_path, "rb");

  if(cheats_file)
  {
    while(fgets(current_line, 256, cheats_file))
    {
printf("current_line: \"%s\"\n", current_line);
		if((name_ptr = check_is_cht(current_line)) != NULL)
		{
//printf("name_ptr:%s\n", name_ptr);
			current_cheat_variant = CHEAT_TYPE_CHT;
			if(!strcasecmp(name_ptr, "gameinfo"))
				current_cheat_variant = CHEAT_TYPE_INVALID;
		}
		else
		{
			// Get the header line first
			name_ptr = strchr(current_line, ' ');
			if(name_ptr)
			{
				*name_ptr = 0;
				name_ptr++;
			}

			if(!strcasecmp(current_line, "gameshark_v1") ||
				!strcasecmp(current_line, "gameshark_v2") ||
				!strcasecmp(current_line, "PAR_v1") ||
				!strcasecmp(current_line, "PAR_v2"))
			{
				current_cheat_variant = CHEAT_TYPE_GAMESHARK_V1;
			}
			else if(!strcasecmp(current_line, "gameshark_v3") ||
				!strcasecmp(current_line, "PAR_v3"))
			{
				current_cheat_variant = CHEAT_TYPE_GAMESHARK_V3;
			}
			else if(!strcasecmp(current_line, "direct_v1") ||
				!strcasecmp(current_line, "direct_v2"))
			{
				current_cheat_variant = CHEAT_TYPE_DIRECT_V1;
			}
			else if(!strcasecmp(current_line, "direct_v3"))
			{
				current_cheat_variant = CHEAT_TYPE_DIRECT_V3;
			}
			else
			{
				current_cheat_variant = CHEAT_TYPE_INVALID;
			}
		}

      if(current_cheat_variant != CHEAT_TYPE_INVALID)
      {
printf("find a valid entrey\n");
		if(string2utf8(name_ptr, current_line_tmp) != 0)
			name_ptr = current_line_tmp;

        strncpy(game_config.cheats_flag[g_num_cheats].cheat_name, name_ptr, CHEAT_NAME_LENGTH - 1);
        game_config.cheats_flag[g_num_cheats].cheat_name[CHEAT_NAME_LENGTH - 1] = 0;
        cheat_name_length = strlen(game_config.cheats_flag[g_num_cheats].cheat_name);
        if(cheat_name_length &&
         ((game_config.cheats_flag[g_num_cheats].cheat_name[cheat_name_length - 1] == '\n') ||
         (game_config.cheats_flag[g_num_cheats].cheat_name[cheat_name_length - 1] == '\r')))
        {
          game_config.cheats_flag[g_num_cheats].cheat_name[cheat_name_length - 1] = 0;
          cheat_name_length--;
        }

        if(cheat_name_length &&
         game_config.cheats_flag[g_num_cheats].cheat_name[cheat_name_length - 1] == '\r')
        {
			game_config.cheats_flag[g_num_cheats].cheat_name[cheat_name_length - 1] = 0;
        }
//printf("cheat%d:%s\n", g_num_cheats, game_config.cheats_flag[g_num_cheats].cheat_name);
        game_config.cheats_flag[g_num_cheats].cheat_variant = current_cheat_variant;
        cheat_code_ptr = game_config.cheats_flag[g_num_cheats].cheat_codes;
        num_cheat_lines = 0;

		if(current_cheat_variant != CHEAT_TYPE_CHT)
		{
			while(fgets(current_line, 256, cheats_file))
			{
			  if(strlen(current_line) < 3)
				break;

			  sscanf(current_line, "%08x %08x", &address, &value);

    	      if((current_cheat_variant != CHEAT_TYPE_DIRECT_V1) && (current_cheat_variant != CHEAT_TYPE_DIRECT_V3))
				decrypt_gsa_code(&address, &value, current_cheat_variant);

    	      cheat_code_ptr[0] = address;
    	      cheat_code_ptr[1] = value;

    	      cheat_code_ptr += 2;
    	      num_cheat_lines++;
	        }
		}
		else
		{
			while(fgets(current_line, 256, cheats_file))
			{
				if(strlen(current_line) < 3)
					break;

				unsigned char *pt;
				unsigned int value;
				unsigned char *var;
				unsigned int var_len;

				pt = strchr(current_line, '=');
				if(pt == NULL) continue;

				pt = sscanf_cht_value(pt+1, &value);
				if(pt == NULL) continue;

				if(value < 0x40000)
					value += 0x2000000;
				else
					value += 0x3000000-0x40000;

				cheat_code_ptr[0] = value;

				var = (unsigned char*)(cheat_code_ptr+2);
				var_len = 0;
				do
				{
					pt = strchr(pt, ',');
					if(pt == NULL) break;

					pt = sscanf_cht_value(pt+1, &value);

					*var = value;
					var_len += 1;
				} while(pt);

				cheat_code_ptr[1] = var_len;
				var_len = (var_len+3)/4 +2;
				cheat_code_ptr += var_len;
				num_cheat_lines++;
	        }
		}

        game_config.cheats_flag[g_num_cheats].num_cheat_lines = num_cheat_lines;
        g_num_cheats++;
//printf("g_num_cheats: %d; num_cheat_lines: %d\n", g_num_cheats, num_cheat_lines);
        if (g_num_cheats == MAX_CHEATS) break;
      }
    }

	strcpy(cheat_path_loaded, cheat_path);
    fclose(cheats_file);
  }
}

static void process_cheat_gs1(CHEAT_TYPE *cheat)
{
  u32 cheat_opcode;
  u32 *code_ptr = cheat->cheat_codes;
  u32 address, value;
  u32 i;

  for(i = 0; i < cheat->num_cheat_lines; i++)
  {
    address = code_ptr[0];
    value = code_ptr[1];

    code_ptr += 2;

    cheat_opcode = address >> 28;
    address &= 0xFFFFFFF;

    switch(cheat_opcode)
    {
      case 0x0:
        write_memory8(address, value);
        break;

      case 0x1:
        write_memory16(address, value);
        break;

      case 0x2:
        write_memory32(address, value);
        break;

      case 0x3:
      {
        u32 num_addresses = address & 0xFFFF;
        u32 address1, address2;
        u32 i2;

        for(i2 = 0; i2 < num_addresses; i2++)
        {
          address1 = code_ptr[0];
          address2 = code_ptr[1];
          code_ptr += 2;
          i++;

          write_memory32(address1, value);
          if(address2 != 0)
            write_memory32(address2, value);
        }
        break;
      }

      case 0x6:
//        if(gamepak_file_large == -1)  // オンメモリのROMの場合だけ
        if(gamepak_file_large == NULL)
          ADDRESS16(gamepak_rom, (address * 2) - 0x08000000) = (value & 0xFFFF);  // データの書込み
        break;

      // GS button down not supported yet
      case 0x8:
        break;

      // Reencryption (DEADFACE) not supported yet
      case 0xD:
        if(read_memory16(address) != (value & 0xFFFF))
        {
          code_ptr += 2;
          i++;
        }
        break;

      case 0xE:
        if(read_memory16(value & 0xFFFFFFF) != (address & 0xFFFF))
        {
          u32 skip = ((address >> 16) & 0x03);
          code_ptr += skip * 2;
          i += skip;
        }
        break;

      // Hook routine not important
      case 0x0F:
        break;
    }
  }
}

// These are especially incomplete.

static void process_cheat_gs3(CHEAT_TYPE *cheat)
{
  u32 cheat_opcode;
  u32 *code_ptr = cheat->cheat_codes;
  u32 address, value;
  u32 i;

  for(i = 0; i < cheat->num_cheat_lines; i++)
  {
    address = code_ptr[0];
    value = code_ptr[1];

    code_ptr += 2;

    cheat_opcode = address >> 28;
    address &= 0xFFFFFFF;

    switch(cheat_opcode)
    {
      case 0x0:
        cheat_opcode = address >> 24;
        address = (address & 0xFFFFF) + ((address << 4) & 0xF000000);

        switch(cheat_opcode)
        {
          case 0x0:
          {
            u32 iterations = value >> 24;
            u32 i2;

            value &= 0xFF;

            for(i2 = 0; i2 <= iterations; i2++, address++)
            {
              write_memory8(address, value);
            }
            break;
          }

          case 0x2:
          {
            u32 iterations = value >> 16;
            u32 i2;

            value &= 0xFFFF;

            for(i2 = 0; i2 <= iterations; i2++, address += 2)
            {
              write_memory16(address, value);
            }
            break;
          }

          case 0x4:
            write_memory32(address, value);
            break;
        }
        break;

      case 0x4:
        cheat_opcode = address >> 24;
        address = (address & 0xFFFFF) + ((address << 4) & 0xF000000);

        switch(cheat_opcode)
        {
          case 0x0:
            address = read_memory32(address) + (value >> 24);
            write_memory8(address, value & 0xFF);
            break;

          case 0x2:
            address = read_memory32(address) + ((value >> 16) * 2);
            write_memory16(address, value & 0xFFFF);
            break;

          case 0x4:
            address = read_memory32(address);
            write_memory32(address, value);
            break;

        }
        break;

      case 0x8:
        cheat_opcode = address >> 24;
        address = (address & 0xFFFFF) + ((address << 4) & 0xF000000);

        switch(cheat_opcode)
        {
          case 0x0:
            value = (value & 0xFF) + read_memory8(address);
            write_memory8(address, value);
            break;

          case 0x2:
            value = (value & 0xFFFF) + read_memory16(address);
            write_memory16(address, value);
            break;

          case 0x4:
            value = value + read_memory32(address);
            write_memory32(address, value);
            break;
        }
        break;

      case 0xC:
        cheat_opcode = address >> 24;
        address = (address & 0xFFFFFF) + 0x4000000;

        switch(cheat_opcode)
        {
          case 0x6:
            write_memory16(address, value);
            break;

          case 0x7:
            write_memory32(address, value);
            break;
        }
        break;
    }
  }
}

static void process_cheat_cht(CHEAT_TYPE *cheat)
{
	u32 i, n;
	unsigned int cheat_num;
	unsigned int avtive_cheat_line;
	unsigned char *dst;
	unsigned char *src;

	avtive_cheat_line = (cheat->num_cheat_lines) >> 16;
//printf("active_cheat_line:%d\n", avtive_cheat_line);
//dump_mem((unsigned char*)(&cheat->cheat_codes[0]), 256);

	n = 1;
	for(i = 0; i < avtive_cheat_line; i++)
	{
		n += (cheat->cheat_codes[n]+3)/4 +2;
	}

	cheat_num = cheat->cheat_codes[n];
	dst = (unsigned char*)cheat->cheat_codes[n-1];
	src = (unsigned char*)(&cheat->cheat_codes[n+1]);

//printf("dst:%08x; src:%08x; len:%d; n:%d\n", dst, src, cheat_num, n);
//	memcpy(dst, src, cheat_num);
	for(i= 0; i < cheat_num; i++)
		write_memory8(dst++, src[i]);
}

void process_cheats()
{
  u32 i;

  for(i = 0; i < g_num_cheats; i++)
  {
    if(game_config.cheats_flag[i].cheat_active == 1)
    {
      switch(game_config.cheats_flag[i].cheat_variant)
      {
        case CHEAT_TYPE_GAMESHARK_V1:
        case CHEAT_TYPE_DIRECT_V1:
          process_cheat_gs1(game_config.cheats_flag + i);
          break;

        case CHEAT_TYPE_GAMESHARK_V3:
        case CHEAT_TYPE_DIRECT_V3:
          process_cheat_gs3(game_config.cheats_flag + i);
          break;

		case CHEAT_TYPE_CHT:
//printf("cheat%d\n", i);
			process_cheat_cht(game_config.cheats_flag + i);
		  break;

        default:
          break;
      }
    }
  }
}
