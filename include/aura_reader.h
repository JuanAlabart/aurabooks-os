#ifndef AURA_READER_H
#define AURA_READER_H

#include "aura_fb.h"
#include "aura_font.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PAGES_CACHE   2048

typedef struct {
    char file_path[256];
    char title[128];
    char author[128];
    char *raw_text;
    size_t raw_text_len;

    /* Paging index */
    uint32_t page_offsets[MAX_PAGES_CACHE];
    int total_pages;
    int current_page; /* 1-based index */
} aura_reader_doc_t;

/* Lifecycle */
aura_reader_doc_t* aura_reader_open(const char *file_path);
void               aura_reader_close(aura_reader_doc_t *doc);

/* Pagination */
void aura_reader_paginate(aura_reader_doc_t *doc, const aura_font_t *font, int margin_x, int margin_y, int content_w, int content_h);
bool aura_reader_next_page(aura_reader_doc_t *doc);
bool aura_reader_prev_page(aura_reader_doc_t *doc);
bool aura_reader_goto_page(aura_reader_doc_t *doc, int page_num);

/* Render page to framebuffer */
void aura_reader_render_page(aura_fb_t *fb, const aura_font_t *font, aura_reader_doc_t *doc, bool show_hud);

#ifdef __cplusplus
}
#endif

#endif /* AURA_READER_H */
