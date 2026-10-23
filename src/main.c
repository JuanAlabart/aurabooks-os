#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>

#include "aura_fb.h"
#include "aura_touch.h"
#include "aura_font.h"
#include "aura_db.h"
#include "aura_ui.h"

static volatile bool g_running = true;

static void sig_handler(int sig) {
    (void)sig;
    g_running = false;
}

static void render_home_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db) {
    int card_w = SCREEN_WIDTH - 32; /* 568px */
    int x = 16;

    /* Top Header */
    aura_ui_draw_header(fb, font_title, "A U R A B O O K S");

    /* 1. Hero Card: Lectura actual (y = 64, h = 240) */
    aura_book_t *active_book = aura_db_get_active_book(db);
    if (active_book) {
        aura_ui_draw_hero_card(fb, font_title, font_text, active_book, x, 64, card_w, 240);
    }

    /* 2. Cita del Día Card (y = 328, h = 150) */
    aura_quote_t *quote = aura_db_get_quote(db, 0);
    if (quote) {
        aura_ui_draw_quote_card(fb, font_title, font_text, quote, x, 328, card_w, 150);
    }

    /* 3. Tu Biblioteca y Actividad Card (y = 502, h = 175) */
    aura_ui_draw_stats_card(fb, font_title, font_text, db, x, 502, card_w, 175);
}

static void render_library_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db) {
    int card_w = SCREEN_WIDTH - 32;
    int x = 16;

    aura_ui_draw_header(fb, font_title, "B I B L I O T E C A");

    /* 5 Filter buttons bar */
    const char *filters[5] = { "Todos", "Autores", "Leyendo", "Por Leer", "Leidos" };
    int fw = 108;
    for (int i = 0; i < 5; i++) {
        aura_fb_draw_rounded_rect(fb, x + i * (fw + 6), 56, fw, 34, 12, (i == 0) ? 2 : 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, filters[i], x + i * (fw + 6), 64, fw, AURA_ALIGN_CENTER, 0x00);
    }

    /* 6 Book Cards (84px tall, 6px gaps) */
    int card_y = 102;
    int count = aura_db_get_book_count(db);
    int display_count = (count > 6) ? 6 : count;

    for (int i = 0; i < display_count; i++) {
        aura_book_t *b = aura_db_get_book(db, i);
        int y = card_y + i * (84 + 6);
        aura_ui_draw_book_row(fb, font_title, font_text, b, x, y, card_w, 84);
    }

    /* Pagination controls */
    aura_fb_draw_rounded_rect(fb, 145, 652, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "< Anterior", 145, 661, 110, AURA_ALIGN_CENTER, 0x00);

    aura_font_draw_aligned(fb, font_text, "Pag. 1 de 1", 260, 661, 80, AURA_ALIGN_CENTER, 0x00);

    aura_fb_draw_rounded_rect(fb, 345, 652, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Siguiente >", 345, 661, 110, AURA_ALIGN_CENTER, 0x00);
}

static void render_quotes_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db) {
    int card_w = SCREEN_WIDTH - 32;
    int x = 16;

    aura_ui_draw_header(fb, font_title, "C I T A S");

    /* 3 Filter buttons */
    const char *q_filters[3] = { "Todas", "Mis Citas", "Celebres" };
    int fw = 180;
    for (int i = 0; i < 3; i++) {
        aura_fb_draw_rounded_rect(fb, x + i * (fw + 14), 56, fw, 34, 12, (i == 0) ? 2 : 1, 0xFF, 0x00);
        aura_font_draw_aligned(fb, font_text, q_filters[i], x + i * (fw + 14), 64, fw, AURA_ALIGN_CENTER, 0x00);
    }

    /* 3 Generous Quote Cards */
    int card_y = 106;
    int q_height = 135;
    int count = aura_db_get_quote_count(db);
    int display_count = (count > 3) ? 3 : count;

    for (int i = 0; i < display_count; i++) {
        aura_quote_t *q = aura_db_get_quote(db, i);
        int y = card_y + i * (q_height + 12);
        aura_ui_draw_quote_row(fb, font_text, q, x, y, card_w, q_height);
    }

    /* Pagination controls */
    aura_fb_draw_rounded_rect(fb, 145, 560, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "< Anterior", 145, 569, 110, AURA_ALIGN_CENTER, 0x00);

    aura_font_draw_aligned(fb, font_text, "Pag. 1 de 2", 260, 569, 80, AURA_ALIGN_CENTER, 0x00);

    aura_fb_draw_rounded_rect(fb, 345, 560, 110, 36, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "Siguiente >", 345, 569, 110, AURA_ALIGN_CENTER, 0x00);
}

static void render_current_view(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, aura_db_t *db, aura_ui_navbar_t *navbar) {
    aura_fb_clear(fb, 0xFF); /* Clear screen to paper white */

    if (navbar->active_tab == AURA_TAB_HOME) {
        render_home_view(fb, font_title, font_text, db);
    } else if (navbar->active_tab == AURA_TAB_LIBRARY) {
        render_library_view(fb, font_title, font_text, db);
    } else {
        render_quotes_view(fb, font_title, font_text, db);
    }

    aura_ui_draw_navbar(fb, font_text, navbar);
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    printf("====================================================\n");
    printf("   AuraBooks Native Embedded OS v0.1 (Kindle Touch)  \n");
    printf("====================================================\n");

    /* 1. Initialize Framebuffer */
    aura_fb_t *fb = aura_fb_init("/dev/fb0");
    if (!fb) {
        fprintf(stderr, "[main] Fatal: Unable to initialize framebuffer.\n");
        return 1;
    }

    /* 2. Initialize Touch Input */
    aura_touch_t *touch = aura_touch_init("/dev/input/event0");

    /* 3. Initialize Fonts (Title: size 22, Text: size 14) */
    aura_font_t *font_title = aura_font_load("/mnt/us/fonts/Literata-Bold.ttf", 22);
    aura_font_t *font_text = aura_font_load("/mnt/us/fonts/Literata-Regular.ttf", 14);

    /* 4. Initialize Database & Storage */
    aura_db_t *db = aura_db_init("/mnt/us/aurabooks_data.bin");

    /* 5. Initialize Navigation */
    aura_ui_navbar_t navbar;
    memset(&navbar, 0, sizeof(navbar));
    navbar.active_tab = AURA_TAB_HOME;

    /* First frame: render complete view and trigger clean hardware refresh */
    render_current_view(fb, font_title, font_text, db, &navbar);
    aura_fb_flush_full(fb, AURA_REFRESH_FULL);
    printf("[main] Initial screen drawn and flashed via EPDC GC16.\n");

    aura_touch_event_t ev;
    while (g_running) {
        if (touch && aura_touch_poll(touch, &ev)) {
            aura_tab_t new_tab;
            if (aura_ui_navbar_handle_touch(&navbar, ev, &new_tab)) {
                printf("[main] Tab switched to: %d\n", new_tab);
                render_current_view(fb, font_title, font_text, db, &navbar);
                aura_fb_flush_full(fb, AURA_REFRESH_FAST);
            }
        }

        /* 50 Hz polling rate: 20ms sleep yields CPU, saves battery */
        usleep(20000);
    }

    printf("[main] Shutting down AuraBooks Native cleanly...\n");
    if (touch) aura_touch_close(touch);
    if (db) aura_db_close(db);
    if (font_title) aura_font_close(font_title);
    if (font_text) aura_font_close(font_text);
    aura_fb_close(fb);

    return 0;
}
