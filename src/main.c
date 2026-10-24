#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

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

static inline uint64_t get_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;
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
    char toast_msg[128];
    uint64_t toast_expiry_ms;
} aura_app_state_t;

static void show_toast(aura_app_state_t *state, const char *msg, uint32_t duration_ms) {
    if (!state || !msg) return;
    snprintf(state->toast_msg, sizeof(state->toast_msg), "%s", msg);
    state->toast_expiry_ms = get_now_ms() + duration_ms;
}

static char* find_book_file(const char *title, const char *filename) {
    static char found_path[256];
    if (filename && filename[0]) {
        snprintf(found_path, sizeof(found_path), "/mnt/us/documents/%s", filename);
        if (access(found_path, R_OK) == 0) return found_path;

        snprintf(found_path, sizeof(found_path), "%s", filename);
        if (access(found_path, R_OK) == 0) return found_path;
    }

    if (title && strlen(title) >= 3) {
        /* Scan /mnt/us/documents/ for matching title */
        DIR *dir = opendir("/mnt/us/documents");
        if (dir) {
            struct dirent *ent;
            while ((ent = readdir(dir)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                if (strcasestr(ent->d_name, title)) {
                    snprintf(found_path, sizeof(found_path), "/mnt/us/documents/%s", ent->d_name);
                    closedir(dir);
                    return found_path;
                }
            }
            closedir(dir);
        }
    }

    return NULL;
}

static void render_home_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_app_state_t *state, aura_system_status_t sys_status) {
    int card_w = SCREEN_WIDTH - 32; /* 568px */
    int x = 16;

    aura_ui_draw_header(fb, font_title, font_text, "AuraBooks", sys_status);

    /* 1. Hero Card (Active Book) */
    aura_book_t *active = aura_db_get_active_book(db);
    aura_ui_draw_hero_card(fb, font_title, font_text, active, x, 56, card_w, 246);

    /* 2. Literary Quote Card */
    int q_idx = state->current_quote_idx;
    if (q_idx >= aura_db_get_quote_count(db)) q_idx = 0;
    aura_quote_t *q = aura_db_get_quote(db, q_idx);
    aura_ui_draw_quote_card(fb, font_title, font_text, q, x, 314, card_w, 142);

    /* 3. Library Summary Card */
    aura_ui_draw_stats_card(fb, font_title, font_text, db, x, 468, card_w, 196);
}

static void render_library_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_db_t *db, const aura_app_state_t *state, aura_system_status_t sys_status) {
    int x = 16;
    aura_ui_draw_header(fb, font_title, font_text, "Biblioteca", sys_status);

    /* 5 Filter Pills: Todos, Autores, Leyendo, Por Leer, Leidos (h = 34px) */
    const char *filters[] = { "Todos", "Autores", "Leyendo", "Por Leer", "Leidos" };
    int fw = 108;
    int f_gap = 6;
    for (int i = 0; i < 5; i++) {
        int fx = x + i * (fw + f_gap);
        bool is_sel = (state->lib_filter == i);
        aura_fb_draw_rounded_rect(fb, fx, 56, fw, 34, 10, is_sel ? 2 : 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, filters[i], fx, 66, fw, AURA_ALIGN_CENTER, 0x00);
    }

    /* 6 Book rows (h = 84px, gap = 6px) */
    int start_y = 102;
    int card_w = SCREEN_WIDTH - 32;
    int card_h = 84;
    int gap = 6;

    int total_books = aura_db_get_book_count(db);
    int books_per_page = 6;
    int page_start = state->lib_page * books_per_page;

    for (int i = 0; i < books_per_page; i++) {
        int book_idx = page_start + i;
        if (book_idx >= total_books) break;

        aura_book_t *b = aura_db_get_book((aura_db_t*)db, book_idx);
        int cy = start_y + i * (card_h + gap);
        aura_ui_draw_book_row(fb, font_title, font_text, b, x, cy, card_w, card_h);
    }

    /* Pagination */
    int total_pages = (total_books + books_per_page - 1) / books_per_page;
    if (total_pages < 1) total_pages = 1;

    char page_str[32];
    snprintf(page_str, sizeof(page_str), "%d / %d", state->lib_page + 1, total_pages);
    aura_font_draw_aligned(fb, font_text, page_str, 0, 660, SCREEN_WIDTH, AURA_ALIGN_CENTER, 0x44);

    if (state->lib_page > 0) {
        aura_fb_draw_rounded_rect(fb, 145, 652, 110, 36, 12, 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, "< Anterior", 145, 662, 110, AURA_ALIGN_CENTER, 0x00);
    }
    if (state->lib_page + 1 < total_pages) {
        aura_fb_draw_rounded_rect(fb, 345, 652, 110, 36, 12, 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, "Siguiente >", 345, 662, 110, AURA_ALIGN_CENTER, 0x00);
    }
}

