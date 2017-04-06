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

#define VRAM_POS(screen, x, y)  ((screen) + ((x) + (y) * DS_SCREEN_WIDTH))

#define BOOTLOGO "SYSTEM/GUI/boot.bmp"
#define GUI_SOURCE_PATH "SYSTEM/GUI"

/* This was calculated from the size of all icons below. If any icons are
 * added, update this value. */
#define GUI_PIC_BUFSIZE 179026

uint16_t gui_picture[GUI_PIC_BUFSIZE] __attribute__((section(".noinit")));

struct gui_icon gui_icons[] = {
	/* 00 */ { "smaybgo", 256, 192, NULL },
	/* 01 */ { "ssubbg", 256, 192, NULL },
	/* 02 */ { "subsela", 245, 22, NULL },
	/* File type icons. */
	/* 03 */ { "zipfile", 16, 16, NULL },
	/* 04 */ { "directory", 16, 16, NULL },
	/* 05 */ { "gbafile", 16, 16, NULL },

	/* Title background. */
	/* 06 */ { "stitle", 256, 33, NULL },
	/* Main menu icons. */
	/* 07 */ { "savo", 52, 52, NULL },
	/* 08 */ { "ssaveo", 52, 52, NULL },
	/* 09 */ { "stoolo", 52, 52, NULL },
	/* 10 */ { "scheato", 52, 52, NULL },
	/* 11 */ { "sother", 52, 52, NULL },
	/* 12 */ { "sexito", 52, 52, NULL },
	/* 13 */ { "smsel", 79, 15, NULL },
	/* 14 */ { "smnsel", 79, 15, NULL },

	/* 15 */ { "snavo", 52, 52, NULL },
	/* 16 */ { "snsaveo", 52, 52, NULL },
	/* 17 */ { "sntoolo", 52, 52, NULL },
	/* 18 */ { "sncheato", 52, 52, NULL },
	/* 19 */ { "snother", 52, 52, NULL },
	/* 20 */ { "snexito", 52, 52, NULL },

	/* Other things. */
	/* 21 */ { "sunnof", 16, 16, NULL },
	/* 22 */ { "smaini", 85, 38, NULL },
	/* 23 */ { "snmaini", 85, 38, NULL },

	/* 24 */ { "sticon", 29, 13, NULL },

	/* 25 */ { "sfullo", 12, 12, NULL },
	/* 26 */ { "snfullo", 12, 12, NULL },
	/* 27 */ { "semptyo", 12, 12, NULL },
	/* 28 */ { "snemptyo", 12, 12, NULL },
	/* 29 */ { "fdoto", 16, 16, NULL },
	/* 30 */ { "backo", 19, 13, NULL },
	/* 31 */ { "nbacko", 19, 13, NULL },
	/* 32 */ { "chtfile", 16, 15, NULL },
	/* 33 */ { "smsgfr", 193, 111, NULL },
	/* 34 */ { "sbutto", 76, 16, NULL }
};

uint16_t COLOR_BG            = BGR555( 0,  0,  0);
uint16_t COLOR_INACTIVE_ITEM = BGR555( 0,  0,  0);
uint16_t COLOR_ACTIVE_ITEM   = BGR555(31, 31, 31);
uint16_t COLOR_MSSG          = BGR555( 0,  0,  0);
uint16_t COLOR_INACTIVE_MAIN = BGR555(31, 31, 31);
uint16_t COLOR_ACTIVE_MAIN   = BGR555(31, 31, 31);

void draw_message_box(uint16_t* screen)
{
	show_icon(screen, &ICON_MSG, (DS_SCREEN_WIDTH - ICON_MSG.x) / 2, (DS_SCREEN_HEIGHT - ICON_MSG.y) / 2);
}

