#ifndef __DRVMEMMGR__H_
#define __DRVMEMMGR__H_

#define ENABLE_MALLOC

extern unsigned int alloc(unsigned int nbytes);
extern void deAlloc(unsigned int address);
extern unsigned int Drv_realloc(unsigned int address, unsigned int nbytes);
extern unsigned int Drv_calloc(unsigned int size, unsigned int n);

#ifdef ENABLE_MALLOC
#define malloc alloc
#define calloc Drv_calloc
#define realloc Drv_realloc
#define free deAlloc
#endif

#endif //__DRVMEMMGR__H_
