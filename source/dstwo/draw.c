/* draw.c
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
//v1.1

/******************************************************************************
 * draw.cpp
 * basic program to draw some graphic
 ******************************************************************************/
#include "common.h"

/******************************************************************************
 * macro definition
 ******************************************************************************/
#define progress_sx (screen_width2 - DS_SCREEN_WIDTH / 3)  // Center -160/-80
#define progress_ex (screen_width2 + DS_SCREEN_WIDTH / 3)  // Center +160/+80
#define progress_sy (screen_height2 + 3)                // Center +3
#define progress_ey (screen_height2 + 13)               // Center +13
#define yesno_sx    (screen_width2 - DS_SCREEN_WIDTH / 3)  // Center -160/-80
#define yesno_ex    (screen_width2 + DS_SCREEN_WIDTH / 3)  // Center +160/+80
#define yesno_sy    (screen_height2 + 3)                // Center +3
#define yesno_ey    (screen_height2 + 13)               // Center +13
#define progress_color BGR555(15,15,15)

//#define progress_wait (0.5 * 1000 * 1000)
#define progress_wait (OS_TICKS_PER_SEC/2)				//0.5S

#define SCREEN_PITCH	256

#define VRAM_POS(screen, x, y)  (screen + (x + (y) * SCREEN_PITCH))

#define BOOTLOGO "SYSTEM/GUI/boot.bmp"
#define GUI_SOURCE_PATH "SYSTEM/GUI"
#define GUI_PIC_BUFSIZE (1024*512)

uint16_t gui_picture[GUI_PIC_BUFSIZE];

struct gui_iconlist gui_icon_list[]= {
    //file system
    /* 00 */ {"zipfile", 16, 16, NULL},
    /* 01 */ {"directory", 16, 16, NULL},
    /* 02 */ {"gbafile", 16, 16, NULL},

	//title
	/* 03 */ {"stitle", 256, 33, NULL},
	//main menu
	/* 04 */ {"savo", 52, 52, NULL},
	/* 05 */ {"ssaveo", 52, 52, NULL},
	/* 06 */ {"stoolo", 52, 52, NULL},
	/* 07 */ {"scheato", 52, 52, NULL},
	/* 08 */ {"sother", 52, 52, NULL},
	/* 09 */ {"sexito", 52, 52, NULL},
	/* 10 */ {"smsel", 79, 15, NULL},
	/* 11 */ {"smnsel", 79, 15, NULL},

	/* 12 */ {"snavo", 52, 52, NULL},
	/* 13 */ {"snsaveo", 52, 52, NULL},
	/* 14 */ {"sntoolo", 52, 52, NULL},
	/* 15 */ {"sncheato", 52, 52, NULL},
	/* 16 */ {"snother", 52, 52, NULL},
	/* 17 */ {"snexito", 52, 52, NULL},

	/* 18 */ {"sunnof", 16, 16, NULL},
	/* 19 */ {"smaini", 85, 38, NULL},
	/* 20 */ {"snmaini", 85, 38, NULL},
	/* 21 */ {"smaybgo", 256, 192, NULL},

	/* 22 */ {"sticon", 29, 13, NULL},
	/* 23 */ {"ssubbg", 256, 192, NULL},

	/* 24 */ {"subsela", 245, 22, NULL},
	/* 25 */ {"sfullo", 12, 12, NULL},
	/* 26 */ {"snfullo", 12, 12, NULL},
	/* 27 */ {"semptyo", 12, 12, NULL},
	/* 28 */ {"snemptyo", 12, 12, NULL},
	/* 29 */ {"fdoto", 16, 16, NULL},
	/* 30 */ {"backo", 19, 13, NULL},
	/* 31 */ {"nbacko", 19, 13, NULL},
	/* 32 */ {"chtfile", 16, 15, NULL},
	/* 33 */ {"smsgfr", 193, 111, NULL},
	/* 34 */ {"sbutto", 76, 16, NULL}
                        };

uint16_t COLOR_BG            = BGR555( 0,  0,  0);
uint16_t COLOR_INACTIVE_ITEM = BGR555( 0,  0,  0);
uint16_t COLOR_ACTIVE_ITEM   = BGR555(31, 31, 31);
uint16_t COLOR_MSSG          = BGR555( 0,  0,  0);
uint16_t COLOR_INACTIVE_MAIN = BGR555(31, 31, 31);
uint16_t COLOR_ACTIVE_MAIN   = BGR555(31, 31, 31);


