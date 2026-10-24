#include "aura_image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

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
        /* Check fallbacks for Kindle and local simulation */
        const char *base = strrchr(path, '/');
        if (!base) base = strrchr(path, '\\');
        base = base ? base + 1 : path;

        const char *search_dirs[] = {
            "assets/covers",
            "covers",
            "/mnt/us/aurabooks/covers",
            "/mnt/us/covers",
            "/mnt/us/koreader/cache/covers/large",
            NULL
        };

        for (int i = 0; search_dirs[i] != NULL; i++) {
            char fallback[256];
            snprintf(fallback, sizeof(fallback), "%s/%s", search_dirs[i], base);
            data = stbi_load(fallback, &w, &h, &channels, 1);
            if (data) break;
        }

        /* Fuzzy search: search directory for any file containing the base book title */
        if (!data && strlen(base) >= 4) {
            char search_token[64];
            snprintf(search_token, sizeof(search_token), "%s", base);
            char *dot = strrchr(search_token, '.');
            if (dot) *dot = '\0';
            if (strlen(search_token) > 16) search_token[16] = '\0';

            for (int i = 0; search_dirs[i] != NULL && !data; i++) {
                DIR *dir = opendir(search_dirs[i]);
                if (!dir) continue;
                struct dirent *ent;
                while ((ent = readdir(dir)) != NULL) {
                    if (ent->d_name[0] == '.') continue;
                    if (strcasestr(ent->d_name, search_token)) {
                        char full_p[256];
                        snprintf(full_p, sizeof(full_p), "%s/%s", search_dirs[i], ent->d_name);
                        data = stbi_load(full_p, &w, &h, &channels, 1);
                        if (data) break;
                    }
                }
                closedir(dir);
            }
        }
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

void aura_image_draw(aura_fb_t *fb, const aura_image_t *img, int x, int y) {
    if (!fb || !img || !img->data) return;

    for (int r = 0; r < img->height; r++) {
        int dst_y = y + r;
        if (dst_y < 0 || dst_y >= SCREEN_HEIGHT) continue;

        for (int c = 0; c < img->width; c++) {
            int dst_x = x + c;
            if (dst_x < 0 || dst_x >= SCREEN_WIDTH) continue;

            uint8_t gray = img->data[r * img->width + c];
            aura_fb_draw_pixel(fb, dst_x, dst_y, gray);
        }
    }
}

void aura_image_draw_scaled(aura_fb_t *fb, const aura_image_t *img, int x, int y, int target_w, int target_h) {
    if (!fb || !img || !img->data || target_w <= 0 || target_h <= 0) return;

    /* Bilinear/Nearest neighbor scale with 16-level E-Ink quantization */
    for (int dy = 0; dy < target_h; dy++) {
        int dst_y = y + dy;
        if (dst_y < 0 || dst_y >= SCREEN_HEIGHT) continue;

        int src_y = (dy * img->height) / target_h;
        if (src_y >= img->height) src_y = img->height - 1;

        for (int dx = 0; dx < target_w; dx++) {
            int dst_x = x + dx;
            if (dst_x < 0 || dst_x >= SCREEN_WIDTH) continue;

            int src_x = (dx * img->width) / target_w;
            if (src_x >= img->width) src_x = img->width - 1;

            uint8_t raw = img->data[src_y * img->width + src_x];
            /* Quantize to 16 E-Ink Pearl levels (0, 17, 34 ... 255) */
            uint8_t quantized = (raw / 17) * 17;
            aura_fb_draw_pixel(fb, dst_x, dst_y, quantized);
        }
    }
}

void aura_image_dither_floyd_steinberg(aura_fb_t *fb, const aura_image_t *img, int x, int y, int target_w, int target_h) {
    if (!fb || !img || !img->data || target_w <= 0 || target_h <= 0) return;

    /* Floyd-Steinberg error diffusion buffer */
    int *errors = (int*)calloc(target_w * target_h, sizeof(int));
    if (!errors) {
        aura_image_draw_scaled(fb, img, x, y, target_w, target_h);
        return;
    }

    /* Initialize errors with scaled image luminance */
    for (int dy = 0; dy < target_h; dy++) {
        int src_y = (dy * img->height) / target_h;
        for (int dx = 0; dx < target_w; dx++) {
            int src_x = (dx * img->width) / target_w;
            errors[dy * target_w + dx] = (int)img->data[src_y * img->width + src_x];
        }
    }

    /* Diffusion pass */
    for (int dy = 0; dy < target_h; dy++) {
        int dst_y = y + dy;
        if (dst_y < 0 || dst_y >= SCREEN_HEIGHT) continue;

        for (int dx = 0; dx < target_w; dx++) {
            int dst_x = x + dx;
            if (dst_x < 0 || dst_x >= SCREEN_WIDTH) continue;

            int old_px = errors[dy * target_w + dx];
            if (old_px < 0) old_px = 0;
            if (old_px > 255) old_px = 255;

            /* Quantize to nearest 16-level step */
            int new_px = ((old_px + 8) / 17) * 17;
            if (new_px > 255) new_px = 255;

            aura_fb_draw_pixel(fb, dst_x, dst_y, (uint8_t)new_px);

            int err = old_px - new_px;

            /* Distribute error to neighboring pixels */
            if (dx + 1 < target_w) {
                errors[dy * target_w + (dx + 1)] += (err * 7) / 16;
            }
            if (dy + 1 < target_h) {
                if (dx - 1 >= 0) {
                    errors[(dy + 1) * target_w + (dx - 1)] += (err * 3) / 16;
                }
                errors[(dy + 1) * target_w + dx] += (err * 5) / 16;
                if (dx + 1 < target_w) {
                    errors[(dy + 1) * target_w + (dx + 1)] += (err * 1) / 16;
                }
            }
        }
    }

    free(errors);
}
