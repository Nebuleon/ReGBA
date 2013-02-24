#ifndef __TCM_H
#define __TCM_H

/*
*	Function: makes cpu to idle state. It will be waken up by RTC in the next second.
*   Of course, it can be waken up by keys at any time.
*/
int enable_enter_idle(void);

#endif
