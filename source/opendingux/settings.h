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

#ifndef __REGBA_SETTINGS_H__
#define __REGBA_SETTINGS_H__

#include "common.h"

extern bool ReGBA_SaveSettings(char *cfg_name, bool PerGame);
extern void ReGBA_LoadSettings(char *cfg_name, bool PerGame);

/*
 * Resolves the value of a setting that can be overridden in per-game
 * configuration.
 * 
 * Input:
 *   GlobalValue: The value of the setting being resolved, as defined in
 *     the global configuration.
 *   PerGameValue: The value of the setting being resolved, as defined in
 *     the per-game configuration. This value is 0 if there is no override
 *     defined for the setting, and the value of the setting plus 1 otherwise.
 * Returns:
 *   The value of the setting being resolved, per-game preferred.
 */
extern uint32_t ResolveSetting(uint32_t GlobalValue, uint32_t PerGameValue);

/*
 * Resolves the value of a button mapping or hotkey that can be overridden in
 * per-game configuration.
 * 
 * Input:
 *   GlobalValue: The value of the button mapping or hotkey being resolved, as
 *     defined in the global configuration.
 *   PerGameValue: The value of the button mapping or hotkey being resolved,
 *     as defined in the per-game configuration. This value is 0 if there is
 *     no override defined for the setting, and the value of the setting
 *     otherwise.
 * Returns:
 *   The value of the setting being resolved, per-game preferred.
 */
extern enum OpenDingux_Buttons ResolveButtons(enum OpenDingux_Buttons GlobalValue, enum OpenDingux_Buttons PerGameValue);

/*
 * Clears per-game setting overrides. Should be called between games after
 * saving the settings of the active game to isolate the settings of different
 * games.
 * Output assertions:
 *   All settings reachable from the alternate version of the Main Menu have
 *   overrides removed. Only global settings take effect.
 */
extern void ReGBA_ClearPerGameSettings();

#endif // !defined __REGBA_SETTINGS_H__