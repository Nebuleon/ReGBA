//console.c

#include <stdio.h>
#include <stdarg.h>
#include "ds2io.h"
#include "memory.h"
#include "font_dot.h"

#define STRING_SIZE		2048

#define CONSOLE_WIDTH	32
#define CONSOLE_HEIGHT	24
#define TAB_SIZE		3

static void ConsoleView(void);
static void ConsoleDrawfontall(void);

static int console_init_done = 0;
static unsigned short f_color;
static unsigned short b_color;
static enum SCREEN_ID console_id;

static int print_row;
static int print_col;
static int print_row_saved;
static int print_col_saved;

static unsigned char*	console_buf;
static unsigned int		console_buf_size;
static unsigned char*	console_buf_front;
static unsigned char*	console_buf_end;
static unsigned char*	console_print_header;
static unsigned short*	console_screen;
static unsigned char*	print_header_saved;

static void ConsoleFlush(void)
{
	unsigned short* screen_addr;
	enum SCREEN_ID id;

	if(console_id & UP_MASK) {
		screen_addr = up_screen_addr;
		id = UP_SCREEN;
	}
	else {
		screen_addr = down_screen_addr;
		id = DOWN_SCREEN;
	}

	memcpy((void*)screen_addr, (void*)console_screen, SCREEN_WIDTH*SCREEN_HEIGHT*2);
	ds2_flipScreen(id, 1);
}

static void ConsoleClearscreen(void)
{
	unsigned short *scr;
	unsigned int i;

	scr = console_screen;
	i = 0;
	while(i < SCREEN_WIDTH*SCREEN_HEIGHT)
		scr[i++] = b_color;
}

static void ConsoleMovewin(int dir, int sw_screen)
{
	unsigned char *pt;
	
	if(dir || sw_screen)
		ConsoleClearscreen();

	//switch to another screen to dispaly text
	if(sw_screen)
	{
		ConsoleFlush();
		//now up screen
		if(console_id & UP_MASK)	{
			console_screen = down_screen_addr;
			console_id = DOWN_SCREEN;
		}
		//switch to up screen
		else
		{
			console_screen = up_screen_addr;
			console_id = UP_SCREEN;
		}
	}

	pt = console_print_header + dir*CONSOLE_WIDTH;

	//screen scroll down
	if(dir > 0)
	{
		if(console_buf_end > console_print_header) {
			if(pt > console_buf_end)
				pt = console_buf_end;
			console_print_header = pt;
		}
		else if(console_buf_end < console_print_header)	{
			if((pt - console_buf) >= console_buf_size) {
				pt -= console_buf_size;
				if(pt > console_buf_end)
					pt = console_buf_end;
			}
			console_print_header = pt;
		}
	}
	//screen scroll up
	else if(dir < 0)
	{
		if(console_buf_front > console_print_header) {
			if(pt < console_buf) {
				pt += console_buf_size;
				if(pt < console_buf_front)
					pt = console_buf_front;
			}
			console_print_header = pt;
		}
		else if(console_buf_front < console_print_header) {
			if(pt < console_buf_front)
				pt = console_buf_front;
			console_print_header = pt;
		}
	}

	if(dir || sw_screen)
	{
		print_row_saved = 0;	//redraw entire screen
		print_col_saved = 0;

		ConsoleDrawfontall();
		ConsoleFlush();
	}
}

void ConsoleClr(int mode)
{
	unsigned char *pt, *pt_end;
	unsigned int i;

	//Clear current screen buffer
	if(0 == mode)
	{
		if(print_col > 0) {
			console_buf_end += CONSOLE_WIDTH;
			if((console_buf_end - console_buf) >= console_buf_size)
				console_buf_end -= console_buf_size;
		}

		console_print_header = console_buf_end;
		print_row = 0;
		print_col = 0;
		print_row_saved = 0;
		print_col_saved = 0;
	}
	//Clear all
	else if(1 == mode)
	{
		console_buf_front = console_buf;
		console_buf_end = console_buf;
		console_print_header = console_buf;
		print_row = 0;
		print_col = 0;
		print_row_saved = 0;
		print_col_saved = 0;

		pt = console_buf;
		pt_end = console_buf + console_buf_size;
		while(pt < pt_end)
		{
			pt[0] = '\0';
			pt += CONSOLE_WIDTH;
		}
	}

	ConsoleClearscreen();
	ConsoleFlush();
}

