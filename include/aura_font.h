#ifndef AURA_FONT_H
#define AURA_FONT_H

#include "aura_fb.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AURA_ALIGN_LEFT = 0,
    AURA_ALIGN_CENTER,
    AURA_ALIGN_RIGHT
} aura_text_align_t;

typedef struct aura_font aura_font_t;

/* Lifecycle */
aura_font_t* aura_font_load(const char *font_path, int point_size);
aura_font_t* aura_font_default(void); /* Embedded serif font, zero dependencies */
void         aura_font_close(aura_font_t *font);

/* Measurements */
int  aura_font_get_height(const aura_font_t *font);
int  aura_font_measure_string(const aura_font_t *font, const char *text);

/* Drawing functions */
void aura_font_draw_string(aura_fb_t *fb, const aura_font_t *font, const char *text, int x, int y, uint8_t color);
void aura_font_draw_aligned(aura_fb_t *fb, const aura_font_t *font, const char *text, int x, int y, int w, aura_text_align_t align, uint8_t color);
int  aura_font_draw_wrapped(aura_fb_t *fb, const aura_font_t *font, const char *text, int x, int y, int max_w, int line_spacing, uint8_t color);

#ifdef __cplusplus
}
#endif

#endif /* AURA_FONT_H */