/*
*	Drawing string aroud center
*/
void print_string_center(uint16_t* screen, uint32_t sy, uint32_t color, uint32_t bg_color, char *str)
{
	int width = 0;//fbm_getwidth(str);
	uint32_t sx = (DS_SCREEN_WIDTH - width) / 2;

	PRINT_STRING_BG(screen, str, color, bg_color, sx, sy);
}

/*
*	Drawing string with shadow around center
*/
void print_string_shadow_center(uint16_t* screen, uint32_t sy, uint32_t color, char *str)
{
	int width = 0;//fbm_getwidth(str);
	uint32_t sx = (DS_SCREEN_WIDTH - width) / 2;

	PRINT_STRING_SHADOW(screen, str, color, sx, sy);
}

/*
*	Drawing horizontal line
*/
void drawhline(uint16_t* screen, uint32_t sx, uint32_t ex, uint32_t y, uint32_t color)
{
	uint32_t x;
	uint32_t width  = (ex - sx) + 1;
	uint16_t *dst = VRAM_POS(screen, sx, y);

	for (x = 0; x < width; x++)
		*dst++ = (uint16_t)color;
}

/*
*	Drawing vertical line
*/
void drawvline(uint16_t* screen, uint32_t x, uint32_t sy, uint32_t ey, uint32_t color)
{
	int y;
	int height = (ey - sy) + 1;
	uint16_t *dst = VRAM_POS(screen, x, sy);

	for (y = 0; y < height; y++)
	{
		*dst = (uint16_t)color;
		dst += SCREEN_PITCH;
	}
}

/*
*	Drawing rectangle
*/
void drawbox(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t color)
{
	drawhline(screen, sx, ex - 1, sy, color);
	drawvline(screen, ex, sy, ey - 1, color);
	drawhline(screen, sx + 1, ex, ey, color);
	drawvline(screen, sx, sy + 1, ey, color);
}

/*
*	Filling a rectangle
*/
void drawboxfill(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t color)
{
	uint32_t x, y;
	uint32_t width  = (ex - sx) + 1;
	uint32_t height = (ey - sy) + 1;
	uint16_t *dst = VRAM_POS(screen, sx, sy);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			dst[x + y * SCREEN_PITCH] = (uint16_t)color;
		}
	}
}

/*
*	Drawing a selection item
- active    0 not fill
-           1 fill with gray
-           2 fill with color
-           3 fill with color and most brithness
- color     0 Red
-           1 Green
-           2 Blue
------------------------------------------------------*/
void draw_selitem(uint16_t* screen, uint32_t x, uint32_t y, uint32_t color, uint32_t active)
{
	uint32_t size;
	uint32_t color0, color1, color2, color3;

	size = 10;

	switch (active) {
	case 1:
		color0 = BGR555(12, 12, 12);
		color1 = BGR555(2, 2, 2);
		color2 = BGR555(7, 7, 7);
		color3 = BGR555(22, 22, 22);
		break;
	case 2:
		switch (color) {
		case 0:  // Red
			color0 = BGR555(12, 12, 12);
			color1 = BGR555(0, 0, 8);
			color2 = BGR555(0, 0, 16);
			color3 = BGR555(0, 0, 24);
			break;
		default:
		case 1:  // Green
			color0 = BGR555(12, 12, 12);
			color1 = BGR555(0, 8, 0);
			color2 = BGR555(0, 16, 0);
			color3 = BGR555(0, 24, 0);
			break;
		case 2:  // Blue
			color0 = BGR555(12, 12, 12);
			color1 = BGR555(8, 0, 0);
			color2 = BGR555(16, 0, 0);
			color3 = BGR555(24, 0, 0);
			break;
		}
		break;
	case 3:
		switch (color) {
		case 0:  // Red
			color0 = BGR555(31, 31, 31);
			color1 = BGR555(0, 0, 16);
			color2 = BGR555(0, 0, 22);
			color3 = BGR555(0, 0, 31);
			break;
		default:
		case 1:  // Green
			color0 = BGR555(31, 31, 31);
			color1 = BGR555(0, 16, 0);
			color2 = BGR555(0, 22, 0);
			color3 = BGR555(0, 31, 0);
			break;
		case 2:  // Blue
			color0 = BGR555(31, 31, 31);
			color1 = BGR555(16, 0, 0);
			color2 = BGR555(22, 0, 0);
			color3 = BGR555(31, 0, 0);
			break;
		}
		break;
	default:
		color0 = BGR555(18, 18, 18);
		color1 = color2 = color3 = BGR555(18, 18, 18);
		break;
	}

	drawbox(screen, x, y, x+size-1, y+size-1, color0);

	if (active > 0) {
		drawbox(screen, x + 1, y + 1, x + size - 2, y + size - 2, color1);
		drawbox(screen, x + 2, y + 2, x + size - 3, y + size - 3, color2);
		drawboxfill(screen, x + 3, y + 3, x + size - 4, y + size - 4, color3);
	}
}

