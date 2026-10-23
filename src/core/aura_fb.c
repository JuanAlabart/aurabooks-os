#include "aura_fb.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

/* Freescale / NXP i.MX508 EPDC ioctl definitions for Kindle */
#define MXCFB_SET_WAVEFORM_MODES	_IOW('F', 0x2B, struct mxcfb_waveform_modes)
#define MXCFB_SET_TEMPERATURE		_IOW('F', 0x2C, int32_t)
#define MXCFB_SET_AUTO_UPDATE_MODE	_IOW('F', 0x2D, uint32_t)
#define MXCFB_SEND_UPDATE		_IOW('F', 0x2E, struct mxcfb_update_data)
#define MXCFB_WAIT_FOR_UPDATE_COMPLETE	_IOWR('F', 0x2F, struct mxcfb_update_marker_data)

struct mxcfb_rect {
    uint32_t top;
    uint32_t left;
    uint32_t width;
    uint32_t height;
};

struct mxcfb_update_data {
    struct mxcfb_rect update_region;
    uint32_t waveform_mode;
    uint32_t update_mode;
    uint32_t update_marker;
    int temp;
    uint32_t flags;
    void *dither_mode;
    void *quant_mode;
};

/* Waveform mode constants on Kindle Touch */
#define WAVEFORM_MODE_INIT	0x0
#define WAVEFORM_MODE_DU	0x1   /* Direct Update (fast B/W) */
#define WAVEFORM_MODE_GC16	0x2   /* Grayscale Clear 16 (Full clean flash) */
#define WAVEFORM_MODE_GC4	0x3
#define WAVEFORM_MODE_A2	0x4   /* Fast 2-level animated */
#define WAVEFORM_MODE_GL16	0x5   /* Fast Grayscale partial (E-Reader text) */
#define WAVEFORM_MODE_AUTO	0x20

#define UPDATE_MODE_PARTIAL	0x0
#define UPDATE_MODE_FULL	0x1

aura_fb_t* aura_fb_init(const char *fb_path) {
    if (!fb_path) {
        fb_path = "/dev/fb0";
    }

    aura_fb_t *fb = (aura_fb_t*)calloc(1, sizeof(aura_fb_t));
    if (!fb) return NULL;

    fb->fd = open(fb_path, O_RDWR);
    if (fb->fd < 0) {
        /* Simulator / headless fallback mode */
        fprintf(stderr, "[aura_fb] Warning: Cannot open %s, running in virtual buffer mode.\n", fb_path);
        fb->is_simulated = true;
        fb->line_length = SCREEN_WIDTH;
        fb->bpp = 8;
        fb->buffer_size = SCREEN_WIDTH * SCREEN_HEIGHT;
        fb->back_buffer = (uint8_t*)malloc(fb->buffer_size);
        memset(fb->back_buffer, 0xFF, fb->buffer_size);
        return fb;
    }

    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) < 0 ||
        ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
        perror("[aura_fb] Error querying framebuffer info");
        close(fb->fd);
        free(fb);
        return NULL;
    }

    fb->bpp = vinfo.bits_per_pixel;
    fb->line_length = finfo.line_length;
    fb->buffer_size = finfo.smem_len;

    fb->fb_mem = (uint8_t*)mmap(NULL, fb->buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
    if (fb->fb_mem == MAP_FAILED) {
        perror("[aura_fb] Error mmapping framebuffer");
        close(fb->fd);
        free(fb);
        return NULL;
    }

    fb->back_buffer = (uint8_t*)malloc(fb->buffer_size);
    if (!fb->back_buffer) {
        munmap(fb->fb_mem, fb->buffer_size);
        close(fb->fd);
        free(fb);
        return NULL;
    }

    /* Initialize back buffer to clean white */
    memset(fb->back_buffer, 0xFF, fb->buffer_size);
    memcpy(fb->fb_mem, fb->back_buffer, fb->buffer_size);

    return fb;
}

void aura_fb_close(aura_fb_t *fb) {
    if (!fb) return;

    if (fb->back_buffer) {
        free(fb->back_buffer);
        fb->back_buffer = NULL;
    }

    if (!fb->is_simulated && fb->fb_mem) {
        munmap(fb->fb_mem, fb->buffer_size);
        fb->fb_mem = NULL;
    }

    if (!fb->is_simulated && fb->fd >= 0) {
        close(fb->fd);
        fb->fd = -1;
    }

    free(fb);
}

void aura_fb_clear(aura_fb_t *fb, uint8_t gray_color) {
    if (!fb || !fb->back_buffer) return;
    memset(fb->back_buffer, gray_color, fb->buffer_size);
}

void aura_fb_draw_pixel(aura_fb_t *fb, int x, int y, uint8_t gray_color) {
    if (!fb || !fb->back_buffer) return;
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;

    fb->back_buffer[y * fb->line_length + x] = gray_color;
}

void aura_fb_fill_rect(aura_fb_t *fb, int x, int y, int w, int h, uint8_t gray_color) {
    if (!fb || !fb->back_buffer) return;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    for (int row = y; row < y + h; row++) {
        memset(&fb->back_buffer[row * fb->line_length + x], gray_color, w);
    }
}

