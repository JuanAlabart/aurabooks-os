#ifndef AURA_SYNC_H
#define AURA_SYNC_H

#include <stdbool.h>
#include <stdint.h>
#include "aura_db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool wifi_connected;
    char wifi_ssid[64];
    char wifi_ip[32];
    int signal_percent;
} aura_wifi_status_t;

typedef struct {
    char supabase_url[128];
    char supabase_key[128];
    char last_sync_time[32];
    int synced_books_count;
    bool sync_in_progress;
    char sync_status_msg[128];
} aura_supabase_sync_t;

/* Wi-Fi Control & Diagnostics */
aura_wifi_status_t aura_sync_get_wifi_status(void);
bool               aura_sync_toggle_wifi(bool enable);

/* Supabase REST API Sync Engine */
void aura_sync_init(aura_supabase_sync_t *sync, const char *url, const char *anon_key);
bool aura_sync_pull_from_supabase(aura_supabase_sync_t *sync, aura_db_t *db);
bool aura_sync_push_progress_to_supabase(aura_supabase_sync_t *sync, const char *book_id, int current_page, int total_pages, const char *status);

#ifdef __cplusplus
}
#endif

#endif /* AURA_SYNC_H */