/*
*	Drawing message box
*	Note if fg_color is transparent, screen_bg can't be transparent
*/
void draw_message_box(uint16_t* screen)
{
	show_icon(screen, &ICON_MSG, (DS_SCREEN_WIDTH - ICON_MSG.x) / 2, (DS_SCREEN_HEIGHT - ICON_MSG.y) / 2);
}

/*
*	Drawing string horizontal center aligned
*/
void draw_string_vcenter(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width, uint32_t fg_color, const char* string)
{
	uint32_t num = 0, i = 0;
	uint16_t *screenp = VRAM_POS(screen, sx, sy);
	uint16_t unicode[strlen(string)];

	num = 0;
	while (*string) {
		string = utf8decode(string, unicode + num);
		num++;
	}

	i = 0;
	while (i < num) {
		uint32_t m, x;
		m = BDF_cut_unicode(&unicode[i], num - i, width, 1);
		x = (width - BDF_cut_unicode(&unicode[i], m, 0, 3)) / 2;
		while (m--) {
			x += BDF_render16_ucs(screenp + x, DS_SCREEN_WIDTH, COLOR_TRANS,
				fg_color, unicode[i++]);
		}
		if (i < num && (unicode[i] == 0x0D || unicode[i] == 0x0A))
			i++;
		else {
			while (i < num && (unicode[i] == ' ')) i++;
		}
		screenp += BDF_GetFontHeight() * DS_SCREEN_WIDTH;
	}
}

struct scroll_string_info {
	uint16_t *screenp;
	uint32_t sx;
	uint32_t sy;
	uint32_t width;
	uint32_t height;
	uint16_t bg_color;
	uint16_t *buff_fonts;
	uint32_t buff_width;
	uint32_t pos_pixel;
};

/*
 * Initialises a text scroller to display a certain string.
 * Input assertions: sx + width < DS_SCREEN_WIDTH &&
 *   sy + [text height] < DS_SCREEN_HEIGHT && string != NULL &&
 *   screen != NULL.
 * Input: 'screen', the address of the upper-left corner of the screen.
 *        'sx' and 'sy', the X and Y coordinates of the upper-left corner of
 *          the text.
 *        'width', the width of the scroller's viewport.
 *        'bg_color', the BGR555 color of the background around the text, or
 *          COLOR_TRANS for transparency.
 *        'fg_color', the BGR555 color of the text.
 *        'string', the text to be scrolled, encoded as UTF-8.
 * Output: the scroller's handle, to be used to scroll the text in
 *   draw_hscroll.
 */
void* hscroll_init(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
        uint16_t bg_color, uint16_t fg_color, const char* string)
{
	size_t i;
	uint32_t x = 0, textWidth = 0, height = BDF_GetFontHeight(), num = 0;
	uint16_t *unicode, *buff_fonts;
	struct scroll_string_info* result;

	result = malloc(sizeof(struct scroll_string_info));
	if (result == NULL)
		goto exit;

	unicode = malloc(strlen(string) * sizeof(uint16_t));
	if (unicode == NULL)
		goto fail_with_scroller;

	while (*string) {
		string = utf8decode(string, &unicode[num]);
		if (unicode[num] != 0x0D && unicode[num] != 0x0A) {
			textWidth += BDF_WidthUCS2(unicode[num]);
			num++;
		}
	}
	if (textWidth < width)
		textWidth = width;

	buff_fonts = malloc(textWidth * height * sizeof(uint16_t));
	if (buff_fonts == NULL)
		goto fail_with_unicode;

	if (bg_color == COLOR_TRANS)
		memset(buff_fonts, 0, textWidth * height * sizeof(uint16_t));

	for (i = 0; i < num; i++) {
		uint16_t ucs2 = unicode[i];
		if (ucs2 != 0x0D && ucs2 != 0x0A) {
			x += BDF_render16_ucs(buff_fonts + x, textWidth, bg_color, fg_color, ucs2);
		}
	}

	result->screenp = screen;
	result->sx = sx;
	result->sy = sy;
	result->bg_color = bg_color;
	result->width = width;
	result->height = height;
	result->buff_fonts = buff_fonts;
	result->buff_width = textWidth;
	result->pos_pixel = 0;

	free(unicode);
	return result;

fail_with_unicode:
	free(unicode);
fail_with_scroller:
	free(result);
exit:
	return result;
}

