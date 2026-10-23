#ifndef AURA_IMAGE_H
#define AURA_IMAGE_H

#include "aura_fb.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int width;
    int height;
    uint8_t *data; /* 8-bit grayscale pixels (0x00 = black, 0xFF = white) */
} aura_image_t;

/* Lifecycle */
aura_image_t* aura_image_load(const char *path);
aura_image_t* aura_image_load_from_memory(const uint8_t *buffer, size_t len);
void          aura_image_free(aura_image_t *img);

/* Rendering primitives */
void aura_image_draw(aura_fb_t *fb, const aura_image_t *img, int dst_x, int dst_y);

/* Scaled rendering with high-quality box filter downsampling and Floyd-Steinberg dithering */
void aura_image_draw_scaled(aura_fb_t *fb, const aura_image_t *img, int dst_x, int dst_y, int dst_w, int dst_h);

/* Fast 16-level grayscale quantizer with Floyd-Steinberg error diffusion for E-Ink */
void aura_image_dither_fs(uint8_t *pixels, int w, int h);

#ifdef __cplusplus
}
#endif

#endif /* AURA_IMAGE_H */
