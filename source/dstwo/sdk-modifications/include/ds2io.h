#ifndef __DS2IO_H__
#define __DS2IO_H__

#ifndef BIT
#define BIT(a) (1<<a)
#endif

#define SCREEN_WIDTH	256
#define SCREEN_HEIGHT	192

#define	AUDIO_BUFFER_COUNT 4 // in 1.2, that's 4, but in 0.13 it would be 16 [Neb]

#ifdef __cplusplus
extern "C" {
#endif

typedef struct touchPosition {
	signed short	x;
	signed short	y;
} touchPosition;

typedef struct T_INPUT
{
    unsigned int keysHeld;
    unsigned int keysUp;
    unsigned int keysDown;
    unsigned int keysDownRepeat;
    touchPosition touchPt;
    touchPosition movedPt;
    int touchDown;
    int touchUp;
    int touchHeld;
    int touchMoved;
}INPUT;

typedef enum KEYPAD_BITS {
	KEY_A      = BIT(0),  //!< Keypad A button.
	KEY_B      = BIT(1),  //!< Keypad B button.
	KEY_SELECT = BIT(2),  //!< Keypad SELECT button.
	KEY_START  = BIT(3),  //!< Keypad START button.
	KEY_RIGHT  = BIT(4),  //!< Keypad RIGHT button.
	KEY_LEFT   = BIT(5),  //!< Keypad LEFT button.
	KEY_UP     = BIT(6),  //!< Keypad UP button.
	KEY_DOWN   = BIT(7),  //!< Keypad DOWN button.
	KEY_R      = BIT(8),  //!< Right shoulder button.
	KEY_L      = BIT(9),  //!< Left shoulder button.
	KEY_X      = BIT(10), //!< Keypad X button.
	KEY_Y      = BIT(11), //!< Keypad Y button.
	KEY_TOUCH  = BIT(12), //!< Touchscreen pendown.
	KEY_LID    = BIT(13)  //!< Lid state.
} KEYPAD_BITS;

struct rtc{
    volatile unsigned char year;		//add 2000 to get 4 digit year
    volatile unsigned char month;		//1 to 12
    volatile unsigned char day;			//1 to (days in month)

    volatile unsigned char weekday;		// day of week
    volatile unsigned char hours;		//0 to 11 for AM, 52 to 63 for PM
    volatile unsigned char minutes;		//0 to 59
    volatile unsigned char seconds;		//0 to 59
};

struct key_buf
{
    unsigned short key;
    unsigned short x;
    unsigned short y;
};

typedef enum SCREEN_ID
{
    UP_SCREEN = 1,
    DOWN_SCREEN = 2,
    DUAL_SCREEN = 3,
    JOINTUSE_SCREEN = 3
} SCREEN_ID;

#define UP_MASK		0x1
#define DOWN_MASK	0x2
#define DUAL_MASK	0x3

//video buffer address of up screen
extern void* up_screen_addr;

//video buffer address of down screen
extern void* down_screen_addr;

//every time call ds2_updateAudio() function, the ds2io layer will transfer
//audio_samples_per_trans *4 bytes audio data to NDS
extern unsigned int audio_samples_per_trans;

/*
*	Function: initialize ds2 I/O(DS2 Input and Output) layer
*	audio_samples_lenght: ds2io layer's audio buffer length, it unit is	sample
*		which is fixed 4(ds2io use stereo and 16-bit audio) bytes,
*		audio_samples_lenght sholud be 128*n, 128 <= audio_samples_lenght <= 4096
*	NOTE: the audio sampling frequence be fixed to 44.1KHz, 2 channels 16-bit
*/
extern int ds2io_init(int audio_samples_lenght);

/*
*	Function: initialize ds2 I/O(DS2 Input and Output) layer (b version)
*	audio_samples_lenght: ds2io layer's audio buffer length, it unit is	sample
*		which is fixed 4(ds2io use stereo and 16-bit audio) bytes,
*		audio_samples_lenght sholud be 128*n, 128 <= audio_samples_lenght <= 4096
*	audio_samples_freq: audio samples frequence, it should be among 44100, 22050,
*			11025
*	reserved1: reserved for future using
*	reserved2: reserved for future using
*	NOTE: the audio samples are 2 channels 16-bit
*/
extern int ds2io_initb(int audio_samples_lenght, int audio_samples_freq, int reserved1,
	int reserved2);

/*
*	Function: update video data from buffer to screen, the ds2io layer have 2 video
*		buffers for up screen and 2 video buffers for down screen, everytime
*		ds2_flipScreen is called, up_screen_addr and/or down_buffer_addr
*		point to the other buffer, but not always do so, see below.
*	screen_num: UP_SCREEN, only update up screen
*				DOWN_SCREEN, only update down screen
*				DUAL_SCREEN, update both up screen and down screen
*	done: after updating video data, the up_screen_addr and/or down_buffer_addr
*		are ready to point the other buffer, but if the other buffer is busy:
*		when done = 0, ds2_flipScreen returns without change up_screen_addr
*			and/or down_buffer_addr pointer. it will not sure the graphic just
*			updated will appear on the screen
*		when done = 1, it will WAIT untill the other buffer idle and change
*			the pointers and returns. the graphic just updated will appear on
*			the screen, but please noting the word "WAIT"
*		when done = 2, it will WAIT untill the other buffer idle, then return
*			without change the pointers, it is convenient for GUI drawing
*/
extern void ds2_flipScreen(enum SCREEN_ID screen_num, int done);

/*
*	Function: set the video buffer to a single color
*	screen_num: UP_SCREEN, only set up screen buffer
*				DOWN_SCREEN, only set down screen buffer
*				DUAL_SCREEN, set both up screen and down screen buffer
*/
extern void ds2_clearScreen(enum SCREEN_ID screen_num, unsigned short color);


/*
*	Function: there are AUDIO_BUFFER_COUNT audio buffers on the ds2io layer, this function to
*		check how many buffers are occupied
*/
extern int ds2_checkAudiobuff(void);

/*
*	Function: get audio buffer address
*	NOTE: ds2_getAudiobuff may return NULL, even if ds2_checkAudiobuff() < AUDIO_BUFFER_COUNT.
*		The fact are that, AUDIO_BUFFER_COUNT audio buffers are on NDS, the ds2io layer using
*		2 other buffers transfering data to the AUDIO_BUFFER_COUNT audio buffers alternately,
*		this function checks the 2 buffers in ds2io layers whether are occupied,
*		it will return the address of the idle buffer, else it return NULL
*/
extern void* ds2_getAudiobuff(void);

/*
*	Function: flush audio data from buffer to ds2io layer
	NOTE: execution of the function ds2_updateAudio() only put audio data transfer request to wait queue,
	it is not equal that the audio data reach NDS buffer.
*/
extern void ds2_updateAudio(void);


/*
*	Function: set audio info
*	audio_samples_freq:  default freq 44100Hz
*   audio_sample_bit: 16bit
*	audio_samples_lenght: sholud be 128*n, 128 <= audio_samples_lenght <= 4096
*   channels: 2
*   data_format: 0: interleave(L R L R L R L R ...) 1: group(L L L L ...R R R R ...)
*/
void ds2_setAudio( unsigned int audio_samples_freq, int audio_sample_bit, unsigned int audio_samples_lenght, int channels, int data_format );


#ifndef RGB15
#define RGB15(r,g,b)  (((r)|((g)<<5)|((b)<<10))|BIT(15))
#endif

/*
*	Functin: get time
*/
extern void ds2_getTime(struct rtc *time);

/*
*	Function: get brightness of the screens
*	return: fours levels 0, 1, 2, 3
*/
extern int ds2_getBrightness(void);

/*
*	Function: set brightness of the screens
*	Input: level, there are 4 levels, 0, 1, 2 and 3
*/
extern void ds2_setBrightness(int level);

/*
*	Function: get the swaping state of the screens
*/
extern int ds2_getSwap(void);

/*
*	Funciotn: swap up screen and down screen
*/
extern void ds2_setSwap(int swap);

/*
*	Function: get backlight status
*	input bit0 = 0 set down screen's backlight off
*			bit0 = 1 set down screen's backlight on
*			bit1 = 0 set up screen's backlight off
*			bit1 = 1 set up screen's backlight on
*/
extern void ds2_setBacklight(int backlight);

/*
*	Function: get backlight status
*	return bit0 = 0 the down screen's backlight is off
*			bit0 = 1 the down screen's backlight is on
*			bit1 = 0 the up screen's backlight is off
*			bit1 = 1 the up screen's backlight is on
*/
extern int ds2_getBacklight(void);

/*
*	Function: system suspend
*/
extern void ds2_setSupend(void);

/*
*	Function: system wakeup
*/
extern void ds2_wakeup(void);

/*
*	Function: NDS power offf
*/
extern void ds2_shutdown(void);

/*
*	Function: set volume of NDS
*	Input: volume 0- mute
*			1 - 127 hardware adjust the volume
*			128-255 software adjust the volume, when close to 255, the sound may
*				be saturation distortion
*/
extern void ds2_setVolume(int volume);

/*
*	Funciton: get key value and touch screen position value
*/
extern void ds2_getrawInput(struct key_buf *input);

/*
*	Function: system exit, return to DSTWO Menu
*/
extern void ds2_plug_exit(void);

/*
*	Function: return ds2sdk version string
*/
extern const char* ds2_getVersion(void);

/*
*	Function: Register a function for debug purpos, such as CONSOLE. when keys
*		pressed, function fun will be called in the interruption
*/
extern void regist_escape_key(void (*fun)(void), unsigned int keys);

/*
*	Function: release the function fointer registered by regist_escape_key
*/
extern void release_escape_key(void);

#ifdef __cplusplus
}
#endif

#endif //__DS2IO_H__

