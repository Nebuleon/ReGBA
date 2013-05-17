/* Tuning parameters for the Supercard DSTwo version of gpSP */
/* Its processor is an Ingenic JZ4740 at 360 MHz with 32 MiB of RAM */
#define CACHE_LINE_SIZE                   32
#define READONLY_CODE_CACHE_SIZE          (1152 * 1024)
#define TRANSLATION_CACHE_LIMIT_THRESHOLD (1024)
#define WRITABLE_CODE_CACHE_SIZE          (1024 * 1024 * 2)