void* draw_hscroll_init(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width,
        uint16_t bg_color, uint16_t fg_color, const char* string)
{
	void* result = hscroll_init(screen, sx, sy, width, bg_color, fg_color, string);

	if (result != NULL)
		draw_hscroll(result, 0 /* stay on the left */);

	return result;
}

/*
 * Scrolls an initialised scroller's text.
 * A scroller is never allowed to go past the beginning of the text when
 * scrolling to the left, or to go past the end when scrolling to the right.
 * Input assertions: index was returned by a previous call to
 *   draw_hscroll_init and not used in a call to draw_hscroll_over.
 * Input: 'handle', the scroller's handle.
 *        'scroll_val', the number of pixels to scroll. The sign affects the
 *          direction. If scroll_val > 0, the scroller's viewport is moved to
 *          the left; if < 0, the scroller's viewport is moved to the right.
 * Output: the number of pixels still available to scroll in the direction
 *   specified by the sign of 'scroll_val'.
 *
 * Example: (assume each letter is 1 pixel; this won't be true in reality)
 *           [some lengthy text shown in ]         |
 * val -5 -> |    [lengthy text shown in a scr]xxxxx -> to right, returns 5
 * val -5 -> |         [hy text shown in a scroller] -> to right, returns 0
 * val  3 -> xxxxxxx[ngthy text shown in a scrol]  | -> to left,  returns 7
 * val  3 -> xxxx[ lengthy text shown in a sc]     | -> to left,  returns 4
 */
uint32_t draw_hscroll(void* handle, int32_t scroll_val)
{
	uint16_t bg_color;
	uint32_t i, width, height;
	struct scroll_string_info* scroller;

	if (handle == NULL)
		return 0;
	scroller = (struct scroll_string_info*) handle;

	if (scroller->buff_fonts == NULL || scroller->screenp == NULL
	 || scroller->buff_width < scroller->width)
		return 0;

	width = scroller->width;
	height = scroller->height;
	bg_color = scroller->bg_color;

	// 1. Shift the scroller.
	if (scroll_val > 0 && scroller->pos_pixel < (uint32_t) scroll_val)
		// Reached the beginning
		scroller->pos_pixel = 0;
	else {
		scroller->pos_pixel -= scroll_val;
		if (scroller->pos_pixel > scroller->buff_width - width)
			// Reached the end
			scroller->pos_pixel = scroller->buff_width - width;
	}

	// 2. Draw the scroller's text at its new position.
	uint32_t x, sx, sy;
	uint16_t pixel;
	const uint16_t* src;
	uint16_t* dst;

	sx = scroller->sx;
	sy = scroller->sy;

	if (bg_color == COLOR_TRANS) {
		for (i = 0; i < height; i++) {
			dst = scroller->screenp + sx + (sy + i) * DS_SCREEN_WIDTH;
			src = scroller->buff_fonts + scroller->pos_pixel + i * scroller->buff_width;
			for (x = 0; x < width; x++) {
				pixel = *src++;
				if (pixel) *dst = pixel;
				dst++;
			}
		}
	} else {
		for (i = 0; i < height; i++) {
			dst = scroller->screenp + sx + (sy + i) * DS_SCREEN_WIDTH;
			src = scroller->buff_fonts + scroller->pos_pixel + i * scroller->buff_width;
			memcpy(dst, src, width * sizeof(uint16_t));
		}
	}

	// 3. Return how many more pixels we can scroll in the same direction.
	if (scroll_val > 0)
		// Scrolling to the left: Return the number of pixels we can still go
		// to the left.
		return scroller->pos_pixel;
	else if (scroll_val < 0)
		// Scrolling to the right: Return the number of pixels we can still go
		// to the right.
		return scroller->buff_width - scroller->pos_pixel - width;
	else
		return 0;
}