static void render_quotes_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_db_t *db, const aura_app_state_t *state, aura_system_status_t sys_status) {
    int x = 16;
    aura_ui_draw_header(fb, font_title, font_text, "Citas y Notas", sys_status);

    /* 3 Filter Pills: Todas, Mis Citas, Celebres (180x34) */
    const char *qfilters[] = { "Todas", "Mis Citas", "Celebres" };
    int fw = 180;
    int f_gap = 14;
    for (int i = 0; i < 3; i++) {
        int fx = x + i * (fw + f_gap);
        bool is_sel = (state->quotes_filter == i);
        aura_fb_draw_rounded_rect(fb, fx, 56, fw, 34, 10, is_sel ? 2 : 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, qfilters[i], fx, 66, fw, AURA_ALIGN_CENTER, 0x00);
    }

    /* 3 Quote rows */
    int start_y = 104;
    int card_w = SCREEN_WIDTH - 32;
    int card_h = 142;
    int gap = 10;

    int total_quotes = aura_db_get_quote_count(db);
    int quotes_per_page = 3;
    int page_start = state->quotes_page * quotes_per_page;

    for (int i = 0; i < quotes_per_page; i++) {
        int q_idx = page_start + i;
        if (q_idx >= total_quotes) break;

        aura_quote_t *q = aura_db_get_quote((aura_db_t*)db, q_idx);
        int cy = start_y + i * (card_h + gap);
        aura_ui_draw_quote_row(fb, font_text, q, x, cy, card_w, card_h);
    }

    /* Pagination */
    int total_pages = (total_quotes + quotes_per_page - 1) / quotes_per_page;
    if (total_pages < 1) total_pages = 1;

    char page_str[32];
    snprintf(page_str, sizeof(page_str), "%d / %d", state->quotes_page + 1, total_pages);
    aura_font_draw_aligned(fb, font_text, page_str, 0, 568, SCREEN_WIDTH, AURA_ALIGN_CENTER, 0x44);

    if (state->quotes_page > 0) {
        aura_fb_draw_rounded_rect(fb, 145, 560, 110, 36, 12, 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, "< Anterior", 145, 570, 110, AURA_ALIGN_CENTER, 0x00);
    }
    if (state->quotes_page + 1 < total_pages) {
        aura_fb_draw_rounded_rect(fb, 345, 560, 110, 36, 12, 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, "Siguiente >", 345, 570, 110, AURA_ALIGN_CENTER, 0x00);
    }
}

