#include "aura_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#include "stb_image.h"

aura_image_t* aura_image_load(const char *path) {
    if (!path) return NULL;

    int w, h, channels;
    /* Force 1-channel 8-bit grayscale */
    uint8_t *data = stbi_load(path, &w, &h, &channels, 1);
    if (!data) {
        /* Check in assets/covers/ with basename */
        const char *base = strrchr(path, '/');
        if (!base) base = strrchr(path, '\\');
        base = base ? base + 1 : path;
        char fallback[256];
        snprintf(fallback, sizeof(fallback), "assets/covers/%s", base);
        data = stbi_load(fallback, &w, &h, &channels, 1);
    }

    if (!data) {
        return NULL;
    }

    aura_image_t *img = (aura_image_t*)malloc(sizeof(aura_image_t));
    if (!img) {
        stbi_image_free(data);
        return NULL;
    }

    img->width = w;
    img->height = h;
    img->data = data;
    return img;
}

aura_image_t* aura_image_load_from_memory(const uint8_t *buffer, size_t len) {
    if (!buffer || len == 0) return NULL;

    int w, h, channels;
    uint8_t *data = stbi_load_from_memory(buffer, (int)len, &w, &h, &channels, 1);
    if (!data) return NULL;

    aura_image_t *img = (aura_image_t*)malloc(sizeof(aura_image_t));
    if (!img) {
        stbi_image_free(data);
        return NULL;
    }

    img->width = w;
    img->height = h;
    img->data = data;
    return img;
}

void aura_image_free(aura_image_t *img) {
    if (!img) return;
    if (img->data) {
        stbi_image_free(img->data);
        img->data = NULL;
    }
    free(img);
}

void aura_image_draw(aura_fb_t *fb, const aura_image_t *img, int dst_x, int dst_y) {
    if (!fb || !fb->back_buffer || !img || !img->data) return;

    for (int y = 0; y < img->height; y++) {
        int py = dst_y + y;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        for (int x = 0; x < img->width; x++) {
            int px = dst_x + x;
            if (px < 0 || px >= SCREEN_WIDTH) continue;

            fb->back_buffer[py * fb->line_length + px] = img->data[y * img->width + x];
        }
    }
}

/* Floyd-Steinberg 16-level grayscale dithering for Kindle E-Ink */
void aura_image_dither_fs(uint8_t *pixels, int w, int h) {
    if (!pixels || w <= 0 || h <= 0) return;

    /* Allocate buffer with signed error values */
    int *errors = (int*)calloc(w * h, sizeof(int));
    if (!errors) return;

    for (int i = 0; i < w * h; i++) {
        errors[i] = pixels[i];
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int old_val = errors[y * w + x];
            if (old_val < 0) old_val = 0;
            if (old_val > 255) old_val = 255;

            /* Quantize to 16 equal levels (0, 17, 34, ..., 255) */
            int new_val = ((old_val + 8) / 17) * 17;
            if (new_val > 255) new_val = 255;

            pixels[y * w + x] = (uint8_t)new_val;
            int err = old_val - new_val;

            /* Distribute error: right (7/16) */
            if (x + 1 < w) {
                errors[y * w + (x + 1)] += (err * 7) / 16;
            }
            /* bottom-left (3/16) */
            if (y + 1 < h && x - 1 >= 0) {
                errors[(y + 1) * w + (x - 1)] += (err * 3) / 16;
            }
            /* bottom (5/16) */
            if (y + 1 < h) {
                errors[(y + 1) * w + x] += (err * 5) / 16;
            }
            /* bottom-right (1/16) */
            if (y + 1 < h && x + 1 < w) {
                errors[(y + 1) * w + (x + 1)] += (err * 1) / 16;
            }
        }
    }

    free(errors);
}

/* Scaled rendering with box-filter averaging and dithering */
void aura_image_draw_scaled(aura_fb_t *fb, const aura_image_t *img, int dst_x, int dst_y, int dst_w, int dst_h) {
    if (!fb || !fb->back_buffer || !img || !img->data || dst_w <= 0 || dst_h <= 0) return;

    /* Temporary scaled buffer for dithering */
    uint8_t *scaled = (uint8_t*)malloc(dst_w * dst_h);
    if (!scaled) return;

    float x_ratio = (float)img->width / (float)dst_w;
    float y_ratio = (float)img->height / (float)dst_h;

    for (int y = 0; y < dst_h; y++) {
        int src_y_start = (int)(y * y_ratio);
        int src_y_end = (int)((y + 1) * y_ratio);
        if (src_y_end <= src_y_start) src_y_end = src_y_start + 1;
        if (src_y_end > img->height) src_y_end = img->height;

        for (int x = 0; x < dst_w; x++) {
            int src_x_start = (int)(x * x_ratio);
            int src_x_end = (int)((x + 1) * x_ratio);
            if (src_x_end <= src_x_start) src_x_end = src_x_start + 1;
            if (src_x_end > img->width) src_x_end = img->width;

            /* Box-filter area average */
            uint32_t sum = 0;
            int count = 0;
            for (int sy = src_y_start; sy < src_y_end; sy++) {
                for (int sx = src_x_start; sx < src_x_end; sx++) {
                    sum += img->data[sy * img->width + sx];
                    count++;
                }
            }

            scaled[y * dst_w + x] = (count > 0) ? (uint8_t)(sum / count) : 0xFF;
        }
    }

    /* Apply Floyd-Steinberg dithering for clean E-Ink 16-level presentation */
    aura_image_dither_fs(scaled, dst_w, dst_h);

    /* Blit onto framebuffer with clipping */
    for (int y = 0; y < dst_h; y++) {
        int py = dst_y + y;
        if (py < 0 || py >= SCREEN_HEIGHT) continue;

        for (int x = 0; x < dst_w; x++) {
            int px = dst_x + x;
            if (px < 0 || px >= SCREEN_WIDTH) continue;

            fb->back_buffer[py * fb->line_length + px] = scaled[y * dst_w + x];
        }
    }

    free(scaled);
}