void draw_hscroll_over(void* handle)
{
	struct scroll_string_info* scroller;

	if (handle == NULL)
		return;
	scroller = (struct scroll_string_info*) handle;

	if (scroller->buff_fonts) {
		free(scroller->buff_fonts);
		scroller->buff_fonts = NULL;
	}

	free(scroller);
}

/*
*	Draw yes or no dialog
*/
uint32_t draw_yesno_dialog(enum DS_Engine engine, uint32_t sy, const char* yes, const char* no)
{
    uint16_t unicode[strlen(yes) > strlen(no) ? strlen(yes) : strlen(no)];
    uint32_t len, width, box_width, i;
    const char* string;
	uint16_t* screen = DS2_GetScreen(engine);

    len= 0;
    string= yes;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            len++;
        }
    }
    width= BDF_cut_unicode(unicode, len, 0, 3);
    
    len= 0;
    string= no;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            len++;
        }
    }
    i= BDF_cut_unicode(unicode, len, 0, 3);

    if(width < i)   width= i;
    box_width= 64;
    if(box_width < (width +6)) box_width = width +6;

	sy = (DS_SCREEN_HEIGHT + ICON_MSG.y) / 2 - 8 - ICON_BUTTON.y;

	uint32_t left_sx = DS_SCREEN_WIDTH / 2 - 8 - ICON_BUTTON.x,
	    right_sx = DS_SCREEN_WIDTH / 2 + 8;

	show_icon(screen, &ICON_BUTTON, left_sx, sy);
    draw_string_vcenter(screen, left_sx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, yes);

	show_icon(screen, &ICON_BUTTON, right_sx, sy);
    draw_string_vcenter(screen, right_sx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, no);

	DS2_UpdateScreen(engine);

    gui_action_type gui_action = CURSOR_NONE;
    while((gui_action != CURSOR_SELECT)  && (gui_action != CURSOR_BACK))
    {
        gui_action = get_gui_input();
	if (gui_action == CURSOR_TOUCH)
	{
		struct DS_InputState inputdata;
		DS2_GetInputState(&inputdata);
		// Turn it into a SELECT (A) or BACK (B) if the button is touched.
		if (inputdata.touch_y >= sy && inputdata.touch_y < sy + ICON_BUTTON.y)
		{
			if (inputdata.touch_x >= left_sx && inputdata.touch_x < left_sx + ICON_BUTTON.x)
				gui_action = CURSOR_SELECT;
			else if (inputdata.touch_x >= right_sx && inputdata.touch_x < right_sx + ICON_BUTTON.x)
				gui_action = CURSOR_BACK;
		}
	}
	usleep(16667);  // TODO Replace this with waiting for interrupts
    }

    if (gui_action == CURSOR_SELECT)
        return 1;
    else
        return 0;
}

/*
*	Draw hotkey dialog
*	Returns DS keys pressed, as in ds2io.h.
*/
uint32_t draw_hotkey_dialog(enum DS_Engine engine, uint32_t sy, const char* clear, const char* cancel)
{
    uint16_t unicode[strlen(clear) > strlen(cancel) ? strlen(clear) : strlen(cancel)];
    uint32_t len, width, box_width, i;
    const char* string;
	uint16_t* screen = DS2_GetScreen(engine);
	struct DS_InputState inputdata;

    len= 0;
    string= clear;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            len++;
        }
    }
    width= BDF_cut_unicode(unicode, len, 0, 3);
    
    len= 0;
    string= cancel;
    while(*string)
    {
        string= utf8decode(string, &unicode[len]);
        if(unicode[len] != 0x0D && unicode[len] != 0x0A)
        {
            len++;
        }
    }
    i= BDF_cut_unicode(unicode, len, 0, 3);

    if(width < i)   width= i;
    box_width= 64;
    if(box_width < (width +6)) box_width = width +6;

    i= DS_SCREEN_WIDTH/2 - box_width - 2;
	show_icon(screen, &ICON_BUTTON, 49, 128);
    draw_string_vcenter(screen, 51, 130, 73, COLOR_WHITE, clear);

    i= DS_SCREEN_WIDTH/2 + 3;
	show_icon(screen, &ICON_BUTTON, 136, 128);
    draw_string_vcenter(screen, 138, 130, 73, COLOR_WHITE, cancel);

	DS2_UpdateScreen(engine);

	// This function has been started by a key press. Wait for it to end.
	DS2_AwaitNoButtons();

	// While there are no keys pressed, wait for keys.
	DS2_AwaitInputChange(&inputdata);

	// Now, while there are keys pressed, keep a tally of keys that have
	// been pressed. (IGNORE TOUCH AND LID! Otherwise, closing the lid or
	// touching to get to the menu will do stuff the user doesn't expect.)
	uint32_t TotalKeys = 0;

	while (true) {
		TotalKeys |= inputdata.buttons & ~(DS_BUTTON_TOUCH | DS_BUTTON_LID);
		// If there's a touch on either button, turn it into a
		// clear (A) or cancel (B) request.
		if (inputdata.buttons & DS_BUTTON_TOUCH)
		{
			if (inputdata.touch_y >= 128 && inputdata.touch_y < 128 + ICON_BUTTON.y)
			{
				if (inputdata.touch_x >= 49 && inputdata.touch_x < 49 + ICON_BUTTON.x)
					return DS_BUTTON_A;
				else if (inputdata.touch_x >= 136 && inputdata.touch_x < 136 + ICON_BUTTON.x)
					return DS_BUTTON_B;
			}
		}

		if (inputdata.buttons == 0 && TotalKeys != 0)
			break;

		DS2_AwaitInputChange(&inputdata);
	}

	return TotalKeys;
}

