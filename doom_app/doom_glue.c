/* Klebstoff: WAD direkt aus der Flash-Partition "wad" (mmap, keine SD noetig)
   und sauberer Ruecksprung zu Bruce statt exit(). Greift per --wrap ein. */
#include <string.h>
#include <strings.h>
#include <stdint.h>
#include "esp_partition.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "doomtype.h"
#include "w_file.h"

extern boolean __real_M_FileExists(const char *filename);
extern wad_file_t *__real_W_OpenFile(char *path);

static wad_file_t g_wad;
static wad_file_class_t g_class;
static const uint8_t *g_base;
static uint32_t g_len;

static int is_iwad(const char *p)
{
    size_t n = p ? strlen(p) : 0;
    return n >= 9 && strcasecmp(p + n - 9, "doom1.wad") == 0;
}

boolean __wrap_M_FileExists(const char *filename)
{
    if (is_iwad(filename)) return (boolean)1;
    return __real_M_FileExists(filename);
}

static void flash_close(wad_file_t *f) { (void)f; }

static size_t flash_read(wad_file_t *f, unsigned int off, void *buf, size_t len)
{
    if (off >= f->length) return 0;
    if (off + len > f->length) len = f->length - off;
    memcpy(buf, g_base + off, len);
    return len;
}

wad_file_t *__wrap_W_OpenFile(char *path)
{
    if (!is_iwad(path)) return __real_W_OpenFile(path);

    if (!g_base) {
        const esp_partition_t *p =
            esp_partition_find_first(ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x40, "wad");
        if (!p) return NULL;
        const void *ptr = NULL;
        uint32_t handle = 0;
        if (esp_partition_mmap(p, 0, p->size, ESP_PARTITION_MMAP_DATA, &ptr, &handle) != ESP_OK)
            return NULL;
        const uint8_t *b = (const uint8_t *)ptr;
        if (memcmp(b + 1, "WAD", 3) != 0) return NULL;          /* kein WAD in der Partition */
        uint32_t nlumps, tableofs;
        memcpy(&nlumps, b + 4, 4);
        memcpy(&tableofs, b + 8, 4);
        g_len = tableofs + nlumps * 16;
        if (g_len > p->size) return NULL;
        g_base = b;
    }
    g_class.OpenFile = NULL;
    g_class.CloseFile = flash_close;
    g_class.Read = flash_read;
    g_wad.file_class = &g_class;
    g_wad.mapped = (byte *)g_base;       /* Zero-Copy: Lumps zeigen direkt in den Flash */
    g_wad.length = g_len;
    g_wad.path = (char *)"doom1.wad";
    return &g_wad;
}

/* Doom beendet sich (Menue "Quit" oder I_Error) -> zurueck zu Bruce (ota_0) */
void __wrap_exit(int code)
{
    (void)code;
    const esp_partition_t *p =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (p) esp_ota_set_boot_partition(p);
    esp_restart();
    for (;;) { }
}
