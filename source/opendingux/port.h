#ifndef _PORT_H_
#define _PORT_H_

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef uint64_t u64;

typedef FILE* FILE_TAG_TYPE;

#define MAX_PATH PATH_MAX
#define MAX_FILE PATH_MAX

#include <SDL/SDL.h>
#include <time.h>

typedef struct timespec timespec;

/* Tuning parameters for the GCW Zero version of gpSP */
/* Its processor is an Ingenic JZ4770 at 1000 MHz with 256..512 MiB of RAM */
#define READONLY_CODE_CACHE_SIZE          (4 * 1024 * 1024)
#define WRITABLE_CODE_CACHE_SIZE          (4 * 1024 * 1024)
/* The following parameter needs to be at least enough bytes to hold
 * the generated code for the largest instruction on your platform.
 * In most cases, that will be the ARM instruction
 * STMDB R0!, {R0,R1,R2,R3,R4,R5,R6,R7,R8,R9,R10,R11,R12,R13,R14,R15} */
#define TRANSLATION_CACHE_LIMIT_THRESHOLD (1024)

#define MAX_AUTO_FRAMESKIP 4

#define FILE_OPEN_APPEND ("a+")

#define FILE_OPEN_READ ("rb")

#define FILE_OPEN_WRITE ("wb")

#define FILE_OPEN(filename_tag, filename, mode)                             \
  filename_tag = fopen(filename, FILE_OPEN_##mode)                          \

#define FILE_CHECK_VALID(filename_tag)                                      \
  (filename_tag != FILE_TAG_INVALID)                                        \

#define FILE_TAG_INVALID                                                    \
  (NULL)                                                                    \

#define FILE_CLOSE(filename_tag)                                            \
  fclose(filename_tag)                                                      \

#define FILE_DELETE(filename)                                               \
  unlink(filename)                                                          \

#define FILE_READ(filename_tag, buffer, size)                               \
  fread(buffer, 1, size, filename_tag)                                      \

#define FILE_WRITE(filename_tag, buffer, size)                              \
  fwrite(buffer, 1, size, filename_tag)                                     \

#define FILE_SEEK(filename_tag, offset, type)                               \
  fseek(filename_tag, offset, type)                                         \

#define FILE_TELL(filename_tag)                                             \
  ftell(filename_tag)                                                       \

#include "draw.h"
#include "gui.h"
#include "main.h"
#include "od-sound.h"
#include "od-input.h"
#include "imageio.h"
#include "unifont.h"

extern uint32_t BootFromBIOS;
extern uint32_t ShowFPS;
extern uint32_t UserFrameskip;

extern struct timespec TimeDifference(struct timespec Past, struct timespec Present);

#endif