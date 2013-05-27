/* Tuning parameters for the Supercard DSTwo version of gpSP */
/* Its processor is an Ingenic JZ4740 at 360 MHz with 32 MiB of RAM */
#define CACHE_LINE_SIZE                   32
#define IWRAM_TRANSLATION_CACHE_SIZE      (1024 * 128)
#define EWRAM_TRANSLATION_CACHE_SIZE      (1024 * 128)
#define VRAM_TRANSLATION_CACHE_SIZE       (1024 * 128)
#define ROM_TRANSLATION_CACHE_SIZE        (1024 * 1152)
#define BIOS_TRANSLATION_CACHE_SIZE       (1024 * 96)
#define PERSISTENT_TRANSLATION_CACHE_SIZE (1024 * 768)
#define TRANSLATION_CACHE_LIMIT_THRESHOLD (1024)
