#include "aura_ui.h"

#include <stdio.h>
#include <string.h>

static bool is_point_in_rect(int px, int py, aura_rect_t r) {
    return (px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h);
}

void aura_ui_draw_header(aura_fb_t *fb, const aura_font_t *font, const char *title) {
    if (!fb || !font) return;

    int y = 14;

    /* Draw Header Title */
    aura_font_draw_string(fb, font, title, 20, y + 6, 0x00);

    /* Draw Settings pill button on top right: 96x32 at x = 488 */
    aura_fb_draw_rounded_rect(fb, 488, y, 96, 32, 10, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font, "Ajustes", 488, y + 8, 96, AURA_ALIGN_CENTER, 0x00);
}

void aura_ui_draw_navbar(aura_fb_t *fb, const aura_font_t *font, aura_ui_navbar_t *navbar) {
    if (!fb || !navbar) return;

    int pill_w = 368;
    int pill_h = 52;
    int pill_x = (SCREEN_WIDTH - pill_w) / 2; /* 116px */
    int pill_y = 732;

    /* Background pill */
    aura_fb_draw_rounded_rect(fb, pill_x, pill_y, pill_w, pill_h, 22, 1, 0xFF, 0x00);

    int btn_w = 114;
    int btn_h = 42;
    int btn_y = pill_y + 5;
    int gap = 8;

    navbar->btn_home.bounds = (aura_rect_t){ pill_x + 5, btn_y, btn_w, btn_h };
    navbar->btn_library.bounds = (aura_rect_t){ pill_x + 5 + btn_w + gap, btn_y, btn_w, btn_h };
    navbar->btn_quotes.bounds = (aura_rect_t){ pill_x + 5 + 2 * (btn_w + gap), btn_y, btn_w, btn_h };

    /* 1. Home Button */
    bool is_home = (navbar->active_tab == AURA_TAB_HOME);
    aura_fb_draw_rounded_rect(fb, navbar->btn_home.bounds.x, btn_y, btn_w, btn_h, 16, is_home ? 2 : 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font, is_home ? "* Inicio" : "Inicio", navbar->btn_home.bounds.x, btn_y + 13, btn_w, AURA_ALIGN_CENTER, 0x00);

    /* 2. Library Button */
    bool is_lib = (navbar->active_tab == AURA_TAB_LIBRARY);
    aura_fb_draw_rounded_rect(fb, navbar->btn_library.bounds.x, btn_y, btn_w, btn_h, 16, is_lib ? 2 : 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font, is_lib ? "* Biblioteca" : "Biblioteca", navbar->btn_library.bounds.x, btn_y + 13, btn_w, AURA_ALIGN_CENTER, 0x00);

    /* 3. Quotes Button */
    bool is_quotes = (navbar->active_tab == AURA_TAB_QUOTES);
    aura_fb_draw_rounded_rect(fb, navbar->btn_quotes.bounds.x, btn_y, btn_w, btn_h, 16, is_quotes ? 2 : 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font, is_quotes ? "* Citas" : "Citas", navbar->btn_quotes.bounds.x, btn_y + 13, btn_w, AURA_ALIGN_CENTER, 0x00);
}

bool aura_ui_navbar_handle_touch(aura_ui_navbar_t *navbar, aura_touch_event_t ev, aura_tab_t *out_new_tab) {
    if (!navbar || !out_new_tab) return false;
    if (ev.type != AURA_TOUCH_UP) return false;

    if (is_point_in_rect(ev.x, ev.y, navbar->btn_home.bounds)) {
        if (navbar->active_tab != AURA_TAB_HOME) {
            *out_new_tab = AURA_TAB_HOME;
            navbar->active_tab = AURA_TAB_HOME;
            return true;
        }
    } else if (is_point_in_rect(ev.x, ev.y, navbar->btn_library.bounds)) {
        if (navbar->active_tab != AURA_TAB_LIBRARY) {
            *out_new_tab = AURA_TAB_LIBRARY;
            navbar->active_tab = AURA_TAB_LIBRARY;
            return true;
        }
    } else if (is_point_in_rect(ev.x, ev.y, navbar->btn_quotes.bounds)) {
        if (navbar->active_tab != AURA_TAB_QUOTES) {
            *out_new_tab = AURA_TAB_QUOTES;
            navbar->active_tab = AURA_TAB_QUOTES;
            return true;
        }
    }

    return false;
}

void aura_ui_draw_progress_bar(aura_fb_t *fb, int x, int y, int w, int h, float percent, uint8_t fill_color, uint8_t border_color) {
    if (!fb || w <= 0 || h <= 0) return;

    if (percent < 0.0f) percent = 0.0f;
    if (percent > 1.0f) percent = 1.0f;

    aura_fb_draw_rounded_rect(fb, x, y, w, h, 4, 1, 0xFF, border_color);

    int fill_w = (int)((w - 4) * percent);
    if (fill_w > 0) {
        aura_fb_fill_rect(fb, x + 2, y + 2, fill_w, h - 4, fill_color);
    }
}

