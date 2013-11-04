/* ReGBA - Persistent settings
 *
 * Copyright (C) 2013 Dingoonity user Nebuleon
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

static void Menu_SaveOption(FILE_TAG_TYPE fd, struct MenuEntry *entry)
{
	char buf[257];
	MenuPersistenceFunction SaveFunction = entry->SaveFunction;
	if (SaveFunction == NULL) SaveFunction = &DefaultSaveFunction;
	(*SaveFunction)(entry, buf);
	buf[256] = '\0';

	FILE_WRITE(fd, &buf, strlen(buf) * sizeof(buf[0]));
}

static void Menu_SaveIterateRecurse(FILE_TAG_TYPE fd, struct Menu *menu)
{
	struct MenuEntry *cur;
	int i=0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			Menu_SaveIterateRecurse(fd, cur->Target);
			break;
		case KIND_OPTION:
			Menu_SaveOption(fd, cur);
			break;
		default:
			break;
		}
	}
}

static void Menu_ClearPerGameIterateRecurse(struct Menu *menu)
{
	struct MenuEntry *cur;
	int i=0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			Menu_ClearPerGameIterateRecurse(cur->Target);
			break;
		case KIND_OPTION:
			*(uint32_t *) cur->Target = 0;
			break;
		default:
			break;
		}
	}
}

static uint32_t Menu_CountIterateRecurse(struct Menu *menu)
{
	struct MenuEntry *cur;
	int i=0;
	uint32_t Result = 0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			Result += Menu_CountIterateRecurse(cur->Target);
			break;
		case KIND_OPTION:
			Result++;
			break;
		default:
			break;
		}
	}
	
	return Result;
}

static void* Menu_PreserveIterateRecurse(struct Menu *menu, void* ptr)
{
	struct MenuEntry *cur;
	int i=0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			ptr = Menu_PreserveIterateRecurse(cur->Target, ptr);
			break;
		case KIND_OPTION:
			*(uint32_t *) ptr = *(uint32_t *) cur->Target;
			ptr = (uint8_t *) ptr + sizeof(uint32_t);
			break;
		default:
			break;
		}
	}
	
	return ptr;
}

static struct MenuEntry *Menu_FindByPersistentName(struct Menu *menu, char *name)
{
	struct MenuEntry *retcode = NULL;

	struct MenuEntry *cur;
	int i=0;

	while ((cur = menu->Entries[i++])) {
		switch (cur->Kind) {
		case KIND_SUBMENU:
			if ((retcode = Menu_FindByPersistentName(cur->Target, name)))
				return retcode;
			break;
		case KIND_OPTION:
			if (strcasecmp(cur->PersistentName, name) == 0)
				return cur;
			break;
		default:
			break;
		}
	}

	return retcode;
}

bool ReGBA_SaveSettings(char *cfg_name, bool PerGame)
{
	char fname[MAX_PATH + 1];
	if (strlen(main_path) + strlen(cfg_name) + 5 /* / .cfg */ > MAX_PATH)
	{
		ReGBA_Trace("E: Somehow you hit the filename size limit :o\n");
		return false;
	}
	sprintf(fname, "%s/%s.cfg", main_path, cfg_name);
	FILE_TAG_TYPE fd;

	ReGBA_ProgressInitialise(PerGame ? FILE_ACTION_SAVE_GAME_SETTINGS : FILE_ACTION_SAVE_GLOBAL_SETTINGS);

	FILE_OPEN(fd, fname, WRITE);
	if(FILE_CHECK_VALID(fd)) {
		Menu_SaveIterateRecurse(fd, PerGame ? MainMenu.AlternateVersion : &MainMenu);
		ReGBA_ProgressUpdate(1, 1);
		ReGBA_ProgressFinalise();
	}
	else
	{
		ReGBA_Trace("E: Couldn't open file %s for writing.\n", fname);
		ReGBA_ProgressFinalise();
		return false;
	}

	FILE_CLOSE(fd);
	return true;
}

/*
 * Fixes up impossible settings after loading them from configuration.
 */
static void FixUpSettings()
{
	int i;
	for (i = 0; i < 10; i++)
		if (KeypadRemapping[i] == 0)
		{
			memcpy(KeypadRemapping, DefaultKeypadRemapping, sizeof(DefaultKeypadRemapping));
			break;
		}
	if (IsImpossibleHotkey(Hotkeys[0]))
		Hotkeys[0] = 0;
	if (Hotkeys[1] == 0 || IsImpossibleHotkey(Hotkeys[1]))
		Hotkeys[1] = OPENDINGUX_BUTTON_FACE_UP;
	if (IsImpossibleHotkey(Hotkeys[2]))
		Hotkeys[2] = 0;
	if (IsImpossibleHotkey(Hotkeys[3]))
		Hotkeys[3] = 0;
	if (IsImpossibleHotkey(Hotkeys[4]))
		Hotkeys[4] = 0;

	if (IsImpossibleHotkey(PerGameHotkeys[0]))
		PerGameHotkeys[0] = 0;
	if (IsImpossibleHotkey(PerGameHotkeys[2]))
		PerGameHotkeys[2] = 0;
	if (IsImpossibleHotkey(PerGameHotkeys[3]))
		PerGameHotkeys[3] = 0;
	if (IsImpossibleHotkey(PerGameHotkeys[4]))
		PerGameHotkeys[4] = 0;
}

