#ifndef BX_KIT_H
#define BX_KIT_H

#include "../bitexact.h"

#ifdef BX_KIT

#ifdef __cplusplus
extern "C" {
#endif

void kit_open(const char *dir);
void kit_layout(const char *entry, const char *lines);
void kit_begin(const char *entry, int edge);
void kit_i32(int value);
void kit_u32(bx_u32 value);
void kit_f64(double value);
void kit_str(const char *s);
void kit_bytes(const void *data, size_t size);
void kit_dropped(const char *entry);
void kit_end(void);
void kit_close(void);

#ifdef __cplusplus
}
#endif

#else

#define kit_open(dir) ((void)0)
#define kit_layout(entry, lines) ((void)0)
#define kit_begin(entry, edge) ((void)0)
#define kit_i32(value) ((void)0)
#define kit_u32(value) ((void)0)
#define kit_f64(value) ((void)0)
#define kit_str(s) ((void)0)
#define kit_bytes(data, size) ((void)0)
#define kit_dropped(entry) ((void)0)
#define kit_end() ((void)0)
#define kit_close() ((void)0)

#endif

#endif
