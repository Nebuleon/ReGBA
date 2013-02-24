#ifndef __DS2_TIMER_H__
#define __DS2_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
*	Function: register a timer interruptting periodly
*	channel: timer id, from 0 to 1
*	period: interrupt period, unit is us, period < 2.5s
*	handle: interrupt handle, if handle = NULL, the timer will not generate interrupt
*	arg: argument to the interrupt handle
*/
extern int initTimer(unsigned int channel, unsigned int period, void (*handle)(unsigned int), int arg);

/*
*	Function: set the timer run
*/
extern void runTimer(unsigned int channel);

/*
*	Function: stop timer
*/
extern void stopTimer(unsigned int channel);

/*
*	Function: reset timer
*/
extern void resetTimer(unsigned int channel);

/*
*	Function: read value of timer
*/
extern unsigned int readTimer(unsigned int channel);


#define SYSTIME_UNIT	42667
/*
*	Function: get the elapsed time since DS2 started
*		it's uint is 42.667us, it will overflow after 50.9 horus since DS2 started
*/
extern unsigned int getSysTime(void);

#ifdef __cplusplus
}
#endif

#endif //__DS2_TIMER_H__

