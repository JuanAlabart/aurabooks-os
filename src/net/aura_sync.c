#include "aura_sync.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_SUPABASE_URL "https://yidwtxqcmwagxdxrtxuf.supabase.co"
#define DEFAULT_SUPABASE_KEY "sb_publishable_rIcJwDeBSc24Q-_0NNMyJA_IK04u_YS"

aura_wifi_status_t aura_sync_get_wifi_status(void) {
    aura_wifi_status_t status;
    memset(&status, 0, sizeof(status));
    status.wifi_connected = false;
    snprintf(status.wifi_ssid, sizeof(status.wifi_ssid), "Desconectado");
    snprintf(status.wifi_ip, sizeof(status.wifi_ip), "0.0.0.0");
    status.signal_percent = 0;

    /* 1. Check wlan0 operstate */
    FILE *fp = fopen("/sys/class/net/wlan0/operstate", "r");
    if (fp) {
        char buf[32];
        if (fgets(buf, sizeof(buf), fp)) {
            if (strstr(buf, "up")) {
                status.wifi_connected = true;
            }
        }
        fclose(fp);
    }

    /* 2. Read IP Address via getifaddrs */
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) != -1) {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
                if (strcmp(ifa->ifa_name, "wlan0") == 0 || (!status.wifi_connected && strcmp(ifa->ifa_name, "eth0") == 0)) {
                    struct sockaddr_in *pAddr = (struct sockaddr_in *)ifa->ifa_addr;
                    snprintf(status.wifi_ip, sizeof(status.wifi_ip), "%s", inet_ntoa(pAddr->sin_addr));
                    status.wifi_connected = true;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }

    /* 3. Extract SSID via iwconfig */
    if (status.wifi_connected) {
        FILE *fp_iw = popen("iwconfig wlan0 2>/dev/null | grep -i 'ESSID'", "r");
        if (fp_iw) {
            char iw_buf[128];
            if (fgets(iw_buf, sizeof(iw_buf), fp_iw)) {
                char *start = strchr(iw_buf, '"');
                if (start) {
                    char *end = strchr(start + 1, '"');
                    if (end) {
                        *end = '\0';
                        snprintf(status.wifi_ssid, sizeof(status.wifi_ssid), "%s", start + 1);
                    }
                }
            }
            pclose(fp_iw);
        }
        if (status.wifi_ssid[0] == '\0' || strcmp(status.wifi_ssid, "Desconectado") == 0) {
            snprintf(status.wifi_ssid, sizeof(status.wifi_ssid), "Wi-Fi Casa");
        }
        status.signal_percent = 85;
    }

    return status;
}

bool aura_sync_toggle_wifi(bool enable) {
    char cmd[128];
    if (enable) {
        snprintf(cmd, sizeof(cmd), "ifconfig wlan0 up; lipc-set-prop com.lab126.wifid enable 1 2>/dev/null");
    } else {
        snprintf(cmd, sizeof(cmd), "lipc-set-prop com.lab126.wifid enable 0 2>/dev/null; ifconfig wlan0 down");
    }
    int res = system(cmd);
    return (res == 0);
}

void aura_sync_init(aura_supabase_sync_t *sync, const char *url, const char *anon_key) {
    if (!sync) return;
    memset(sync, 0, sizeof(aura_supabase_sync_t));

    snprintf(sync->supabase_url, sizeof(sync->supabase_url), "%s", url ? url : DEFAULT_SUPABASE_URL);
    snprintf(sync->supabase_key, sizeof(sync->supabase_key), "%s", anon_key ? anon_key : DEFAULT_SUPABASE_KEY);
    snprintf(sync->last_sync_time, sizeof(sync->last_sync_time), "Nunca");
    snprintf(sync->sync_status_msg, sizeof(sync->sync_status_msg), "Listo para sincronizar");
    sync->synced_books_count = 0;
    sync->sync_in_progress = false;
}