//Draw part of the screen
static void ConsoleDrawfont(void)
{
	unsigned char *pt, *dot_map;
	unsigned char ch, dot;
	unsigned short *dst, *pt_r;
	unsigned int row, col;
	unsigned int x, j, k;

	pt_r = console_screen;
	row = print_row_saved;
	col = print_col_saved;
	x = col*8;

	pt = console_print_header + row * CONSOLE_WIDTH;

	while(row != print_row || col != print_col)
	{
		ch = pt[col++] & 0x7F;
		//'\n'
		if('\n' == ch || '\0' == ch || col > CONSOLE_WIDTH) {
			pt += CONSOLE_WIDTH;
			if((pt - console_buf) >= console_buf_size) pt -= console_buf_size;
			col = 0;
			row += 1;
			x = 0;
		}
		//character not '\n' nor '\0'
		else {
			dot_map = (unsigned char*)font_map[ch];

			for(j= 0; j < 8; j++)
			{
				dot = *dot_map++;
				dst = pt_r + (row*8+j)*SCREEN_WIDTH + x;
				for(k = 0; k < 8; k++)
					*dst++ = (dot & (0x80>>k)) ? f_color : b_color;
			}
			x += 8;
		}
	}
}

//Redraw the hole screen
static void ConsoleDrawfontall(void)
{
	unsigned char *pt, *end_pt, *dot_map;
	unsigned int i, j, k;
	unsigned char ch, dot;
	unsigned short *dst, *pt_r;
	unsigned int x, y;

	//Clear screen to b_color
	pt_r = console_screen;
	i = 0;
	while(i < SCREEN_WIDTH*SCREEN_HEIGHT)
		pt_r[i++] = b_color;

	pt = console_print_header;
	end_pt = console_buf_end;
	x = 0;
	y = 0;
	i = 0;
	while(pt != end_pt)
	{
		ch = pt[i++] & 0x7F;
		//'\n'
		if('\n' == ch || '\0' == ch || i > CONSOLE_WIDTH) {
			pt += CONSOLE_WIDTH;
			if((pt - console_buf) >= console_buf_size) pt -= console_buf_size;
			i = 0;
			x = 0;
			y += 1;
			if(y >= CONSOLE_HEIGHT) break;
		}
		//character not '\n' nor '\0'
		else {
			dot_map = (unsigned char*)font_map[ch];

			for(j= 0; j < 8; j++)
			{
				dot = *dot_map++;
				dst = pt_r + (y*8+j)*SCREEN_WIDTH + x;
				for(k = 0; k < 8; k++)
					*dst++ = (dot & (0x80>>k)) ? f_color : b_color;
			}
			x += 8;
		}
	}
}

static void ConsoleNewline(void)
{
	print_row += 1;
	if(print_row >= CONSOLE_HEIGHT)
	{
		print_row -= 1;
		console_print_header += CONSOLE_WIDTH;
		if((console_print_header - console_buf) >= console_buf_size)
			console_print_header = console_buf;

		print_row_saved = 0;
		print_col_saved = 0;

		ConsoleClearscreen();
	}

	console_buf_end += CONSOLE_WIDTH;
	if((console_buf_end - console_buf) >= console_buf_size)
		console_buf_end = console_buf;

	//scrollback
	if(console_buf_end == console_buf_front)
	{
		console_buf_front += CONSOLE_WIDTH;
		if((console_buf_front - console_buf) >= console_buf_size)
			console_buf_front = console_buf;

		console_buf_end[0] = '\0';
	}
}