/*
*	Drawing scroll bar
*/
#define SCROLLBAR_COLOR1 BGR555( 8,  2,  0)
#define SCROLLBAR_COLOR2 BGR555(15, 15, 15)

void scrollbar(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t ex, uint32_t ey, uint32_t all, uint32_t view, uint32_t now)
{
	uint32_t scrollbar_sy;
	uint32_t scrollbar_ey;
	uint32_t len;

	len = ey - sy - 2;

	if ((all != 0) && (all > now))
		scrollbar_sy = (uint32_t)((float)len * (float)now / (float)all) +sy + 1;
	else
		scrollbar_sy = sy + 1;

	if ((all > (now + view)) && (all != 0))
		scrollbar_ey = (uint32_t)((float)len * (float)(now + view) / (float)all ) + sy + 1;
	else
		scrollbar_ey = len + sy + 1;

	drawbox(screen, sx, sy, ex, ey, COLOR_BLACK);
	drawboxfill(screen, sx + 1, sy + 1, ex - 1, ey - 1, SCROLLBAR_COLOR1);
	drawboxfill(screen, sx + 1, scrollbar_sy, ex - 1, scrollbar_ey, SCROLLBAR_COLOR2);
}

/*
* Loads GUI icons corresponding to the given language_id.
*
* In:
*   language_id: The sequence number of the language to load language-specific
*     icons for.
* Returns:
*   0 on success.
*   1 if there is insufficient space in gui_picture for all image data
*   (this is an internal error).
*   -(n + 1) if an error occurred while loading any icon, with 'n' being the
*   index of the first icon for which an error occurred.
*/
int gui_change_icon(uint32_t language_id)
{
	char path[MAX_PATH];
	char suffix[15];
	size_t i, item_count = sizeof(gui_icon_list) / sizeof(gui_icon_list[0]);
	int err, ret = 0;
	uint16_t *icondst = gui_picture;

	sprintf(suffix, "%" PRIu32 ".bmp", language_id);
	for (i = 0; i < item_count; i++) {
		sprintf(path, "%s/%s/%s%s", main_path, GUI_SOURCE_PATH, gui_icon_list[i].iconname, suffix);

		if (icondst + gui_icon_list[i].x * gui_icon_list[i].y
		 >= &gui_picture[GUI_PIC_BUFSIZE]) {
			ret = 1;
			break;
		}

		gui_icon_list[i].iconbuff = NULL;
		err = BMP_Read(path, icondst, gui_icon_list[i].x, gui_icon_list[i].y);
		if (err != BMP_OK) {
			sprintf(path, "%s/%s/%s%s", main_path, GUI_SOURCE_PATH, gui_icon_list[i].iconname, ".bmp");
			err = BMP_Read(path, icondst, gui_icon_list[i].x, gui_icon_list[i].y);
		}

		if (err == BMP_OK) {
			gui_icon_list[i].iconbuff = icondst;
			icondst += gui_icon_list[i].x * gui_icon_list[i].y;
		} else {
			if (ret == 0) ret = -(i+1);
		}
	}

	return ret;
}

