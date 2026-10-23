#include "aura_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* PalmDOC / MOBI Big-Endian 16-bit and 32-bit helpers */
static inline uint16_t read_be16(const uint8_t *p) {
    return (uint16_t)((p[0] << 8) | p[1]);
}

static inline uint32_t read_be32(const uint8_t *p) {
    return (uint32_t)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
}

/* PalmDOC LZ77 Decompression in C */
static size_t decompress_palmdoc(const uint8_t *src, size_t src_len, uint8_t *dst, size_t dst_max) {
    size_t s = 0;
    size_t d = 0;

    while (s < src_len && d < dst_max) {
        uint8_t byte = src[s++];

        if (byte == 0x00) {
            dst[d++] = 0x00;
        } else if (byte >= 0x01 && byte <= 0x08) {
            /* Copy next 'byte' literal bytes */
            while (byte-- > 0 && s < src_len && d < dst_max) {
                dst[d++] = src[s++];
            }
        } else if (byte <= 0x7F) {
            /* Single literal byte */
            dst[d++] = byte;
        } else if (byte >= 0xC0) {
            /* Byte literal followed by space */
            dst[d++] = byte ^ 0x80;
            if (d < dst_max) dst[d++] = ' ';
        } else {
            /* 0x80 to 0xBF: Distance & Length pair */
            if (s >= src_len) break;
            uint8_t next = src[s++];
            int distance = ((byte & 0x3F) << 3) | (next >> 5);
            int length = (next & 0x1F) + 3;

            if (distance > 0 && distance <= (int)d) {
                size_t copy_src = d - distance;
                while (length-- > 0 && d < dst_max) {
                    dst[d++] = dst[copy_src++];
                }
            }
        }
    }

    return d;
}

aura_reader_doc_t* aura_reader_open(const char *file_path) {
    if (!file_path) return NULL;

    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        fprintf(stderr, "[aura_reader] Unable to open %s\n", file_path);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size < 78) {
        fclose(fp);
        return NULL;
    }

    uint8_t *file_data = (uint8_t*)malloc(file_size);
    if (!file_data) {
        fclose(fp);
        return NULL;
    }

    if (fread(file_data, 1, file_size, fp) != (size_t)file_size) {
        free(file_data);
        fclose(fp);
        return NULL;
    }
    fclose(fp);

    aura_reader_doc_t *doc = (aura_reader_doc_t*)calloc(1, sizeof(aura_reader_doc_t));
    snprintf(doc->file_path, sizeof(doc->file_path), "%s", file_path);

    /* Check if file has PalmDOC / MOBI header */
    uint16_t num_records = read_be16(&file_data[76]);
    bool is_mobi = (file_size > 100 && memcmp(&file_data[60], "BOOKMOBI", 8) == 0);

    if (is_mobi && num_records > 1) {
        /* PalmDOC compression type is at record 0 + 0 */
        uint32_t rec0_offset = read_be32(&file_data[78]);
        uint16_t compression = read_be16(&file_data[rec0_offset]);
        uint16_t text_record_count = read_be16(&file_data[rec0_offset + 8]);

        /* Allocate text buffer (average 4KB per record) */
        size_t est_text_size = text_record_count * 4096 + 1024;
        doc->raw_text = (char*)malloc(est_text_size);
        size_t total_decompressed = 0;

        for (uint16_t i = 1; i <= text_record_count && i < num_records; i++) {
            uint32_t rec_offset = read_be32(&file_data[78 + i * 8]);
            uint32_t next_offset = (i + 1 < num_records) ? read_be32(&file_data[78 + (i + 1) * 8]) : (uint32_t)file_size;
            size_t rec_len = next_offset - rec_offset;

            if (rec_offset + rec_len <= (size_t)file_size) {
                if (compression == 2) {
                    /* PalmDOC LZ77 */
                    size_t out_len = decompress_palmdoc(
                        &file_data[rec_offset], rec_len,
                        (uint8_t*)&doc->raw_text[total_decompressed], est_text_size - total_decompressed
                    );
                    total_decompressed += out_len;
                } else {
                    /* Uncompressed */
                    memcpy(&doc->raw_text[total_decompressed], &file_data[rec_offset], rec_len);
                    total_decompressed += rec_len;
                }
            }
        }

        doc->raw_text[total_decompressed] = '\0';
        doc->raw_text_len = total_decompressed;
    } else {
        /* Plain text / UTF-8 fallback */
        doc->raw_text = (char*)file_data;
        doc->raw_text_len = file_size;
        file_data = NULL; /* Transfer ownership */
    }

    if (file_data) free(file_data);

    /* Extract basic title from filename */
    const char *base = strrchr(file_path, '/');
    if (!base) base = strrchr(file_path, '\\');
    base = base ? base + 1 : file_path;
    snprintf(doc->title, sizeof(doc->title), "%s", base);

    doc->current_page = 1;
    return doc;
}

