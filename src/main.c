#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "aura_fb.h"
#include "aura_touch.h"
#include "aura_font.h"
#include "aura_image.h"
#include "aura_power.h"
#include "aura_db.h"
#include "aura_ui.h"
#include "aura_reader.h"
#include "aura_sync.h"

#define AUTOSTART_FLAG_PATH "/mnt/us/ENABLE_AURABOOKS_AUTOSTART"

static volatile bool g_running = true;

static void sig_handler(int sig) {
    (void)sig;
    g_running = false;
}

static inline bool in_rect(int px, int py, int x, int y, int w, int h) {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

/* App State */
typedef struct {
    aura_view_mode_t view_mode;
    aura_ui_navbar_t navbar;
    int lib_filter;
    int lib_page;
    int quotes_filter;
    int quotes_page;
    int current_quote_idx;
    bool show_reader_hud;
    int pages_since_full_refresh;
    int reader_font_size;
    aura_reader_doc_t *reader_doc;
    aura_wifi_status_t wifi;
    aura_supabase_sync_t sync;
} aura_app_state_t;

static void render_home_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_app_state_t *state, aura_system_status_t sys_status) {
    int card_w = SCREEN_WIDTH - 32; /* 568px */
    int x = 16;

    aura_ui_draw_header(fb, font_title, font_text, "A U R A B O O K S", sys_status);

    /* 1. Hero Card: Lectura actual (y = 64, h = 240) */
    aura_book_t *active_book = aura_db_get_active_book(db);
    if (active_book) {
        aura_ui_draw_hero_card(fb, font_title, font_text, active_book, x, 64, card_w, 240);
    }

    /* 2. Cita del Día Card (y = 328, h = 150) */
    aura_quote_t *quote = aura_db_get_quote(db, state->current_quote_idx);
    if (quote) {
        aura_ui_draw_quote_card(fb, font_title, font_text, quote, x, 328, card_w, 150);
    }

    /* 3. Tu Biblioteca y Actividad Card (y = 502, h = 175) */
    aura_ui_draw_stats_card(fb, font_title, font_text, db, x, 502, card_w, 175);
}