void aura_ui_draw_hero_card(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_book_t *book, int x, int y, int w, int h) {
    if (!fb || !book) return;

    /* Card background with rounded corners */
    aura_fb_draw_rounded_rect(fb, x, y, w, h, 16, 1, 0xFF, 0x00);

    /* Book cover thumbnail: 120x175 */
    int cover_x = x + 16;
    int cover_y = y + 16;
    aura_fb_draw_rounded_rect(fb, cover_x, cover_y, 120, 175, 8, 1, 0xF0, 0x00);
    aura_font_draw_aligned(fb, font_text, "[ Portada ]", cover_x, cover_y + 80, 120, AURA_ALIGN_CENTER, 0x66);

    /* Right column info */
    int info_x = cover_x + 120 + 16;
    int info_w = w - (120 + 48);

    /* Badge: "LECTURA EN CURSO" */
    char badge_str[64];
    snprintf(badge_str, sizeof(badge_str), "* LECTURA EN CURSO  •  %d%% leido", book->percent);
    aura_font_draw_string(fb, font_text, badge_str, info_x, y + 18, 0x44);

    /* Title */
    aura_font_draw_string(fb, font_title, book->title, info_x, y + 42, 0x00);

    /* Author & Rating */
    char author_str[128];
    const char *stars = (book->rating == 5) ? "[*****]" : (book->rating == 4) ? "[****-]" : "[***--]";
    snprintf(author_str, sizeof(author_str), "%s   %s", book->author, stars);
    aura_font_draw_string(fb, font_text, author_str, info_x, y + 72, 0x44);

    /* Progress Info: "Pagina X de Y" */
    char page_str[64];
    snprintf(page_str, sizeof(page_str), "Pagina %d de %d  (%d%%)", book->current_page, book->total_pages, book->percent);
    aura_font_draw_string(fb, font_text, page_str, info_x, y + 104, 0x00);

    /* Progress Bar */
    float pct = (book->total_pages > 0) ? (float)book->current_page / book->total_pages : 0.0f;
    aura_ui_draw_progress_bar(fb, info_x, y + 124, info_w, 10, pct, 0x00, 0x00);

    /* Action buttons: [ Continuar Lectura ] and [ Ver Portada ] */
    int btn_h = 42;
    int btn_y = y + h - btn_h - 16;
    int btn1_w = (int)(info_w * 0.58f);
    int btn2_w = info_w - btn1_w - 10;

    aura_fb_draw_rounded_rect(fb, info_x, btn_y, btn1_w, btn_h, 12, 2, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "> Continuar", info_x, btn_y + 13, btn1_w, AURA_ALIGN_CENTER, 0x00);

    aura_fb_draw_rounded_rect(fb, info_x + btn1_w + 10, btn_y, btn2_w, btn_h, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Portada", info_x + btn1_w + 10, btn_y + 13, btn2_w, AURA_ALIGN_CENTER, 0x00);
}

void aura_ui_draw_quote_card(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_quote_t *quote, int x, int y, int w, int h) {
    if (!fb || !quote) return;

    aura_fb_draw_rounded_rect(fb, x, y, w, h, 14, 1, 0xFF, 0x00);

    /* Header */
    aura_font_draw_string(fb, font_title, "* CITA DEL DIA", x + 16, y + 16, 0x00);

    /* "Otra cita" button */
    aura_fb_draw_rounded_rect(fb, x + w - 105, y + 12, 90, 26, 8, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Otra cita", x + w - 105, y + 17, 90, AURA_ALIGN_CENTER, 0x44);

    /* Quote body */
    char quote_buf[STR_LEN_LARGE + 8];
    snprintf(quote_buf, sizeof(quote_buf), "\"%s\"", quote->text);
    int used_h = aura_font_draw_wrapped(fb, font_text, quote_buf, x + 16, y + 48, w - 32, 4, 0x00);

    /* Quote Author */
    char author_buf[STR_LEN_MED + 8];
    snprintf(author_buf, sizeof(author_buf), "- %s, %s", quote->author, quote->book);
    aura_font_draw_string(fb, font_text, author_buf, x + 16, y + 54 + used_h, 0x44);
}

void aura_ui_draw_stats_card(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_db_t *db, int x, int y, int w, int h) {
    if (!fb || !db) return;

    aura_fb_draw_rounded_rect(fb, x, y, w, h, 14, 1, 0xFF, 0x00);

    /* Header */
    aura_font_draw_string(fb, font_title, "* TU BIBLIOTECA Y ACTIVIDAD", x + 16, y + 14, 0x00);

    /* 3 Stat pills (height 62px) */
    int pill_w = (w - 32 - 16) / 3;
    int pill_y = y + 42;

    int pill1_x = x + 16;
    int pill2_x = pill1_x + pill_w + 8;
    int pill3_x = pill2_x + pill_w + 8;

    /* Pill 1: Libros */
    aura_fb_draw_rounded_rect(fb, pill1_x, pill_y, pill_w, 62, 12, 1, 0xFF, 0x00);
    char p1_str[32];
    snprintf(p1_str, sizeof(p1_str), "%d Libros", db->book_count);
    aura_font_draw_aligned(fb, font_title, p1_str, pill1_x, pill_y + 12, pill_w, AURA_ALIGN_CENTER, 0x00);
    aura_font_draw_aligned(fb, font_text, "En tu Kindle", pill1_x, pill_y + 36, pill_w, AURA_ALIGN_CENTER, 0x66);

    /* Pill 2: Leyendo */
    aura_fb_draw_rounded_rect(fb, pill2_x, pill_y, pill_w, 62, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "2 Leyendo", pill2_x, pill_y + 12, pill_w, AURA_ALIGN_CENTER, 0x00);
    aura_font_draw_aligned(fb, font_text, "En progreso", pill2_x, pill_y + 36, pill_w, AURA_ALIGN_CENTER, 0x66);

    /* Pill 3: Citas */
    aura_fb_draw_rounded_rect(fb, pill3_x, pill_y, pill_w, 62, 12, 1, 0xFF, 0x00);
    char p3_str[32];
    snprintf(p3_str, sizeof(p3_str), "%d Citas", db->quote_count);
    aura_font_draw_aligned(fb, font_title, p3_str, pill3_x, pill_y + 12, pill_w, AURA_ALIGN_CENTER, 0x00);
    aura_font_draw_aligned(fb, font_text, "Guardadas", pill3_x, pill_y + 36, pill_w, AURA_ALIGN_CENTER, 0x66);

    /* Full width shortcut button: "Explorar Biblioteca Completa" */
    int btn_w = w - 32;
    int btn_y = pill_y + 62 + 10;
    aura_fb_draw_rounded_rect(fb, x + 16, btn_y, btn_w, 38, 10, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "Explorar Biblioteca Completa >", x + 16, btn_y + 10, btn_w, AURA_ALIGN_CENTER, 0x00);
}

void aura_ui_draw_book_row(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_book_t *book, int x, int y, int w, int h) {
    if (!fb || !book) return;

    aura_fb_draw_rounded_rect(fb, x, y, w, h, 12, 1, 0xFF, 0x00);

    /* Cover thumbnail: 52x74 */
    aura_fb_draw_rounded_rect(fb, x + 5, y + 5, 52, 74, 4, 1, 0xF0, 0x00);
    aura_font_draw_aligned(fb, font_text, "Img", x + 5, y + 32, 52, AURA_ALIGN_CENTER, 0x66);

    /* Book Info */
    int info_x = x + 52 + 16;
    aura_font_draw_string(fb, font_title, book->title, info_x, y + 10, 0x00);
    aura_font_draw_string(fb, font_text, book->author, info_x, y + 34, 0x66);

    char star_buf[64];
    const char *stars = (book->rating == 5) ? "[*****]" : (book->rating == 4) ? "[****-]" : "[***--]";
    snprintf(star_buf, sizeof(star_buf), "%s  •  %d%% leido", stars, book->percent);
    aura_font_draw_string(fb, font_text, star_buf, info_x, y + 54, 0x44);
}

void aura_ui_draw_quote_row(aura_fb_t *fb, const aura_font_t *font_text, const aura_quote_t *quote, int x, int y, int w, int h) {
    if (!fb || !quote) return;

    aura_fb_draw_rounded_rect(fb, x, y, w, h, 12, 1, 0xFF, 0x00);

    /* Badge */
    const char *badge = quote->is_user_highlight ? "* CITA GUARDADA EN LECTURA" : "* LITERATURA CLASICA";
    aura_font_draw_string(fb, font_text, badge, x + 14, y + 10, 0x66);

    /* Copy Button */
    aura_fb_draw_rounded_rect(fb, x + w - 75, y + 8, 62, 24, 6, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Copiar", x + w - 75, y + 12, 62, AURA_ALIGN_CENTER, 0x00);

    /* Quote Text */
    char buf[STR_LEN_LARGE + 8];
    snprintf(buf, sizeof(buf), "\"%s\"", quote->text);
    int used_h = aura_font_draw_wrapped(fb, font_text, buf, x + 14, y + 38, w - 28, 4, 0x00);

    /* Author & Book */
    char author_str[STR_LEN_MED + 8];
    snprintf(author_str, sizeof(author_str), "- %s, %s", quote->author, quote->book);
    aura_font_draw_string(fb, font_text, author_str, x + 14, y + 44 + used_h, 0x44);
}