/*************************************************************/
int icon_init(uint32_t language_id)
{
	return gui_change_icon(language_id);
}

int color_init()
{
	char path[MAX_PATH];
	char current_line[256];

	sprintf(path, "%s/%s/%s", main_path, GUI_SOURCE_PATH, "uicolors.txt");
	FILE* fp = fopen(path, "r");
	if (fp != NULL) {
		while (fgets(current_line, 256, fp)) {
			char* colon = strchr(current_line, ':');
			if (colon) {
				*colon = '\0';
				uint16_t* color = NULL;
				if (strcasecmp(current_line, "Background") == 0)
					color = &COLOR_BG;
				else if (strcasecmp(current_line, "ActiveItem") == 0)
					color = &COLOR_ACTIVE_ITEM;
				else if (strcasecmp(current_line, "InactiveItem") == 0)
					color = &COLOR_INACTIVE_ITEM;
				else if (strcasecmp(current_line, "MessageText") == 0)
					color = &COLOR_MSSG;
				else if (strcasecmp(current_line, "ActiveMain") == 0)
					color = &COLOR_ACTIVE_MAIN;
				else if (strcasecmp(current_line, "InactiveMain") == 0)
					color = &COLOR_INACTIVE_MAIN;

				if (color != NULL) {
					char* ptr = colon + 1;
					char* end = strchr(ptr, '\0') - 1;
					while (*end && (*end == '\r' || *end == '\n'))
						*end-- = '\0';
					while (*ptr && *ptr == ' ')
						ptr++;
					uint32_t color32;
					uint8_t  r, g, b;
					if (strlen(ptr) == 7 && *ptr == '#')
					{
						color32 = strtol(ptr + 1, NULL, 16);
						r = (color32 >> 16) & 0xFF;
						g = (color32 >>  8) & 0xFF;
						b =  color32        & 0xFF;
						*color = BGR555(b >> 3, g >> 3, r >> 3);
					}
				}
			}
		}

		fclose(fp);
		return 0;
	}
	else
		return 1;
}

/*************************************************************/
void show_icon(uint16_t* screen, struct gui_iconlist* icon, uint32_t x, uint32_t y)
{
	uint32_t i, k;
	uint16_t *src = icon->iconbuff, *dst = VRAM_POS(screen, x, y);

	if (!src) return;  /* The icon failed to load */

	if (icon->x == DS_SCREEN_WIDTH && icon->y == DS_SCREEN_HEIGHT && x == 0 && y == 0) {
		// Don't support transparency for a background.
		memcpy(dst, src, DS_SCREEN_WIDTH * DS_SCREEN_HEIGHT * sizeof(uint16_t));
	} else {
		for (i = 0; i < icon->y; i++) {
			for (k = 0; k < icon->x; k++) {
				if (*src != 0x03E0) dst[k] = *src;
				src++;
			}

			dst += DS_SCREEN_WIDTH;
		}
	}
}

/*************************************************************/
void show_Vscrollbar(uint16_t* screen, uint32_t x, uint32_t y, uint32_t part, uint32_t total)
{
//    show_icon(screen, ICON_VSCROL_UPAROW, x+235, y+55);
//    show_icon(screen, ICON_VSCROL_DWAROW, x+235, y+167);
//    show_icon(screen, ICON_VSCROL_SLIDER, x+239, y+64);
//    if(total <= 1)
//        show_icon(screen, ICON_VSCROL_BAR, x+236, y+64);
//    else
//        show_icon(screen, ICON_VSCROL_BAR, x+236, y+64+(part*90)/(total-1));
}

/*
* Displays the boot logo. No error is returned if the logo is not present;
* the screen is simply not overwritten.
*/
void show_log()
{
	char tmp_path[MAX_PATH];

	sprintf(tmp_path, "%s/%s", main_path, BOOTLOGO);

	BMP_Read(tmp_path, DS2_GetSubScreen(), DS_SCREEN_WIDTH, DS_SCREEN_HEIGHT);
}

void ReGBA_ProgressInitialise(enum ReGBA_FileAction Action)
{
}

void ReGBA_ProgressUpdate(uint32_t Current, uint32_t Total)
{
}

void ReGBA_ProgressFinalise()
{
}
