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

// VIDEO OUTの有効画素は660x440(GBAの2.75倍)
// インタレース時の有効画素は660x220},

const SCREEN_PARAMETER screen_parameter_psp_menu_init =
{
    // LCD OUT / MENU SCREEN (FIX)
    {0, 0x000, 480, 272, 1, 15, 0},
    {0, 0},
    {480, 480, 272},
    {9, 9},
    {480, 272},
    {0, 0, 480, 272},
    {0, 0, 0, 0, 0, 480, 272, 480, 272, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

const SCREEN_PARAMETER screen_parameter_psp_game_init[] =
{
  {
    // LCD OUT / GAME SCREEN / unscaled (FIX)
    {0, 0x000, 480, 272, 1, 15, 0},
    {0, 1},
    {240, 240, 160},
    {8, 8},
    {480, 272},
    {0, 0, 480, 272},
    {0, 0, 120, 56, 0, 240, 160, 360, 216, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  {
    // LCD OUT / GAME SCREEN / aspect(FIX)
    {0, 0x000, 480, 272, 1, 15, 0},
    {0, 1},
    {240, 240, 160},
    {8, 8},
    {480, 272},
    {0, 0, 480, 272},
    {0, 0, 36, 0, 0, 240, 160, 444, 272, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  {
    // LCD OUT / GAME SCREEN / fullscreen (FIX)
    {0, 0x000, 480, 272, 1, 15, 0},
    {0, 1},
    {240, 240, 160},
    {8, 8},
    {480, 272},
    {0, 0, 480, 272},
    {0, 0, 0, 0, 0, 240, 160, 480, 272, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  {
    // LCD OUT / GAME SCREEN / option 1 180 rotate (FIX)
    {0, 0x000, 480, 272, 1, 15, 0},
    {0, 1},
    {240, 240, 160},
    {8, 8},
    {480, 272},
    {0, 0, 480, 272},
    {240, 160, 36, 0, 0, 0, 0, 444, 272, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  },
  {
    // LCD OUT / GAME SCREEN / option 2 (FIX)
    {0, 0x000, 480, 272, 1, 15, 0},
    {0, 1},
    {240, 240, 160},
    {8, 8},
    {480, 272},
    {0, 0, 480, 272},
    {0, 0, 120, 56, 0, 240, 160, 360, 216, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
  }
};