static void render_fullscreen_cover(aura_fb_t *fb, const aura_book_t *book) {
    aura_fb_clear(fb, 0xFF);
    if (!book) return;

    if (book->cover_path[0] != '\0') {
        aura_image_t *img = aura_image_load(book->cover_path);
        if (img) {
            aura_image_dither_floyd_steinberg(fb, img, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            aura_image_free(img);
            return;
        }
    }

    /* Minimalist sleep cover */
    aura_fb_draw_rounded_rect(fb, 40, 100, SCREEN_WIDTH - 80, SCREEN_HEIGHT - 200, 24, 2, 0xFF, 0x00);
}

static void render_current_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_app_state_t *state) {
    aura_system_status_t sys_status = aura_power_get_status();

    if (state->view_mode == AURA_VIEW_USB_MODAL) {
        aura_ui_draw_usb_modal(fb, font_title, font_text);
        return;
    }

    if (state->view_mode == AURA_VIEW_COVER_FULLSCREEN) {
        aura_book_t *b = aura_db_get_active_book(db);
        render_fullscreen_cover(fb, b);
        return;
    }

    if (state->view_mode == AURA_VIEW_READER) {
        if (state->reader_doc) {
            aura_reader_render_page(fb, font_text, state->reader_doc, state->show_reader_hud);
        } else {
            aura_book_t *b = aura_db_get_active_book(db);
            aura_ui_draw_missing_book_screen(fb, font_title, font_text, b);
        }
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

    /* Active Toast Notification */
    uint64_t now = get_now_ms();
    if (now < state->toast_expiry_ms && state->toast_msg[0] != '\0') {
        aura_ui_draw_toast(fb, font_text, state->toast_msg);
    }
}

static void open_book_in_reader(aura_app_state_t *state, aura_db_t *db, const aura_font_t *font_text, int book_idx) {
    aura_book_t *book = aura_db_get_book(db, book_idx);
    if (!book) return;

    if (state->reader_doc) {
        aura_reader_close(state->reader_doc);
        state->reader_doc = NULL;
    }

    char *target = find_book_file(book->title, book->filename);
    if (target) {
        state->reader_doc = aura_reader_open(target);
    } else {
        state->reader_doc = NULL;
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
    printf("   AuraBooks Native Embedded OS v0.3 (Kindle Touch)  \n");
    printf("   Architecture: ARMv7-A Cortex-A8 (i.MX508 EPDC)   \n");
    printf("====================================================\n");

    /* 1. Initialize Framebuffer */
    aura_fb_t *fb = aura_fb_init("/dev/fb0");
    if (!fb) {
        fprintf(stderr, "[main] Fatal: Framebuffer init failed.\n");
        return 1;
    }

    /* 2. Initialize Touch Controller */
    aura_touch_t *touch = aura_touch_init("/dev/input/event0");
    if (!touch) {
        fprintf(stderr, "[main] Warning: Starting without hardware touch.\n");
    }

    /* 3. Load Vector Typography (Literata) */
    aura_font_t *font_title = aura_font_load("Literata-Bold.ttf", 20);
    aura_font_t *font_text = aura_font_load("Literata-Regular.ttf", 16);

    if (!font_title) font_title = aura_font_load("/mnt/us/aurabooks/fonts/Literata-Bold.ttf", 20);
    if (!font_text) font_text = aura_font_load("/mnt/us/aurabooks/fonts/Literata-Regular.ttf", 16);

    if (!font_title || !font_text) {
        fprintf(stderr, "[main] Fatal: Unable to load Literata TrueType fonts.\n");
        aura_fb_close(fb);
        return 1;
    }

    /* 4. Initialize Local Database */
    aura_db_t *db = aura_db_init("/mnt/us/aurabooks_data.bin");
    if (!db) {
        fprintf(stderr, "[main] Fatal: Database init failed.\n");
        aura_font_close(font_title);
        aura_font_close(font_text);
        aura_fb_close(fb);
        return 1;
    }

    /* 5. Initialize Application State */
    aura_app_state_t state;
    memset(&state, 0, sizeof(state));
    state.view_mode = AURA_VIEW_SHELL;
    state.navbar.active_tab = AURA_TAB_HOME;
    state.reader_font_size = 18;
    state.toast_expiry_ms = 0;
    aura_sync_init(&state.sync, NULL, NULL);
    state.wifi = aura_sync_get_wifi_status();

    /* First frame: render complete view and trigger clean hardware refresh */
    render_current_view(fb, font_title, font_text, db, &state);
    aura_fb_flush_full(fb, AURA_REFRESH_FULL);
    printf("[main] Initial screen drawn and flashed via EPDC GC16.\n");

    static bool prev_usb_online = false;
    aura_touch_event_t ev;

    while (g_running) {
        /* Monitor USB Connection state */
        aura_system_status_t p_status = aura_power_get_status();
        if (p_status.usb_online && !prev_usb_online) {
            aura_db_save(db);
            if (state.reader_doc) {
                aura_reader_close(state.reader_doc);
                state.reader_doc = NULL;
            }
            state.view_mode = AURA_VIEW_USB_MODAL;
            render_current_view(fb, font_title, font_text, db, &state);
            aura_fb_flush_full(fb, AURA_REFRESH_FULL);
        } else if (!p_status.usb_online && prev_usb_online) {
            /* USB disconnected: re-scan documents folder for newly copied books */
            aura_db_scan_documents(db, "/mnt/us/documents");
            if (state.view_mode == AURA_VIEW_USB_MODAL) {
                state.view_mode = AURA_VIEW_SHELL;
            }
            show_toast(&state, "Biblioteca de libros actualizada", 2500);
            render_current_view(fb, font_title, font_text, db, &state);
            aura_fb_flush_full(fb, AURA_REFRESH_FULL);
        }
        prev_usb_online = p_status.usb_online;

        if (touch && aura_touch_poll(touch, &ev)) {
            bool needs_render = false;
            aura_refresh_mode_t refresh_mode = AURA_REFRESH_FAST;

            /* ========================================================= */
            /* VIEW: USB MODAL                                           */
            /* ========================================================= */
            if (state.view_mode == AURA_VIEW_USB_MODAL) {
                if (ev.type == AURA_TOUCH_UP) {
                    state.view_mode = AURA_VIEW_SHELL;
                    needs_render = true;
                    refresh_mode = AURA_REFRESH_FULL;
                }
            }

            /* ========================================================= */
            /* VIEW: FULLSCREEN COVER ART                                */
            /* ========================================================= */
            else if (state.view_mode == AURA_VIEW_COVER_FULLSCREEN) {
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
                        show_toast(&state, "Antena Wi-Fi actualizada", 2500);
                        needs_render = true;
                    }
                    /* 2. Supabase Sync Button: mx + 24, my + 248, mw - 48, 42 */
                    else if (in_rect(ev.x, ev.y, mx + 24, my + 248, mw - 48, 42)) {
                        show_toast(&state, "Sincronizando con Supabase...", 3000);
                        render_current_view(fb, font_title, font_text, db, &state);
                        aura_fb_flush_full(fb, AURA_REFRESH_FAST);
                        
                        aura_sync_pull_from_supabase(&state.sync, db);
                        show_toast(&state, "Sincronizacion completada", 2500);
                        needs_render = true;
                        refresh_mode = AURA_REFRESH_FULL;
                    }
                    /* 3. Toggle Autoarranque: mx + 24, my + 370, mw - 48, 38 */
                    else if (in_rect(ev.x, ev.y, mx + 24, my + 370, mw - 48, 38)) {
                        bool autoboot = (access(AUTOSTART_FLAG_PATH, F_OK) == 0);
                        if (autoboot) {
                            remove(AUTOSTART_FLAG_PATH);
                            show_toast(&state, "Autoarranque Desactivado (Kindle original)", 2500);
                        } else {
                            FILE *fp = fopen(AUTOSTART_FLAG_PATH, "w");
                            if (fp) {
                                fputs("aurabooks_native=1\n", fp);
                                fclose(fp);
                                show_toast(&state, "Autoarranque Puro AuraBooks Activado", 2500);
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
                if (!state.reader_doc) {
                    /* Missing book file fallback: tap anywhere or [< Volver] returns to library */
                    if (ev.type == AURA_TOUCH_UP) {
                        state.view_mode = AURA_VIEW_SHELL;
                        state.navbar.active_tab = AURA_TAB_LIBRARY;
                        needs_render = true;
                    }
                } else if (ev.type == AURA_GESTURE_SWIPE_DOWN) {
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
