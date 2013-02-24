#ifndef __DS2_MALLOC_H__
#define __DS2_MALLOC_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void heapInit(unsigned int start, unsigned int end);

extern void* Drv_alloc(unsigned int nbytes);
extern void Drv_deAlloc(void* address);
extern void* Drv_realloc(void* address, unsigned int nbytes);
extern void* Drv_calloc(unsigned int nmem, unsigned int size);

#ifdef __cplusplus
}
#endif

#define malloc		Drv_alloc
#define calloc		Drv_calloc
#define realloc		Drv_realloc
#define free		Drv_deAlloc

#ifdef __cplusplus
#include <stdio.h>
inline void* operator new ( size_t s ) { return malloc( s ); }
inline void* operator new[] ( size_t s ) { return malloc( s ); }
inline void operator delete ( void* p ) { free( p ); }
inline void operator delete[] ( void* p ) { free( p ); }
#endif

#endif //__DS2_MALLOC_H__