static void render_library_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_app_state_t *state, aura_system_status_t sys_status) {
    int card_w = SCREEN_WIDTH - 32;
    int x = 16;

    aura_ui_draw_header(fb, font_title, font_text, "B I B L I O T E C A", sys_status);

    /* 5 Filter buttons bar */
    const char *filters[5] = { "Todos", "Autores", "Leyendo", "Por Leer", "Leidos" };
    int fw = 108;
    for (int i = 0; i < 5; i++) {
        bool is_active = (state->lib_filter == i);
        aura_fb_draw_rounded_rect(fb, x + i * (fw + 6), 56, fw, 34, 12, is_active ? 2 : 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, filters[i], x + i * (fw + 6), 64, fw, AURA_ALIGN_CENTER, 0x00);
    }

    /* 6 Book Cards (84px tall, 6px gaps) */
    int card_y = 102;
    int total_books = aura_db_get_book_count(db);
    int books_per_page = 6;
    int start_idx = state->lib_page * books_per_page;
    int total_pages = (total_books + books_per_page - 1) / books_per_page;
    if (total_pages < 1) total_pages = 1;

    for (int i = 0; i < books_per_page; i++) {
        int b_idx = start_idx + i;
        if (b_idx >= total_books) break;
        aura_book_t *b = aura_db_get_book(db, b_idx);
        int y = card_y + i * (84 + 6);
        aura_ui_draw_book_row(fb, font_title, font_text, b, x, y, card_w, 84);
    }

    /* Pagination controls */
    aura_fb_draw_rounded_rect(fb, 145, 652, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "< Anterior", 145, 661, 110, AURA_ALIGN_CENTER, 0x00);

    char page_str[32];
    snprintf(page_str, sizeof(page_str), "Pag. %d de %d", state->lib_page + 1, total_pages);
    aura_font_draw_aligned(fb, font_text, page_str, 260, 661, 80, AURA_ALIGN_CENTER, 0x00);

    aura_fb_draw_rounded_rect(fb, 345, 652, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Siguiente >", 345, 661, 110, AURA_ALIGN_CENTER, 0x00);
}

static void render_quotes_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_app_state_t *state, aura_system_status_t sys_status) {
    int card_w = SCREEN_WIDTH - 32;
    int x = 16;

    aura_ui_draw_header(fb, font_title, font_text, "C I T A S", sys_status);

    /* 3 Filter buttons */
    const char *q_filters[3] = { "Todas", "Mis Citas", "Celebres" };
    int fw = 180;
    for (int i = 0; i < 3; i++) {
        bool is_active = (state->quotes_filter == i);
        aura_fb_draw_rounded_rect(fb, x + i * (fw + 14), 56, fw, 34, 12, is_active ? 2 : 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, q_filters[i], x + i * (fw + 14), 64, fw, AURA_ALIGN_CENTER, 0x00);
    }

    /* 3 Generous Quote Cards */
    int card_y = 106;
    int q_height = 135;
    int total_quotes = aura_db_get_quote_count(db);
    int quotes_per_page = 3;
    int start_idx = state->quotes_page * quotes_per_page;
    int total_pages = (total_quotes + quotes_per_page - 1) / quotes_per_page;
    if (total_pages < 1) total_pages = 1;

    for (int i = 0; i < quotes_per_page; i++) {
        int q_idx = start_idx + i;
        if (q_idx >= total_quotes) break;
        aura_quote_t *q = aura_db_get_quote(db, q_idx);
        int y = card_y + i * (q_height + 12);
        aura_ui_draw_quote_row(fb, font_text, q, x, y, card_w, q_height);
    }

    /* Pagination controls */
    aura_fb_draw_rounded_rect(fb, 145, 560, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "< Anterior", 145, 569, 110, AURA_ALIGN_CENTER, 0x00);

    char page_str[32];
    snprintf(page_str, sizeof(page_str), "Pag. %d de %d", state->quotes_page + 1, total_pages);
    aura_font_draw_aligned(fb, font_text, page_str, 260, 569, 80, AURA_ALIGN_CENTER, 0x00);

    aura_fb_draw_rounded_rect(fb, 345, 560, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Siguiente >", 345, 569, 110, AURA_ALIGN_CENTER, 0x00);
}

static void render_fullscreen_cover(aura_fb_t *fb, const aura_book_t *book) {
    aura_fb_clear(fb, 0xFF);
    if (!book) return;

    if (book->cover_path[0] != '\0') {
        aura_image_t *img = aura_image_load(book->cover_path);
        if (img) {
            aura_image_draw_scaled(fb, img, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            aura_image_free(img);
            return;
        }
    }
}

static void render_current_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_app_state_t *state) {
    aura_system_status_t sys_status = aura_power_get_status();

    if (state->view_mode == AURA_VIEW_READER) {
        if (state->reader_doc) {
            aura_reader_render_page(fb, font_text, state->reader_doc, state->show_reader_hud);
        }
        return;
    }

    if (state->view_mode == AURA_VIEW_COVER_FULLSCREEN) {
        aura_book_t *b = aura_db_get_active_book(db);
        render_fullscreen_cover(fb, b);
        return;
    }

    aura_fb_clear(fb, 0xFF); /* Paper white */

    if (state->navbar.active_tab == AURA_TAB_HOME) {
        render_home_view(fb, font_title, font_text, db, state, sys_status);
    } else if (state->navbar.active_tab == AURA_TAB_LIBRARY) {
        render_library_view(fb, font_title, font_text, db, state, sys_status);
    } else {
        render_quotes_view(fb, font_title, font_text, db, state, sys_status);
    }

    aura_ui_draw_navbar(fb, font_text, &state->navbar);

    if (state->view_mode == AURA_VIEW_SETTINGS) {
        bool autoboot = (access(AUTOSTART_FLAG_PATH, F_OK) == 0);
        aura_ui_draw_settings_modal(fb, font_title, font_text, autoboot, state->wifi, state->sync);
    }
}

static void open_book_in_reader(aura_app_state_t *state, aura_db_t *db, const aura_font_t *font_text, int book_idx) {
    aura_book_t *book = aura_db_get_book(db, book_idx);
    if (!book) return;

    if (state->reader_doc) {
        aura_reader_close(state->reader_doc);
        state->reader_doc = NULL;
    }

    char book_path[256];
    snprintf(book_path, sizeof(book_path), "/mnt/us/documents/%s", book->filename);

    state->reader_doc = aura_reader_open(book_path);
    if (!state->reader_doc) {
        /* Try current directory fallback */
        state->reader_doc = aura_reader_open(book->filename);
    }

    if (state->reader_doc) {
        snprintf(state->reader_doc->title, sizeof(state->reader_doc->title), "%s", book->title);
        int margin_x = 36;
        int margin_y = 36;
        int footer_h = 36;
        aura_reader_paginate(state->reader_doc, font_text, margin_x, margin_y, SCREEN_WIDTH - 2 * margin_x, SCREEN_HEIGHT - margin_y - footer_h - 10);
        
        if (book->current_page > 0) {
            aura_reader_goto_page(state->reader_doc, book->current_page);
        }
    }

    db->active_book_idx = book_idx;
    state->view_mode = AURA_VIEW_READER;
    state->show_reader_hud = false;
    state->pages_since_full_refresh = 0;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("====================================================\n");
    printf("   AuraBooks Native Embedded OS v0.2 (Kindle Touch)  \n");
    printf("   Architecture: ARMv7-A Cortex-A8 (i.MX508 EPDC)   \n");
    printf("====================================================\n");

    /* 1. Initialize Framebuffer */
    aura_fb_t *fb = aura_fb_init("/dev/fb0");
    if (!fb) {
        fprintf(stderr, "[main] Fatal: Unable to initialize framebuffer.\n");
        return 1;
    }

    /* 2. Initialize Touch Input */
    aura_touch_t *touch = aura_touch_init("/dev/input/event0");

    /* 3. Initialize Fonts (Literata TrueType) */
    aura_font_t *font_title = aura_font_load("Literata-Bold.ttf", 22);
    if (!font_title) font_title = aura_font_load("/mnt/us/fonts/Literata-Bold.ttf", 22);

    aura_font_t *font_text = aura_font_load("Literata-Regular.ttf", 14);
    if (!font_text) font_text = aura_font_load("/mnt/us/fonts/Literata-Regular.ttf", 14);

    /* 4. Initialize Database & Storage */
    aura_db_t *db = aura_db_init("/mnt/us/aurabooks_data.bin");

    /* 5. Initialize App State */
    aura_app_state_t state;
    memset(&state, 0, sizeof(state));
    state.view_mode = AURA_VIEW_SHELL;
    state.navbar.active_tab = AURA_TAB_HOME;
    state.lib_filter = 0;
    state.lib_page = 0;
    state.quotes_filter = 0;
    state.quotes_page = 0;
    state.current_quote_idx = 0;
    state.reader_font_size = 14;

    /* Initialize Wi-Fi and Supabase sync subsystems */
    aura_sync_init(&state.sync, NULL, NULL);
    state.wifi = aura_sync_get_wifi_status();

    /* First frame: render complete view and trigger clean hardware refresh */
    render_current_view(fb, font_title, font_text, db, &state);
    aura_fb_flush_full(fb, AURA_REFRESH_FULL);
    printf("[main] Initial screen drawn and flashed via EPDC GC16.\n");

    aura_touch_event_t ev;
    while (g_running) {
        if (touch && aura_touch_poll(touch, &ev)) {
            bool needs_render = false;
            aura_refresh_mode_t refresh_mode = AURA_REFRESH_FAST;

            /* ========================================================= */
            /* VIEW: FULLSCREEN COVER ART                                */
            /* ========================================================= */
            if (state.view_mode == AURA_VIEW_COVER_FULLSCREEN) {
                if (ev.type == AURA_TOUCH_UP) {
                    state.view_mode = AURA_VIEW_SHELL;
                    needs_render = true;
                    refresh_mode = AURA_REFRESH_FAST;
                }
            }

            /* ========================================================= */
            /* VIEW: SETTINGS MODAL                                      */
            /* ========================================================= */
            else if (state.view_mode == AURA_VIEW_SETTINGS) {
                if (ev.type == AURA_TOUCH_UP) {
                    int mw = 520;
                    int mh = 640;
                    int mx = (SCREEN_WIDTH - mw) / 2;
                    int my = 60;

                    /* 1. Wi-Fi Button: mx + 24, my + 112, mw - 48, 36 */
                    if (in_rect(ev.x, ev.y, mx + 24, my + 112, mw - 48, 36)) {
                        state.wifi = aura_sync_get_wifi_status();
                        printf("[settings] Wi-Fi refreshed: %s\n", state.wifi.wifi_ssid);
                        needs_render = true;
                    }
                    /* 2. Supabase Sync Button: mx + 24, my + 248, mw - 48, 42 */
                    else if (in_rect(ev.x, ev.y, mx + 24, my + 248, mw - 48, 42)) {
                        printf("[settings] Triggering Supabase cloud sync...\n");
                        aura_sync_pull_from_supabase(&state.sync, db);
                        needs_render = true;
                        refresh_mode = AURA_REFRESH_FULL;
                    }
                    /* 3. Toggle Autoarranque: mx + 24, my + 370, mw - 48, 38 */
                    else if (in_rect(ev.x, ev.y, mx + 24, my + 370, mw - 48, 38)) {
                        bool autoboot = (access(AUTOSTART_FLAG_PATH, F_OK) == 0);
                        if (autoboot) {
                            remove(AUTOSTART_FLAG_PATH);
                            printf("[settings] Disabled autoboot.\n");
                        } else {
                            FILE *fp = fopen(AUTOSTART_FLAG_PATH, "w");
                            if (fp) {
                                fputs("aurabooks_native=1\n", fp);
                                fclose(fp);
                                printf("[settings] Enabled pure AuraBooks autoboot.\n");
                            }
                        }
                        needs_render = true;
                    }
                    /* 4. Close Button: mx + 170, my + mh - 55, 180, 42 */
                    else if (in_rect(ev.x, ev.y, mx + 170, my + mh - 55, 180, 42)) {
                        state.view_mode = AURA_VIEW_SHELL;
                        needs_render = true;
                    }
                }
            }

            /* ========================================================= */
            /* VIEW: READER ENGINE                                       */
            /* ========================================================= */
            else if (state.view_mode == AURA_VIEW_READER) {
                if (ev.type == AURA_GESTURE_SWIPE_DOWN) {
                    /* Return to shell */
                    if (state.reader_doc) {
                        aura_book_t *b = aura_db_get_active_book(db);
                        if (b) {
                            aura_db_update_progress(db, b->filename, state.reader_doc->current_page, state.reader_doc->total_pages);
                            aura_sync_push_progress_to_supabase(&state.sync, b->title, state.reader_doc->current_page, state.reader_doc->total_pages, "reading");
                        }
                    }
                    state.view_mode = AURA_VIEW_SHELL;
                    needs_render = true;
                } else if (ev.type == AURA_GESTURE_SWIPE_LEFT) {
                    /* Next Page */
                    if (state.reader_doc && aura_reader_next_page(state.reader_doc)) {
                        needs_render = true;
                    }
                } else if (ev.type == AURA_GESTURE_SWIPE_RIGHT) {
                    /* Prev Page */
                    if (state.reader_doc && aura_reader_prev_page(state.reader_doc)) {
                        needs_render = true;
                    }
                } else if (ev.type == AURA_TOUCH_UP) {
                    if (state.show_reader_hud) {
                        /* HUD: [ < Volver ] */
                        if (in_rect(ev.x, ev.y, 20, 16, 90, 36)) {
                            if (state.reader_doc) {
                                aura_book_t *b = aura_db_get_active_book(db);
                                if (b) {
                                    aura_db_update_progress(db, b->filename, state.reader_doc->current_page, state.reader_doc->total_pages);
                                    aura_sync_push_progress_to_supabase(&state.sync, b->title, state.reader_doc->current_page, state.reader_doc->total_pages, "reading");
                                }
                            }
                            state.view_mode = AURA_VIEW_SHELL;
                            needs_render = true;
                        }
                        /* HUD: [ A- ] */
                        else if (in_rect(ev.x, ev.y, SCREEN_WIDTH - 110, 16, 40, 36)) {
                            if (state.reader_font_size > 10) {
                                state.reader_font_size -= 2;
                                aura_font_t *new_f = aura_font_load("Literata-Regular.ttf", state.reader_font_size);
                                if (new_f) {
                                    aura_font_close(font_text);
                                    font_text = new_f;
                                    aura_reader_paginate(state.reader_doc, font_text, 36, 36, SCREEN_WIDTH - 72, SCREEN_HEIGHT - 82);
                                    needs_render = true;
                                }
                            }
                        }
                        /* HUD: [ A+ ] */
                        else if (in_rect(ev.x, ev.y, SCREEN_WIDTH - 65, 16, 40, 36)) {
                            if (state.reader_font_size < 32) {
                                state.reader_font_size += 2;
                                aura_font_t *new_f = aura_font_load("Literata-Regular.ttf", state.reader_font_size);
                                if (new_f) {
                                    aura_font_close(font_text);
                                    font_text = new_f;
                                    aura_reader_paginate(state.reader_doc, font_text, 36, 36, SCREEN_WIDTH - 72, SCREEN_HEIGHT - 82);
                                    needs_render = true;
                                }
                            }
                        } else {
                            state.show_reader_hud = false;
                            needs_render = true;
                        }
                    } else {
                        /* Tap top 70px toggles HUD */
                        if (ev.y < 70) {
                            state.show_reader_hud = true;
                            needs_render = true;
                        }
                        /* Tap right 70% -> Next Page */
                        else if (ev.x >= 180) {
                            if (state.reader_doc && aura_reader_next_page(state.reader_doc)) {
                                state.pages_since_full_refresh++;
                                if (state.pages_since_full_refresh >= 6) {
                                    refresh_mode = AURA_REFRESH_FULL;
                                    state.pages_since_full_refresh = 0;
                                }
                                needs_render = true;
                            }
                        }
                        /* Tap left 30% -> Prev Page */
                        else {
                            if (state.reader_doc && aura_reader_prev_page(state.reader_doc)) {
                                needs_render = true;
                            }
                        }
                    }
                }
            }

            /* ========================================================= */
            /* VIEW: SHELL (HOME / LIBRARY / QUOTES)                     */
            /* ========================================================= */
            else if (state.view_mode == AURA_VIEW_SHELL) {
                if (ev.type == AURA_TOUCH_UP) {
                    /* 1. Header: Ajustes button (488, 14, 96, 32) */
                    if (in_rect(ev.x, ev.y, 488, 14, 96, 32)) {
                        state.view_mode = AURA_VIEW_SETTINGS;
                        needs_render = true;
                    }

                    /* 2. Floating Navbar Buttons */
                    aura_tab_t new_tab;
                    if (aura_ui_navbar_handle_touch(&state.navbar, ev, &new_tab)) {
                        needs_render = true;
                    }

                    /* 3. Tab: HOME */
                    if (state.navbar.active_tab == AURA_TAB_HOME) {
                        /* Hero Card: [ > Continuar ] button */
                        if (in_rect(ev.x, ev.y, 168, 246, 230, 42)) {
                            open_book_in_reader(&state, db, font_text, db->active_book_idx);
                            needs_render = true;
                        }
                        /* Hero Card: [ Portada ] button */
                        else if (in_rect(ev.x, ev.y, 408, 246, 160, 42)) {
                            state.view_mode = AURA_VIEW_COVER_FULLSCREEN;
                            needs_render = true;
                        }
                        /* Quote Card: [ Otra cita ] button */
                        else if (in_rect(ev.x, ev.y, SCREEN_WIDTH - 16 - 105, 340, 90, 26)) {
                            int total_q = aura_db_get_quote_count(db);
                            if (total_q > 1) {
                                state.current_quote_idx = (state.current_quote_idx + 1) % total_q;
                                needs_render = true;
                            }
                        }
                        /* Stats Card: [ Explorar Biblioteca Completa > ] */
                        else if (in_rect(ev.x, ev.y, 32, 616, SCREEN_WIDTH - 64, 38)) {
                            state.navbar.active_tab = AURA_TAB_LIBRARY;
                            needs_render = true;
                        }
                    }

                    /* 4. Tab: LIBRARY */
                    else if (state.navbar.active_tab == AURA_TAB_LIBRARY) {
                        /* 5 Filter buttons */
                        int fw = 108;
                        for (int i = 0; i < 5; i++) {
                            if (in_rect(ev.x, ev.y, 16 + i * (fw + 6), 56, fw, 34)) {
                                state.lib_filter = i;
                                state.lib_page = 0;
                                needs_render = true;
                                break;
                            }
                        }

                        /* 6 Book rows (card_y = 102, h = 84, gap = 6) */
                        for (int i = 0; i < 6; i++) {
                            int y = 102 + i * 90;
                            if (in_rect(ev.x, ev.y, 16, y, SCREEN_WIDTH - 32, 84)) {
                                int b_idx = state.lib_page * 6 + i;
                                if (b_idx < aura_db_get_book_count(db)) {
                                    open_book_in_reader(&state, db, font_text, b_idx);
                                    needs_render = true;
                                }
                                break;
                            }
                        }

                        /* Pagination: [ < Anterior ] */
                        if (in_rect(ev.x, ev.y, 145, 652, 110, 36)) {
                            if (state.lib_page > 0) {
                                state.lib_page--;
                                needs_render = true;
                            }
                        }
                        /* Pagination: [ Siguiente > ] */
                        else if (in_rect(ev.x, ev.y, 345, 652, 110, 36)) {
                            int total_pages = (aura_db_get_book_count(db) + 5) / 6;
                            if (state.lib_page + 1 < total_pages) {
                                state.lib_page++;
                                needs_render = true;
                            }
                        }
                    }

                    /* 5. Tab: QUOTES */
                    else if (state.navbar.active_tab == AURA_TAB_QUOTES) {
                        /* 3 Filter buttons */
                        int fw = 180;
                        for (int i = 0; i < 3; i++) {
                            if (in_rect(ev.x, ev.y, 16 + i * (fw + 14), 56, fw, 34)) {
                                state.quotes_filter = i;
                                state.quotes_page = 0;
                                needs_render = true;
                                break;
                            }
                        }

                        /* Pagination: [ < Anterior ] */
                        if (in_rect(ev.x, ev.y, 145, 560, 110, 36)) {
                            if (state.quotes_page > 0) {
                                state.quotes_page--;
                                needs_render = true;
                            }
                        }
                        /* Pagination: [ Siguiente > ] */
                        else if (in_rect(ev.x, ev.y, 345, 560, 110, 36)) {
                            int total_pages = (aura_db_get_quote_count(db) + 2) / 3;
                            if (state.quotes_page + 1 < total_pages) {
                                state.quotes_page++;
                                needs_render = true;
                            }
                        }
                    }
                }
            }

            if (needs_render) {
                render_current_view(fb, font_title, font_text, db, &state);
                aura_fb_flush_full(fb, refresh_mode);
            }
        }

        /* 50 Hz polling rate: 20ms sleep yields CPU, saves battery */
        usleep(20000);
    }

    printf("[main] Shutting down AuraBooks Native cleanly...\n");
    if (state.reader_doc) aura_reader_close(state.reader_doc);
    if (touch) aura_touch_close(touch);
    if (db) aura_db_close(db);
    if (font_title) aura_font_close(font_title);
    if (font_text) aura_font_close(font_text);
    aura_fb_close(fb);

    return 0;
}