static void ConsolePrintchar(unsigned char ch)
{
	int i;

	if(print_col >= CONSOLE_WIDTH) {
		print_col = 0;
		ConsoleNewline();
	}

	switch(ch) {
		case 9:									//'\t'
			if((print_col + TAB_SIZE) < CONSOLE_WIDTH)
			{
				i = print_col % TAB_SIZE;
				i = TAB_SIZE - i;
				while(i--)
				{
					console_buf_end[print_col] = ' ';
					print_col += 1;
				}
			}
			break;
		case 10:								//'\n'
		case 13:								//'\r'
			console_buf_end[print_col] = '\n';
			print_col = 0;
			ConsoleNewline();
			break;
		default:
			console_buf_end[print_col] = ch;
			if(ch != '\0')
				print_col += 1;
			break;
	}
}

void ConsolePrintstring(unsigned char* string)
{
	unsigned char *pt;
	unsigned char ch;

	print_row_saved = print_row;
	print_col_saved = print_col;
	console_print_header = print_header_saved;
//cprintf("print_row %d; print_col %d; [%s]\n", print_row, print_col, string);
	pt = string;
	do
	{
		ch = *pt++;
		ConsolePrintchar(ch);
	}
	while ('\0' != ch);

	print_header_saved = console_print_header;

	ConsoleDrawfont();
	ConsoleFlush();
}

//---------------------------------------------------------------------------------
//parameter:
//	front_color: color of character
//	background_color: background color
//	screen: UP-up screen used as output, DOWN-down screen used as output, 
//			using only one screen at a time
//	buf_size: buffer size to hold the output, it's unit is screen
//---------------------------------------------------------------------------------
int ConsoleInit(unsigned short front_color, unsigned short background_color, enum SCREEN_ID screen, unsigned int buf_size)
{
	unsigned char *pt;
	unsigned int size;
	unsigned short **scr_ppt;

	console_init_done = 0;
	f_color = front_color;
	b_color = background_color;


	//Using only one screen at a time
	if(screen & UP_MASK)
		console_id = UP_SCREEN;
	else
		console_id = DOWN_SCREEN;

	if(!buf_size) buf_size = 1;
	size = buf_size *CONSOLE_WIDTH *CONSOLE_HEIGHT;

	pt = (unsigned char*)Drv_alloc(size);
	if(NULL == pt)
		return -1;							//there is something error

	console_buf = pt;
	memset(console_buf, 0, size);

	print_row = 0;
	print_col = 0;
	print_row_saved = print_row;
	print_col_saved = print_col;
	console_buf_size = size;
	console_buf_front = console_buf;
	console_buf_end = console_buf;
	console_print_header = console_buf;
	print_header_saved = console_print_header;

	console_screen = (unsigned short*)Drv_alloc(SCREEN_WIDTH * SCREEN_HEIGHT*2);
	if(NULL == console_screen) {
		Drv_deAlloc((void*)console_buf);
		return -1;
	}

	ConsoleClr(1);

	console_init_done = 1;

	regist_escape_key(ConsoleView, (KEY_L | KEY_R | KEY_A | KEY_B | KEY_X));
	return 0;
}

void ConsoleView(void)
{
	unsigned int key;

cprintf("enter console mode\n");

	do {
		key = getKey();

		switch(key)
		{
			//screen scroll down 1 line
			case KEY_UP:
					ConsoleMovewin(-1, 0);
				break;

			//screen scroll up 1 line
			case KEY_DOWN:
					ConsoleMovewin(1, 0);
				break;

			//screen scroll done (CONSOLE_HEIGHT-1) line
			case KEY_LEFT:
					ConsoleMovewin(-(CONSOLE_HEIGHT-1), 0);
				break;

			//screen scroll up (CONSOLE_HEIGHT-1) line
			case KEY_RIGHT:
					ConsoleMovewin(CONSOLE_HEIGHT-1, 0);
				break;

			//switch screen
			case KEY_B:
cprintf("switch to another screen\n");
					ConsoleMovewin(0, 1);
				break;

			default: break;
		}

		mdelay(20);
	} while(key != KEY_Y);

cprintf("exit console mode\n");
}

int printf(const char *format, ...)
{
	char string[STRING_SIZE];
	int ret;
	va_list ap;

	if(!console_init_done)
		return 0;

	va_start (ap, format);
	ret = vsnprintf(string, STRING_SIZE, format, ap);
	va_end (ap);

	ConsolePrintstring(string);

	return ret;
}