void ReGBA_LoadSettings(char *cfg_name, bool PerGame)
{
	char fname[MAX_PATH + 1];
	if (strlen(main_path) + strlen(cfg_name) + 5 /* / .cfg */ > MAX_PATH)
	{
		ReGBA_Trace("E: Somehow you hit the filename size limit :o\n");
		return;
	}
	sprintf(fname, "%s/%s.cfg", main_path, cfg_name);

	FILE_TAG_TYPE fd;

	ReGBA_ProgressInitialise(PerGame ? FILE_ACTION_LOAD_GAME_SETTINGS : FILE_ACTION_LOAD_GLOBAL_SETTINGS);

	FILE_OPEN(fd, fname, READ);

	if(FILE_CHECK_VALID(fd)) {
		char line[257];

		while(fgets(line, 256, fd))
		{
			line[256] = '\0';

			char* opt = NULL;
			char* arg = NULL;

			char* cur = line;

			// Find the start of the option name.
			while (*cur == ' ' || *cur == '\t')
				cur++;
			// Now find where it ends.
			while (*cur && !(*cur == ' ' || *cur == '\t' || *cur == '='))
			{
				if (*cur == '#')
					continue;
				else if (opt == NULL)
					opt = cur;
				cur++;
			}
			if (opt == NULL)
				continue;
			bool WasEquals = *cur == '=';
			*cur++ = '\0';
			if (!WasEquals)
			{
				// Skip all whitespace before =.
				while (*cur == ' ' || *cur == '\t')
					cur++;
				if (*cur != '=')
					continue;
				cur++;
			}
			// Find the start of the option argument.
			while (*cur == ' ' || *cur == '\t')
				cur++;
			// Now find where it ends.
			while (*cur)
			{
				if (*cur == '#')
				{
					*cur = '\0';
					break;
				}
				else if (arg == NULL)
					arg = cur;
				cur++;
			}
			if (arg == NULL)
				continue;
			cur--;
			while (*cur == ' ' || *cur == '\t' || *cur == '\n' || *cur == '\r')
			{
				*cur = '\0';
				cur--;
			}

			struct MenuEntry* entry = Menu_FindByPersistentName(PerGame ? MainMenu.AlternateVersion : &MainMenu, opt);
			if (entry == NULL)
			{
				ReGBA_Trace("W: Option '%s' not found; ignored", opt);
			}
			else
			{
				MenuPersistenceFunction LoadFunction = entry->LoadFunction;
				if (LoadFunction == NULL) LoadFunction = &DefaultLoadFunction;
				(*LoadFunction)(entry, arg);
			}
		}
		ReGBA_ProgressUpdate(1, 1);
	}
	else
	{
		ReGBA_Trace("W: Couldn't open file %s for loading.\n", fname);
	}
	FixUpSettings();
	ReGBA_ProgressFinalise();
}

void ReGBA_ClearPerGameSettings()
{
	Menu_ClearPerGameIterateRecurse(MainMenu.AlternateVersion);
}

void* ReGBA_PreserveMenuSettings(struct Menu* Menu)
{
	uint32_t EntryCount = Menu_CountIterateRecurse(Menu);

	void* Result = malloc(EntryCount * sizeof(uint32_t));
	if (Result != NULL)
	{
		Menu_PreserveIterateRecurse(Menu, Result);
	}
	return Result;
}

bool ReGBA_AreMenuSettingsEqual(struct Menu* Menu, void* A, void* B)
{
	uint32_t EntryCount = Menu_CountIterateRecurse(Menu);

	return memcmp(A, B, EntryCount * sizeof(uint32_t)) == 0;
}

uint32_t ResolveSetting(uint32_t GlobalValue, uint32_t PerGameValue)
{
	if (PerGameValue != 0)
		return PerGameValue - 1;
	else
		return GlobalValue;
}

enum OpenDingux_Buttons ResolveButtons(enum OpenDingux_Buttons GlobalValue, enum OpenDingux_Buttons PerGameValue)
{
	if (PerGameValue != 0)
		return PerGameValue;
	else
		return GlobalValue;
}