void aura_reader_close(aura_reader_doc_t *doc) {
    if (!doc) return;
    if (doc->raw_text) free(doc->raw_text);
    free(doc);
}

void aura_reader_paginate(aura_reader_doc_t *doc, const aura_font_t *font, int margin_x, int margin_y, int content_w, int content_h) {
    if (!doc || !font || !doc->raw_text || doc->raw_text_len == 0) return;

    (void)margin_x; (void)margin_y;
    int line_h = aura_font_get_height(font) + 4;
    int max_lines_per_page = content_h / line_h;
    if (max_lines_per_page <= 0) max_lines_per_page = 20;

    int char_w = 8; /* Font scale 1 */
    int chars_per_line = content_w / char_w;
    if (chars_per_line <= 0) chars_per_line = 40;

    doc->total_pages = 0;
    doc->page_offsets[doc->total_pages++] = 0;

    const char *text = doc->raw_text;
    size_t len = doc->raw_text_len;
    size_t pos = 0;

    while (pos < len && doc->total_pages < MAX_PAGES_CACHE) {
        int lines = 0;

        while (pos < len && lines < max_lines_per_page) {
            /* Find end of line */
            int line_chars = 0;
            while (pos < len && line_chars < chars_per_line && text[pos] != '\n') {
                pos++;
                line_chars++;
            }

            if (pos < len && text[pos] == '\n') {
                pos++; /* Skip newline */
            }
            lines++;
        }

        if (pos < len && doc->total_pages < MAX_PAGES_CACHE) {
            doc->page_offsets[doc->total_pages++] = (uint32_t)pos;
        }
    }

    if (doc->total_pages == 0) doc->total_pages = 1;
    doc->current_page = 1;
}

bool aura_reader_next_page(aura_reader_doc_t *doc) {
    if (!doc || doc->current_page >= doc->total_pages) return false;
    doc->current_page++;
    return true;
}

bool aura_reader_prev_page(aura_reader_doc_t *doc) {
    if (!doc || doc->current_page <= 1) return false;
    doc->current_page--;
    return true;
}

bool aura_reader_goto_page(aura_reader_doc_t *doc, int page_num) {
    if (!doc || page_num < 1 || page_num > doc->total_pages) return false;
    doc->current_page = page_num;
    return true;
}

void aura_reader_render_page(aura_fb_t *fb, const aura_font_t *font, aura_reader_doc_t *doc) {
    if (!fb || !font || !doc || !doc->raw_text) return;

    aura_fb_clear(fb, 0xFF); /* Clear screen to paper white */

    int margin_x = 32;
    int margin_y = 28;
    int content_w = SCREEN_WIDTH - (2 * margin_x);
    int content_h = SCREEN_HEIGHT - (2 * margin_y) - 30; /* 30px footer budget */
    int line_h = aura_font_get_height(font) + 4;

    uint32_t start_offset = doc->page_offsets[doc->current_page - 1];
    uint32_t end_offset = (doc->current_page < doc->total_pages) ? doc->page_offsets[doc->current_page] : (uint32_t)doc->raw_text_len;

    /* Render text lines */
    int cur_y = margin_y;
    const char *text = &doc->raw_text[start_offset];
    size_t length = end_offset - start_offset;
    size_t pos = 0;
    int char_w = 8;
    int chars_per_line = content_w / char_w;

    char line_buf[128];

    while (pos < length && cur_y + line_h <= margin_y + content_h) {
        int line_chars = 0;
        while (pos < length && line_chars < chars_per_line && line_chars < (int)sizeof(line_buf) - 1 && text[pos] != '\n') {
            line_buf[line_chars++] = text[pos++];
        }
        line_buf[line_chars] = '\0';
        if (pos < length && text[pos] == '\n') pos++;

        aura_font_draw_string(fb, font, line_buf, margin_x, cur_y, 0x00);
        cur_y += line_h;
    }

    /* Render Bottom Reader Status Line */
    int footer_y = SCREEN_HEIGHT - 24;
    char footer_str[64];
    int pct = (doc->total_pages > 0) ? (doc->current_page * 100 / doc->total_pages) : 0;
    snprintf(footer_str, sizeof(footer_str), "%s  •  Pag. %d de %d (%d%%)", doc->title, doc->current_page, doc->total_pages, pct);
    aura_font_draw_aligned(fb, font, footer_str, 0, footer_y, SCREEN_WIDTH, AURA_ALIGN_CENTER, 0x66);
}
