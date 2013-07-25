#ifndef __DS2_TYPES_H__
#define __DS2_TYPES_H__

typedef           unsigned char       u8;
typedef                    char       s8;

typedef           unsigned short      u16;
typedef                    short      s16;

typedef           unsigned int        u32;
typedef                    int        s32;

typedef           unsigned long long  u64;
typedef                    long long  s64;

typedef  volatile unsigned char       vu8;
typedef  volatile          char       vs8;

typedef  volatile unsigned short      vu16;
typedef  volatile          short      vs16;

typedef  volatile unsigned int        vu32;
typedef  volatile          int        vs32;

typedef  volatile unsigned long long  vu64;
typedef  volatile          long long  vs64;

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#endif //__DS2_TYPES_H__
