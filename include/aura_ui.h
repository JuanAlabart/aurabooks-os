#ifndef AURA_UI_H
#define AURA_UI_H

#include "aura_fb.h"
#include "aura_touch.h"
#include "aura_font.h"
#include "aura_image.h"
#include "aura_db.h"
#include "aura_power.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AURA_TAB_HOME = 0,
    AURA_TAB_LIBRARY,
    AURA_TAB_QUOTES
} aura_tab_t;

typedef enum {
    AURA_FILTER_ALL = 0,
    AURA_FILTER_AUTHORS,
    AURA_FILTER_READING,
    AURA_FILTER_TO_READ,
    AURA_FILTER_COMPLETED
} aura_lib_filter_t;

typedef enum {
    AURA_QFILTER_ALL = 0,
    AURA_QFILTER_MINE,
    AURA_QFILTER_FAMOUS
} aura_quotes_filter_t;

typedef enum {
    AURA_VIEW_SHELL = 0,
    AURA_VIEW_READER,
    AURA_VIEW_COVER_FULLSCREEN,
    AURA_VIEW_SETTINGS
} aura_view_mode_t;

typedef struct {
    aura_rect_t bounds;
    const char *text;
    bool is_active;
} aura_ui_button_t;

typedef struct {
    aura_tab_t active_tab;
    aura_ui_button_t btn_home;
    aura_ui_button_t btn_library;
    aura_ui_button_t btn_quotes;
} aura_ui_navbar_t;

/* High-level View Composers */
void aura_ui_draw_header(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const char *title, aura_system_status_t status);
void aura_ui_draw_navbar(aura_fb_t *fb, const aura_font_t *font, aura_ui_navbar_t *navbar);
bool aura_ui_navbar_handle_touch(aura_ui_navbar_t *navbar, aura_touch_event_t ev, aura_tab_t *out_new_tab);

void aura_ui_draw_hero_card(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_book_t *book, int x, int y, int w, int h);
void aura_ui_draw_quote_card(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_quote_t *quote, int x, int y, int w, int h);
void aura_ui_draw_stats_card(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_db_t *db, int x, int y, int w, int h);

void aura_ui_draw_book_row(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_book_t *book, int x, int y, int w, int h);
void aura_ui_draw_quote_row(aura_fb_t *fb, const aura_font_t *font_text, const aura_quote_t *quote, int x, int y, int w, int h);

void aura_ui_draw_progress_bar(aura_fb_t *fb, int x, int y, int w, int h, float percent, uint8_t fill_color, uint8_t border_color);

/* Settings Modal */
void aura_ui_draw_settings_modal(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, bool autoboot_enabled);

#ifdef __cplusplus
}
#endif

#endif /* AURA_UI_H */