/* Minimal parser for Supabase JSON books string */
static void parse_json_books_into_db(const char *json, aura_db_t *db, int *out_count) {
    if (!json || !db) return;

    int count = 0;
    const char *p = json;

    while ((p = strstr(p, "{\"id\":")) != NULL && count < MAX_BOOKS) {
        const char *obj_end = strchr(p, '}');
        if (!obj_end) break;

        char title[STR_LEN_MED] = {0};
        char author[STR_LEN_MED] = {0};
        int current_page = 0;
        int total_pages = 250;
        int rating = 0;

        /* Extract title */
        const char *t_key = strstr(p, "\"title\":\"");
        if (t_key && t_key < obj_end) {
            t_key += 9;
            const char *t_end = strchr(t_key, '"');
            if (t_end && t_end < obj_end) {
                int len = (int)(t_end - t_key);
                if (len >= STR_LEN_MED) len = STR_LEN_MED - 1;
                strncpy(title, t_key, len);
                title[len] = '\0';
            }
        }

        /* Extract author */
        const char *a_key = strstr(p, "\"author\":\"");
        if (a_key && a_key < obj_end) {
            a_key += 10;
            const char *a_end = strchr(a_key, '"');
            if (a_end && a_end < obj_end) {
                int len = (int)(a_end - a_key);
                if (len >= STR_LEN_MED) len = STR_LEN_MED - 1;
                strncpy(author, a_key, len);
                author[len] = '\0';
            }
        }

        /* Extract current_page */
        const char *cp_key = strstr(p, "\"current_page\":");
        if (cp_key && cp_key < obj_end) {
            current_page = atoi(cp_key + 15);
        }

        /* Extract total_pages */
        const char *tp_key = strstr(p, "\"total_pages\":");
        if (tp_key && tp_key < obj_end) {
            total_pages = atoi(tp_key + 14);
        }

        /* Extract rating */
        const char *r_key = strstr(p, "\"rating\":");
        if (r_key && r_key < obj_end) {
            rating = atoi(r_key + 9);
        }

        if (title[0] != '\0') {
            aura_book_t *b = &db->books[count];
            snprintf(b->title, STR_LEN_MED, "%s", title);
            snprintf(b->author, STR_LEN_MED, "%s", author[0] ? author : "Autor Desconocido");
            snprintf(b->filename, STR_LEN_MED, "%s.mobi", title);
            b->current_page = current_page;
            b->total_pages = total_pages > 0 ? total_pages : 250;
            b->percent = (b->total_pages > 0) ? (int)((float)current_page / b->total_pages * 100) : 0;
            b->rating = (rating > 5) ? 5 : rating;
            count++;
        }

        p = obj_end + 1;
    }

    if (count > 0) {
        db->book_count = count;
        if (out_count) *out_count = count;
        aura_db_save(db);
    }
}

bool aura_sync_pull_from_supabase(aura_supabase_sync_t *sync, aura_db_t *db) {
    if (!sync || !db) return false;

    sync->sync_in_progress = true;
    snprintf(sync->sync_status_msg, sizeof(sync->sync_status_msg), "Conectando con Supabase...");

    char tmp_file[64] = "/tmp/supabase_books.json";
    char cmd[1024];

    snprintf(cmd, sizeof(cmd),
        "curl -s --connect-timeout 8 --max-time 15 -X GET "
        "-H \"apikey: %s\" "
        "-H \"Authorization: Bearer %s\" "
        "\"%s/rest/v1/books?select=id,title,author,current_page,total_pages,status,rating&order=title.asc\" "
        "> %s 2>/dev/null",
        sync->supabase_key, sync->supabase_key, sync->supabase_url, tmp_file
    );

    int ret = system(cmd);
    if (ret != 0) {
        sync->sync_in_progress = false;
        snprintf(sync->sync_status_msg, sizeof(sync->sync_status_msg), "Error de conexion (verificar Wi-Fi)");
        return false;
    }

    FILE *fp = fopen(tmp_file, "r");
    if (!fp) {
        sync->sync_in_progress = false;
        snprintf(sync->sync_status_msg, sizeof(sync->sync_status_msg), "Error al leer respuesta");
        return false;
    }

    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (sz < 5) {
        fclose(fp);
        remove(tmp_file);
        sync->sync_in_progress = false;
        snprintf(sync->sync_status_msg, sizeof(sync->sync_status_msg), "Respuesta vacia");
        return false;
    }

    char *json = (char*)malloc(sz + 1);
    if (!json) {
        fclose(fp);
        remove(tmp_file);
        sync->sync_in_progress = false;
        return false;
    }

    fread(json, 1, sz, fp);
    json[sz] = '\0';
    fclose(fp);
    remove(tmp_file);

    int count = 0;
    parse_json_books_into_db(json, db, &count);
    free(json);

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    if (lt) {
        snprintf(sync->last_sync_time, sizeof(sync->last_sync_time), "%02d:%02d", lt->tm_hour, lt->tm_min);
    } else {
        snprintf(sync->last_sync_time, sizeof(sync->last_sync_time), "Ahora");
    }

    sync->synced_books_count = count;
    sync->sync_in_progress = false;
    snprintf(sync->sync_status_msg, sizeof(sync->sync_status_msg), "Sincronizado: %d libros listos", count);
    return true;
}

bool aura_sync_push_progress_to_supabase(aura_supabase_sync_t *sync, const char *book_id, int current_page, int total_pages, const char *status) {
    if (!sync || !book_id || book_id[0] == '\0') return false;

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "curl -s --connect-timeout 5 --max-time 10 -X PATCH "
        "-H \"apikey: %s\" "
        "-H \"Authorization: Bearer %s\" "
        "-H \"Content-Type: application/json\" "
        "-H \"Prefer: return=minimal\" "
        "-d '{\"current_page\": %d, \"total_pages\": %d, \"status\": \"%s\"}' "
        "\"%s/rest/v1/books?id=eq.%s\" > /dev/null 2>&1 &",
        sync->supabase_key, sync->supabase_key, current_page, total_pages,
        status ? status : "reading",
        sync->supabase_url, book_id
    );

    system(cmd);
    return true;
}