void aura_fb_draw_rect(aura_fb_t *fb, int x, int y, int w, int h, int border_width, uint8_t gray_color) {
    if (border_width <= 0) return;

    /* Top border */
    aura_fb_fill_rect(fb, x, y, w, border_width, gray_color);
    /* Bottom border */
    aura_fb_fill_rect(fb, x, y + h - border_width, w, border_width, gray_color);
    /* Left border */
    aura_fb_fill_rect(fb, x, y + border_width, border_width, h - (2 * border_width), gray_color);
    /* Right border */
    aura_fb_fill_rect(fb, x + w - border_width, y + border_width, border_width, h - (2 * border_width), gray_color);
}

void aura_fb_draw_rounded_rect(aura_fb_t *fb, int x, int y, int w, int h, int radius, int border_width, uint8_t fill_color, uint8_t border_color) {
    if (radius <= 0) {
        if (fill_color != 0xFF) aura_fb_fill_rect(fb, x, y, w, h, fill_color);
        if (border_width > 0) aura_fb_draw_rect(fb, x, y, w, h, border_width, border_color);
        return;
    }

    /* Fill interior body */
    if (fill_color != 0xFF) {
        aura_fb_fill_rect(fb, x + radius, y, w - 2 * radius, h, fill_color);
        aura_fb_fill_rect(fb, x, y + radius, radius, h - 2 * radius, fill_color);
        aura_fb_fill_rect(fb, x + w - radius, y + radius, radius, h - 2 * radius, fill_color);
    }

    /* Draw straight borders */
    if (border_width > 0) {
        aura_fb_fill_rect(fb, x + radius, y, w - 2 * radius, border_width, border_color);
        aura_fb_fill_rect(fb, x + radius, y + h - border_width, w - 2 * radius, border_width, border_color);
        aura_fb_fill_rect(fb, x, y + radius, border_width, h - 2 * radius, border_color);
        aura_fb_fill_rect(fb, x + w - border_width, y + radius, border_width, h - 2 * radius, border_color);
    }

    /* Round corner rasterization (Bresenham circle algorithm) */
    int cx[4] = { x + radius, x + w - radius - 1, x + radius, x + w - radius - 1 };
    int cy[4] = { y + radius, y + radius, y + h - radius - 1, y + h - radius - 1 };

    for (int dy = 0; dy <= radius; dy++) {
        for (int dx = 0; dx <= radius; dx++) {
            int dist_sq = dx * dx + dy * dy;
            int r_outer_sq = radius * radius;
            int r_inner_sq = (radius - border_width) * (radius - border_width);

            if (dist_sq <= r_outer_sq) {
                uint8_t col = (dist_sq >= r_inner_sq && border_width > 0) ? border_color : fill_color;
                
                /* Top-Left */
                aura_fb_draw_pixel(fb, cx[0] - dx, cy[0] - dy, col);
                /* Top-Right */
                aura_fb_draw_pixel(fb, cx[1] + dx, cy[1] - dy, col);
                /* Bottom-Left */
                aura_fb_draw_pixel(fb, cx[2] - dx, cy[2] + dy, col);
                /* Bottom-Right */
                aura_fb_draw_pixel(fb, cx[3] + dx, cy[3] + dy, col);
            }
        }
    }
}

void aura_fb_flush_rect(aura_fb_t *fb, aura_rect_t rect, aura_refresh_mode_t mode) {
    if (!fb || fb->is_simulated || !fb->fb_mem || !fb->back_buffer) return;

    /* Clamp rect */
    if (rect.x < 0) { rect.w += rect.x; rect.x = 0; }
    if (rect.y < 0) { rect.h += rect.y; rect.y = 0; }
    if (rect.x + rect.w > SCREEN_WIDTH) rect.w = SCREEN_WIDTH - rect.x;
    if (rect.y + rect.h > SCREEN_HEIGHT) rect.h = SCREEN_HEIGHT - rect.y;
    if (rect.w <= 0 || rect.h <= 0) return;

    /* Copy modified region from back buffer to mapped hardware framebuffer */
    for (int row = rect.y; row < rect.y + rect.h; row++) {
        size_t offset = row * fb->line_length + rect.x;
        memcpy(&fb->fb_mem[offset], &fb->back_buffer[offset], rect.w);
    }

    /* Send Freescale EPDC hardware update ioctl */
    struct mxcfb_update_data update_data;
    memset(&update_data, 0, sizeof(update_data));

    update_data.update_region.left = rect.x;
    update_data.update_region.top = rect.y;
    update_data.update_region.width = rect.w;
    update_data.update_region.height = rect.h;

    if (mode == AURA_REFRESH_FULL) {
        update_data.waveform_mode = WAVEFORM_MODE_GC16;
        update_data.update_mode = UPDATE_MODE_FULL;
    } else if (mode == AURA_REFRESH_DIRECT) {
        update_data.waveform_mode = WAVEFORM_MODE_DU;
        update_data.update_mode = UPDATE_MODE_PARTIAL;
    } else {
        update_data.waveform_mode = WAVEFORM_MODE_GL16;
        update_data.update_mode = UPDATE_MODE_PARTIAL;
    }

    update_data.temp = 24; /* Ambient sensor fallback */
    update_data.flags = 0;

    ioctl(fb->fd, MXCFB_SEND_UPDATE, &update_data);
}

void aura_fb_flush_full(aura_fb_t *fb, aura_refresh_mode_t mode) {
    aura_rect_t full_screen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    aura_fb_flush_rect(fb, full_screen, mode);
}
