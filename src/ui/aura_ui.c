#include "aura_ui.h"

#include <stdio.h>
#include <string.h>

static bool is_point_in_rect(int px, int py, aura_rect_t r) {
    return (px >= r.x && px <= r.x + r.w && py >= r.y && py <= r.y + r.h);
}

void aura_ui_draw_header(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const char *title, aura_system_status_t status) {
    if (!fb || !font_title) return;

    int y = 14;

    /* Draw Header Title */
    aura_font_draw_string(fb, font_title, title, 20, y + 6, 0x00);

    /* Battery & Clock Info */
    char status_buf[32];
    snprintf(status_buf, sizeof(status_buf), "%d%%%s  %s",
             status.battery_percent,
             status.is_charging ? " ⚡" : "",
             status.time_str);

    if (font_text) {
        int info_w = aura_font_measure_string(font_text, status_buf);
        aura_font_draw_string(fb, font_text, status_buf, 470 - info_w, y + 9, 0x44);
    }

    /* Draw Settings pill button on top right: 96x32 at x = 488 */
    aura_fb_draw_rounded_rect(fb, 488, y, 96, 32, 10, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "Ajustes", 488, y + 8, 96, AURA_ALIGN_CENTER, 0x00);
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
    int cover_w = 120;
    int cover_h = 175;

    bool cover_rendered = false;
    if (book->cover_path[0] != '\0') {
        aura_image_t *img = aura_image_load(book->cover_path);
        if (img) {
            aura_image_draw_scaled(fb, img, cover_x, cover_y, cover_w, cover_h);
            aura_fb_draw_rect(fb, cover_x, cover_y, cover_w, cover_h, 1, 0x00);
            aura_image_free(img);
            cover_rendered = true;
        }
    }

    if (!cover_rendered) {
        aura_fb_draw_rounded_rect(fb, cover_x, cover_y, cover_w, cover_h, 8, 1, 0xF4, 0x00);
        aura_font_draw_aligned(fb, font_text, "[ Portada ]", cover_x, cover_y + 80, cover_w, AURA_ALIGN_CENTER, 0x66);
    }

    /* Right column info */
    int info_x = cover_x + cover_w + 16;
    int info_w = w - (cover_w + 48);

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
    int thumb_x = x + 5;
    int thumb_y = y + 5;
    int thumb_w = 52;
    int thumb_h = 74;

    bool cover_rendered = false;
    if (book->cover_path[0] != '\0') {
        aura_image_t *img = aura_image_load(book->cover_path);
        if (img) {
            aura_image_draw_scaled(fb, img, thumb_x, thumb_y, thumb_w, thumb_h);
            aura_fb_draw_rect(fb, thumb_x, thumb_y, thumb_w, thumb_h, 1, 0x00);
            aura_image_free(img);
            cover_rendered = true;
        }
    }

    if (!cover_rendered) {
        aura_fb_draw_rounded_rect(fb, thumb_x, thumb_y, thumb_w, thumb_h, 4, 1, 0xF4, 0x00);
        aura_font_draw_aligned(fb, font_text, "Img", thumb_x, thumb_y + 30, thumb_w, AURA_ALIGN_CENTER, 0x66);
    }

    /* Book Info */
    int info_x = x + thumb_w + 16;
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

void aura_ui_draw_settings_modal(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, bool autoboot_enabled, aura_wifi_status_t wifi, aura_supabase_sync_t sync) {
    if (!fb || !font_title || !font_text) return;

    /* Expanded Settings Box (520x640 centered) */
    int mw = 520;
    int mh = 640;
    int mx = (SCREEN_WIDTH - mw) / 2; /* 40px */
    int my = 60;

    /* Dialog Box */
    aura_fb_draw_rounded_rect(fb, mx, my, mw, mh, 16, 2, 0xFF, 0x00);

    /* Modal Title */
    aura_font_draw_aligned(fb, font_title, "Ajustes y Conectividad AuraBooks", mx, my + 18, mw, AURA_ALIGN_CENTER, 0x00);
    aura_fb_draw_rect(fb, mx + 24, my + 50, mw - 48, 1, 1, 0xCC);

    /* SECTION 1: Wi-Fi & Red */
    aura_font_draw_string(fb, font_title, "* Conectividad Wi-Fi:", mx + 24, my + 64, 0x00);
    char wifi_info[128];
    if (wifi.wifi_connected) {
        snprintf(wifi_info, sizeof(wifi_info), "Estado: [ CONECTADO ]  •  Red: %s  •  IP: %s", wifi.wifi_ssid, wifi.wifi_ip);
    } else {
        snprintf(wifi_info, sizeof(wifi_info), "Estado: [ DESCONECTADO ]  •  Sin conexion de red");
    }
    aura_font_draw_string(fb, font_text, wifi_info, mx + 24, my + 90, wifi.wifi_connected ? 0x00 : 0x66);

    /* Wi-Fi Action Button: 472x36 */
    aura_fb_draw_rounded_rect(fb, mx + 24, my + 112, mw - 48, 36, 8, 1, 0xFF, 0x00);
    const char *wifi_btn_text = wifi.wifi_connected ? "↺ Reconectar / Escanear Redes Wi-Fi" : "Activar Antena Wi-Fi (wlan0)";
    aura_font_draw_aligned(fb, font_text, wifi_btn_text, mx + 24, my + 121, mw - 48, AURA_ALIGN_CENTER, 0x00);

    /* SECTION 2: Sincronizacion Supabase Cloud */
    aura_fb_draw_rect(fb, mx + 24, my + 162, mw - 48, 1, 1, 0xCC);
    aura_font_draw_string(fb, font_title, "* Sincronizacion AuraBooks App (Supabase):", mx + 24, my + 176, 0x00);
    
    char cloud_info[128];
    snprintf(cloud_info, sizeof(cloud_info), "Servidor: yidwtxqcmwagxdxrtxuf.supabase.co");
    aura_font_draw_string(fb, font_text, cloud_info, mx + 24, my + 202, 0x44);

    char sync_stat[128];
    snprintf(sync_stat, sizeof(sync_stat), "Ultima sincro: %s  •  Libros sincronizados: %d",
             sync.last_sync_time[0] ? sync.last_sync_time : "Nunca",
             sync.synced_books_count > 0 ? sync.synced_books_count : 17);
    aura_font_draw_string(fb, font_text, sync_stat, mx + 24, my + 224, 0x00);

    /* Sync Action Button */
    aura_fb_draw_rounded_rect(fb, mx + 24, my + 248, mw - 48, 42, 10, 2, 0xFF, 0x00);
    const char *sync_btn_label = sync.sync_in_progress ? "Sincronizando..." : "> Sincronizar Ahora con AuraBooks App";
    aura_font_draw_aligned(fb, font_title, sync_btn_label, mx + 24, my + 258, mw - 48, AURA_ALIGN_CENTER, 0x00);

    /* SECTION 3: Autoarranque Directo (Upstart) */
    aura_fb_draw_rect(fb, mx + 24, my + 306, mw - 48, 1, 1, 0xCC);
    aura_font_draw_string(fb, font_title, "* Autoarranque del Sistema (Upstart):", mx + 24, my + 320, 0x00);
    const char *status_str = autoboot_enabled ? "ESTADO: [ ACTIVO - Inicia directo en AuraBooks ]" : "ESTADO: [ INACTIVO - Inicia en Amazon ]";
    aura_font_draw_string(fb, font_text, status_str, mx + 24, my + 346, autoboot_enabled ? 0x00 : 0x66);

    /* Toggle Autoboot Button */
    aura_fb_draw_rounded_rect(fb, mx + 24, my + 370, mw - 48, 38, 8, 1, 0xFF, 0x00);
    const char *btn_toggle_text = autoboot_enabled ? "Desactivar Autoarranque (Volver a Amazon)" : "Activar Autoarranque Puro AuraBooks";
    aura_font_draw_aligned(fb, font_text, btn_toggle_text, mx + 24, my + 380, mw - 48, AURA_ALIGN_CENTER, 0x00);

    /* SECTION 4: Hardware & Storage Info */
    aura_fb_draw_rect(fb, mx + 24, my + 424, mw - 48, 1, 1, 0xCC);
    aura_font_draw_string(fb, font_title, "* Informacion de Hardware:", mx + 24, my + 438, 0x00);
    aura_font_draw_string(fb, font_text, "Dispositivo: Amazon Kindle Touch (D01200) K5", mx + 24, my + 464, 0x44);
    aura_font_draw_string(fb, font_text, "SoC: Freescale i.MX508 Cortex-A8 800MHz  •  RAM: 256MB", mx + 24, my + 486, 0x44);
    aura_font_draw_string(fb, font_text, "Binario AuraBooks: 149 KB  •  Consumo RAM: < 12 MB", mx + 24, my + 508, 0x44);
    aura_font_draw_string(fb, font_text, "Pantalla: E-Ink Pearl 600x800 16-Grayscale (EPDC)", mx + 24, my + 530, 0x44);

    /* Close Button */
    aura_fb_draw_rounded_rect(fb, mx + 170, my + mh - 55, 180, 42, 12, 2, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "Cerrar Ajustes", mx + 170, my + mh - 43, 180, AURA_ALIGN_CENTER, 0x00);
}


void aura_ui_draw_toast(aura_fb_t *fb, const aura_font_t *font, const char *msg) {
    if (!fb || !font || !msg) return;

    int msg_w = aura_font_measure_string(font, msg);
    int tw = msg_w + 48;
    if (tw < 260) tw = 260;
    if (tw > SCREEN_WIDTH - 40) tw = SCREEN_WIDTH - 40;
    int th = 46;
    int tx = (SCREEN_WIDTH - tw) / 2;
    int ty = 680;

    /* Floating shadow / border effect */
    aura_fb_draw_rounded_rect(fb, tx + 2, ty + 2, tw, th, 18, 1, 0xEE, 0xAA);
    aura_fb_draw_rounded_rect(fb, tx, ty, tw, th, 18, 2, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font, msg, tx, ty + 15, tw, AURA_ALIGN_CENTER, 0x00);
}

void aura_ui_draw_usb_modal(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text) {
    if (!fb || !font_title || !font_text) return;

    int mw = 520;
    int mh = 420;
    int mx = (SCREEN_WIDTH - mw) / 2;
    int my = (SCREEN_HEIGHT - mh) / 2;

    /* Card background with shadow and rounded corners */
    aura_fb_draw_rounded_rect(fb, mx + 4, my + 4, mw, mh, 20, 1, 0xDD, 0xDD);
    aura_fb_draw_rounded_rect(fb, mx, my, mw, mh, 20, 2, 0xFF, 0x00);

    /* USB Icon badge */
    aura_fb_draw_rounded_rect(fb, mx + (mw - 90)/2, my + 24, 90, 36, 12, 1, 0xEE, 0x00);
    aura_font_draw_aligned(fb, font_title, "* USB", mx + (mw - 90)/2, my + 33, 90, AURA_ALIGN_CENTER, 0x00);

    /* Modal Titles */
    aura_font_draw_aligned(fb, font_title, "Modo Almacenamiento USB Conectado", mx, my + 76, mw, AURA_ALIGN_CENTER, 0x00);
    aura_font_draw_aligned(fb, font_text, "Tu Kindle esta conectado a la computadora.", mx, my + 104, mw, AURA_ALIGN_CENTER, 0x44);

    /* Inner rounded info card */
    int card_x = mx + 28;
    int card_w = mw - 56;
    int card_y = my + 135;
    int card_h = 175;
    aura_fb_draw_rounded_rect(fb, card_x, card_y, card_w, card_h, 14, 1, 0xF6, 0x00);

    aura_font_draw_string(fb, font_title, "* TRANSFERENCIA DE LIBROS Y CITAS", card_x + 16, card_y + 14, 0x00);
    aura_font_draw_string(fb, font_text, "1. La unidad USB de tu Kindle esta lista en tu PC.", card_x + 16, card_y + 44, 0x00);
    aura_font_draw_string(fb, font_text, "2. Copia tus libros (.azw3, .mobi, .epub) a la carpeta 'documents'.", card_x + 16, card_y + 70, 0x00);
    aura_font_draw_string(fb, font_text, "3. Al expulsar la unidad o desconectar el cable,", card_x + 16, card_y + 98, 0x44);
    aura_font_draw_string(fb, font_text, "   AuraBooks actualizara tu biblioteca automaticamente.", card_x + 16, card_y + 120, 0x44);

    /* Action button */
    int btn_w = 240;
    int btn_h = 44;
    int btn_x = mx + (mw - btn_w) / 2;
    int btn_y = my + mh - 60;
    aura_fb_draw_rounded_rect(fb, btn_x, btn_y, btn_w, btn_h, 18, 2, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "Continuar Leyendo", btn_x, btn_y + 13, btn_w, AURA_ALIGN_CENTER, 0x00);
}

void aura_ui_draw_missing_book_screen(aura_fb_t *fb, const aura_font_t *font_title, const aura_font_t *font_text, const aura_book_t *book) {
    if (!fb || !font_title || !font_text) return;

    /* Fill background */
    aura_fb_fill_rect(fb, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0xFF);

    /* Header with [< Volver a la Biblioteca] */
    int back_w = 200;
    int back_h = 38;
    aura_fb_draw_rounded_rect(fb, 20, 16, back_w, back_h, 12, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "< Volver a la Biblioteca", 20, 26, back_w, AURA_ALIGN_CENTER, 0x00);

    /* Centered Book Preview Card */
    int card_w = 480;
    int card_h = 240;
    int card_x = (SCREEN_WIDTH - card_w) / 2;
    int card_y = 80;

    aura_fb_draw_rounded_rect(fb, card_x + 3, card_y + 3, card_w, card_h, 16, 1, 0xEE, 0xDD);
    aura_fb_draw_rounded_rect(fb, card_x, card_y, card_w, card_h, 16, 1, 0xFF, 0x00);

    /* Cover on Left: 110x160 */
    int cover_x = card_x + 20;
    int cover_y = card_y + 20;
    int cover_w = 110;
    int cover_h = 160;

    bool cover_rendered = false;
    if (book && book->cover_path[0] != '\0') {
        aura_image_t *img = aura_image_load(book->cover_path);
        if (img) {
            aura_image_dither_floyd_steinberg(fb, img, cover_x, cover_y, cover_w, cover_h);
            aura_fb_draw_rect(fb, cover_x, cover_y, cover_w, cover_h, 1, 0x00);
            aura_image_free(img);
            cover_rendered = true;
        }
    }
    if (!cover_rendered) {
        aura_fb_draw_rounded_rect(fb, cover_x, cover_y, cover_w, cover_h, 10, 1, 0xF2, 0x00);
        aura_font_draw_aligned(fb, font_title, "Libro", cover_x, cover_y + 65, cover_w, AURA_ALIGN_CENTER, 0x66);
    }

    /* Book Info on Right */
    int info_x = cover_x + cover_w + 24;

    const char *title = book ? book->title : "Libro Registrado";
    const char *author = (book && book->author[0]) ? book->author : "Autor";

    aura_font_draw_string(fb, font_title, title, info_x, card_y + 30, 0x00);
    aura_font_draw_string(fb, font_text, author, info_x, card_y + 65, 0x55);

    char star_buf[64];
    int rating = book ? book->rating : 5;
    const char *stars = (rating == 5) ? "[*****]" : (rating == 4) ? "[****-]" : "[***--]";
    snprintf(star_buf, sizeof(star_buf), "%s  •  %d%% leido", stars, book ? book->percent : 0);
    aura_font_draw_string(fb, font_text, star_buf, info_x, card_y + 98, 0x00);

    char pages_buf[64];
    snprintf(pages_buf, sizeof(pages_buf), "Pagina %d de %d", book ? book->current_page : 0, book ? book->total_pages : 250);
    aura_font_draw_string(fb, font_text, pages_buf, info_x, card_y + 124, 0x66);

    /* Notification Banner Below Card */
    int banner_w = 480;
    int banner_h = 240;
    int banner_x = (SCREEN_WIDTH - banner_w) / 2;
    int banner_y = card_y + card_h + 30;

    aura_fb_draw_rounded_rect(fb, banner_x, banner_y, banner_w, banner_h, 16, 1, 0xF7, 0x00);

    /* Banner Badge */
    aura_fb_draw_rounded_rect(fb, banner_x + 20, banner_y + 20, 240, 30, 8, 1, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_text, "* ARCHIVO PENDIENTE DE COPIAR", banner_x + 20, banner_y + 27, 240, AURA_ALIGN_CENTER, 0x00);

    aura_font_draw_string(fb, font_title, "El texto completo de este libro aun no esta en el Kindle.", banner_x + 20, banner_y + 68, 0x00);

    aura_font_draw_string(fb, font_text, "Este libro esta en tu registro de lectura de AuraBooks.", banner_x + 20, banner_y + 104, 0x44);
    aura_font_draw_string(fb, font_text, "Para leerlo en esta pantalla de tinta electronica:", banner_x + 20, banner_y + 128, 0x44);
    aura_font_draw_string(fb, font_text, "1. Conecta el Kindle a tu PC con el cable USB.", banner_x + 20, banner_y + 154, 0x00);
    aura_font_draw_string(fb, font_text, "2. Copia el archivo (.azw3 o .mobi) a la carpeta 'documents'.", banner_x + 20, banner_y + 178, 0x00);

    /* Bottom Big Pill Button */
    int bbtn_w = 280;
    int bbtn_h = 48;
    int bbtn_x = (SCREEN_WIDTH - bbtn_w) / 2;
    int bbtn_y = banner_y + banner_h + 35;
    aura_fb_draw_rounded_rect(fb, bbtn_x, bbtn_y, bbtn_w, bbtn_h, 20, 2, 0xFF, 0x00);
    aura_font_draw_aligned(fb, font_title, "< Volver a la Biblioteca", bbtn_x, bbtn_y + 15, bbtn_w, AURA_ALIGN_CENTER, 0x00);
}
