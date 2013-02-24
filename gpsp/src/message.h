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

#ifndef MESSAGE_H
#define MESSAGE_H

enum MSG
{
    MSG_MAIN_MENU_0,    /* 0 */
    MSG_MAIN_MENU_1,    /* 1 */
    MSG_MAIN_MENU_2,    /* 2 */
    MSG_MAIN_MENU_3,    /* 3 */
    MSG_MAIN_MENU_4,    /* 4 */
    MSG_MAIN_MENU_5,    /* 5 */
    MSG_SUB_MENU_00,    /* 6 */
    MSG_SUB_MENU_01,    /* 7 */
    MSG_SUB_MENU_02,    /* 8 */
    MSG_SUB_MENU_03,    /* 9 */
    MSG_SUB_MENU_04,    /* 10 */
    MSG_SUB_MENU_05,    /* 11 */
    MSG_SUB_MENU_10,    /* 12 */
    MSG_SUB_MENU_11,    /* 13 */
    MSG_SUB_MENU_12,    /* 14 */
    MSG_SUB_MENU_13,    /* 15 */
    MSG_SUB_MENU_14,    /* 16 */
    MSG_SUB_MENU_20,    /* 17 */
    MSG_SUB_MENU_21,    /* 18 */
    MSG_SUB_MENU_22,    /* 19 */
    MSG_SUB_MENU_23,    /* 20 */
    MSG_SUB_MENU_24,    /* 21 */
    MSG_SUB_MENU_30,    /* 22 */
    MSG_SUB_MENU_31,    /* 23 */
    MSG_SUB_MENU_32,    /* 24 */
    MSG_SUB_MENU_40,    /* 25 */
    MSG_SUB_MENU_41,    /* 26 */
    MSG_SUB_MENU_42,    /* 27 */
    MSG_SUB_MENU_43,    /* 28 */
    MSG_SUB_MENU_44,    /* 29 */
    MSG_SUB_MENU_45,    /* 30 */
    MSG_SUB_MENU_300,   /* 31 */
    MSG_SUB_MENU_301,   /* 32 */
    MSG_SUB_MENU_302,   /* 33 */
    MSG_SUB_MENU_310,   /* 34 */
    MSG_SUB_MENU_311,   /* 35 */
    MSG_SUB_MENU_312,   /* 36 */
    MSG_SUB_MENU_313,   /* 37 */
    MSG_SUB_MENU_314,   /* 38 */
    MSG_SUB_MENU_315,   /* 39 */
    MSG_SUB_MENU_60,    /* 40 */
    MSG_SUB_MENU_61,    /* 41 */
    MSG_SUB_MENU_62,    /* 42 */

    MSG_SCREEN_RATIO_0, /* 43 */
    MSG_SCREEN_RATIO_1, /* 44 */

    MSG_FRAMESKIP_0,    /* 45 */
    MSG_FRAMESKIP_1,    /* 46 */

    MSG_ON_OFF_0,       /* 47 */
    MSG_ON_OFF_1,       /* 48 */
    
    MSG_SOUND_SWITCH_0, /* 49 */
    MSG_SOUND_SWITCH_1, /* 50 */

    MSG_SNAP_FRAME_0,   /* 51 */
    MSG_SNAP_FRAME_1,   /* 52 */

    MSG_EN_DIS_ABLE_0,  /* 53 */
    MSG_EN_DIS_ABLE_1,  /* 54 */

    MSG_NON_LOAD_GAME,  /* 55 */
    MSG_CHEAT_MENU_NON_LOAD,/* 56 */
    MSG_CHEAT_MENU_LOADED,   /* 57 */

    MSG_LOAD_STATE,     /* 58 */
    MSG_LOAD_STATE_END, /* 59 */
    MSG_SAVE_STATE,     /* 60 */
    MSG_SAVE_STATE_END, /* 61 */

    MSG_KEY_MAP_NONE,   /* 62 */
    MSG_KEY_MAP_A,      /* 63 */
    MSG_KEY_MAP_B,      /* 64 */
    MSG_KEY_MAP_SL,     /* 65 */
    MSG_KEY_MAP_ST,     /* 66 */
    MSG_KEY_MAP_RT,     /* 67 */
    MSG_KEY_MAP_LF,     /* 68 */
    MSG_KEY_MAP_UP,     /* 69 */
    MSG_KEY_MAP_DW,     /* 70 */
    MSG_KEY_MAP_R,      /* 71 */
    MSG_KEY_MAP_L,      /* 72 */
    MSG_KEY_MAP_X,      /* 73 */
    MSG_KEY_MAP_Y,      /* 74 */
    MSG_KEY_MAP_TOUCH,  /* 75 */

    MSG_SAVESTATE_EMPTY,/* 76 */
    MSG_SAVESTATE_FULL, /* 77 */
    MSG_SAVESTATE_DOING,/* 78 */
    MSG_SAVESTATE_FAILUER,/* 79 */
    MSG_SAVESTATE_SUCCESS,/* 80 */
    MSG_SAVESTATE_SLOT_EMPTY,/* 81 */
    MSG_SAVESTATE_FILE_BAD, /* 82 */
    MSG_LOADSTATE_DOING,/* 83 */
    MSG_LOADSTATE_FAILURE,/* 84 */
    MSG_LOADSTATE_SUCCESS,/* 85 */

    MSG_WARING_DIALOG,  /* 86 */
    MSG_TIME_FORMATE,   /* 87 */

    MSG_SUB_MENU_130,   /* 88 */
    MSG_SUB_MENU_131,   /* 89 */

    MSG_DELETTE_ALL_SAVESTATE_WARING,
    MSG_DELETTE_SINGLE_SAVESTATE_WARING,
    MSG_DELETTE_SAVESTATE_NOTHING,

    MSG_SAVE_SNAPSHOT,
    MSG_SAVE_SNAPSHOT_COMPLETE,
    MSG_SAVE_SNAPSHOT_FAILURE,

    MSG_CHANGE_LANGUAGE,
    MSG_CHANGE_LANGUAGE_WAITING,

    MSG_NO_SLIDE,
    MSG_PLAYING_SLIDE,
    MSG_PAUSE_SLIDE,
    MSG_PLAY_SLIDE1,
    MSG_PLAY_SLIDE2,
    MSG_PLAY_SLIDE3,
    MSG_PLAY_SLIDE4,
    MSG_PLAY_SLIDE5,
    MSG_PLAY_SLIDE6,
    
    MSG_LOADING_GAME,

    MSG_GBA_VERSION0,
    MSG_GBA_VERSION1,

    MSG_LOAD_DEFAULT_WARING,
    MSG_DEFAULT_LOADING,

    MSG_BACK,

    MSG_END
};

enum LANGUAGE{
    ENGLISH,
    CHINESE_SIMPLIFIED,
    CHINESE_TRADITIONAL
};

char *msg[MSG_END+1];
char msg_data[16 * 1024]; // メッセージ用のデータ 16kb

#endif
