#ifndef AURA_FB_H
#define AURA_FB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH   600
#define SCREEN_HEIGHT  800

typedef enum {
    AURA_REFRESH_FAST,    /* GL16 / DU: Fast partial refresh for UI / reading */
    AURA_REFRESH_FULL,    /* GC16: Flashing waveform to clear ghosting */
    AURA_REFRESH_DIRECT   /* 1-bit black/white for typing/sliders */
} aura_refresh_mode_t;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} aura_rect_t;

typedef struct {
    int fd;
    uint8_t *fb_mem;        /* Hardware mapped mmap buffer */
    uint8_t *back_buffer;   /* Working draw buffer in RAM */
    size_t buffer_size;
    uint32_t line_length;
    uint32_t bpp;
    bool is_simulated;
} aura_fb_t;

/* Lifecycle */
aura_fb_t* aura_fb_init(const char *fb_path);
void       aura_fb_close(aura_fb_t *fb);

/* Drawing primitives (in RAM buffer) */
void aura_fb_clear(aura_fb_t *fb, uint8_t gray_color);
void aura_fb_draw_pixel(aura_fb_t *fb, int x, int y, uint8_t gray_color);
void aura_fb_fill_rect(aura_fb_t *fb, int x, int y, int w, int h, uint8_t gray_color);
void aura_fb_draw_rect(aura_fb_t *fb, int x, int y, int w, int h, int border_width, uint8_t gray_color);
void aura_fb_draw_rounded_rect(aura_fb_t *fb, int x, int y, int w, int h, int radius, int border_width, uint8_t fill_color, uint8_t border_color);

/* Hardware E-Ink Refresh Triggers */
void aura_fb_flush_rect(aura_fb_t *fb, aura_rect_t rect, aura_refresh_mode_t mode);
void aura_fb_flush_full(aura_fb_t *fb, aura_refresh_mode_t mode);

#ifdef __cplusplus
}
#endif

#endif /* AURA_FB_H */