void draw_string_vcenter(uint16_t* screen, uint32_t sx, uint32_t sy, uint32_t width, uint32_t color, const char* string)
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
		m = BDF_CutUCS2s(&unicode[i], num - i, width);
		x = (width - BDF_WidthUCS2s(&unicode[i], m)) / 2;
		while (m--) {
			x += BDF_RenderUCS2(screenp + x, DS_SCREEN_WIDTH, COLOR_TRANS,
				color, unicode[i++]);
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
			x += BDF_RenderUCS2(buff_fonts + x, textWidth, bg_color, fg_color, ucs2);
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

bool draw_yesno_dialog(enum DS_Engine engine, const char* yes, const char* no)
{
	uint16_t* screen = DS2_GetScreen(engine);

	uint32_t sy = (DS_SCREEN_HEIGHT + ICON_MSG.y) / 2 - 8 - ICON_BUTTON.y,
	         lx = DS_SCREEN_WIDTH / 2 - 8 - ICON_BUTTON.x,
	         rx = DS_SCREEN_WIDTH / 2 + 8;

	show_icon(screen, &ICON_BUTTON, lx, sy);
	draw_string_vcenter(screen, lx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, yes);

	show_icon(screen, &ICON_BUTTON, rx, sy);
	draw_string_vcenter(screen, rx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, no);

	DS2_UpdateScreen(engine);

	gui_action_type gui_action = CURSOR_NONE;
	while (gui_action != CURSOR_SELECT && gui_action != CURSOR_BACK) {
		gui_action = get_gui_input();

		if (gui_action == CURSOR_TOUCH) {
			struct DS_InputState inputdata;
			DS2_GetInputState(&inputdata);
			// Turn it into a SELECT (A) or BACK (B) if the button is touched.
			if (inputdata.touch_y >= sy && inputdata.touch_y < sy + ICON_BUTTON.y) {
				if (inputdata.touch_x >= lx && inputdata.touch_x < lx + ICON_BUTTON.x)
					gui_action = CURSOR_SELECT;
				else if (inputdata.touch_x >= rx && inputdata.touch_x < rx + ICON_BUTTON.x)
					gui_action = CURSOR_BACK;
			}
		}
		usleep(16667);  // TODO Replace this with waiting for interrupts
	}

	return gui_action == CURSOR_SELECT;
}

uint16_t draw_hotkey_dialog(enum DS_Engine engine, const char* clear, const char* cancel)
{
	uint16_t* screen = DS2_GetScreen(engine);
	struct DS_InputState inputdata;

	uint32_t sy = (DS_SCREEN_HEIGHT + ICON_MSG.y) / 2 - 8 - ICON_BUTTON.y,
	         lx = DS_SCREEN_WIDTH / 2 - 8 - ICON_BUTTON.x,
	         rx = DS_SCREEN_WIDTH / 2 + 8;

	show_icon(screen, &ICON_BUTTON, lx, sy);
	draw_string_vcenter(screen, lx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, clear);

	show_icon(screen, &ICON_BUTTON, rx, sy);
	draw_string_vcenter(screen, rx + 2, sy, ICON_BUTTON.x - 4, COLOR_WHITE, cancel);

	DS2_UpdateScreen(engine);

	// This function has been started by a key press. Wait for it to end.
	DS2_AwaitNoButtons();

	// While there are no keys pressed, wait for keys.
	DS2_AwaitInputChange(&inputdata);

	// Now, while there are keys pressed, keep a tally of keys that have
	// been pressed. (IGNORE TOUCH AND LID! Otherwise, closing the lid or
	// touching to get to the menu will do stuff the user doesn't expect.)
	uint32_t TotalKeys = 0;

	while (1) {
		TotalKeys |= inputdata.buttons & ~(DS_BUTTON_TOUCH | DS_BUTTON_LID);
		// If there's a touch on either button, turn it into a
		// clear (A) or cancel (B) request.
		if (inputdata.buttons & DS_BUTTON_TOUCH) {
			if (inputdata.touch_y >= 128 && inputdata.touch_y < 128 + ICON_BUTTON.y) {
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

int gui_change_icon(uint32_t language_id)
{
	char path[MAX_PATH];
	char suffix[15];
	size_t i, item_count = sizeof(gui_icons) / sizeof(gui_icons[0]);
	int err, ret = 0;
	uint16_t *dst = gui_picture;

	sprintf(suffix, "%" PRIu32 ".bmp", language_id);
	for (i = 0; i < item_count; i++) {
		sprintf(path, "%s/%s/%s%s", main_path, GUI_SOURCE_PATH, gui_icons[i].name, suffix);

		if (dst + gui_icons[i].x * gui_icons[i].y > &gui_picture[GUI_PIC_BUFSIZE]) {
			ret = 1;
			break;
		}

		gui_icons[i].data = NULL;
		err = BMP_Read(path, dst, gui_icons[i].x, gui_icons[i].y);
		if (err != BMP_OK) {
			sprintf(path, "%s/%s/%s%s", main_path, GUI_SOURCE_PATH, gui_icons[i].name, ".bmp");
			err = BMP_Read(path, dst, gui_icons[i].x, gui_icons[i].y);
		}

		if (err == BMP_OK) {
			gui_icons[i].data = dst;
			dst += gui_icons[i].x * gui_icons[i].y;
		} else {
			if (ret == 0) ret = -(i+1);
		}
	}

	return ret;
}

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

void show_icon(uint16_t* screen, const struct gui_icon* icon, uint32_t x, uint32_t y)
{
	uint32_t i, k;
	const uint16_t* src = icon->data;
	uint16_t* dst = VRAM_POS(screen, x, y);

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

void show_logo()
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
