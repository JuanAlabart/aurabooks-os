#include "aura_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void seed_default_data(aura_db_t *db) {
    /* 1. Seed sample books matching Kindle library */
    db->book_count = 5;

    snprintf(db->books[0].filename, STR_LEN_MED, "Crimen y castigo - Fiodor Dostoyevski.azw3");
    snprintf(db->books[0].title, STR_LEN_MED, "Crimen y castigo");
    snprintf(db->books[0].author, STR_LEN_MED, "Fiódor Dostoyevski");
    snprintf(db->books[0].cover_path, STR_LEN_MED, "/mnt/us/koreader/cache/covers/large/Crimen y castigo (trad. F. Oter - Fiodor Dostoyevski.azw3.jpg");
    db->books[0].current_page = 142;
    db->books[0].total_pages = 420;
    db->books[0].percent = 33;
    db->books[0].rating = 5;

    snprintf(db->books[1].filename, STR_LEN_MED, "El extranjero - Albert Camus.azw3");
    snprintf(db->books[1].title, STR_LEN_MED, "El extranjero");
    snprintf(db->books[1].author, STR_LEN_MED, "Albert Camus");
    snprintf(db->books[1].cover_path, STR_LEN_MED, "/mnt/us/koreader/cache/covers/large/El extranjero - Albert Camus.azw3.jpg");
    db->books[1].current_page = 95;
    db->books[1].total_pages = 120;
    db->books[1].percent = 79;
    db->books[1].rating = 5;

    snprintf(db->books[2].filename, STR_LEN_MED, "La metamorfosis.azw");
    snprintf(db->books[2].title, STR_LEN_MED, "La metamorfosis");
    snprintf(db->books[2].author, STR_LEN_MED, "Franz Kafka");
    snprintf(db->books[2].cover_path, STR_LEN_MED, "/mnt/us/koreader/cache/covers/large/La metamorfosis_LQYOPA24R3DGH3OZVDROYP645BFFVYNU.azw.jpg");
    db->books[2].current_page = 0;
    db->books[2].total_pages = 85;
    db->books[2].percent = 0;
    db->books[2].rating = 4;

    snprintf(db->books[3].filename, STR_LEN_MED, "La sombra del viento - Carlos Ruiz Zafon.azw3");
    snprintf(db->books[3].title, STR_LEN_MED, "La sombra del viento");
    snprintf(db->books[3].author, STR_LEN_MED, "Carlos Ruiz Zafón");
    snprintf(db->books[3].cover_path, STR_LEN_MED, "/mnt/us/koreader/cache/covers/large/La sombra del viento - Carlos Ruiz Zafon.azw3.jpg");
    db->books[3].current_page = 0;
    db->books[3].total_pages = 560;
    db->books[3].percent = 0;
    db->books[3].rating = 5;

    snprintf(db->books[4].filename, STR_LEN_MED, "El jugador - Fiodor Dostoyevski.azw3");
    snprintf(db->books[4].title, STR_LEN_MED, "El jugador");
    snprintf(db->books[4].author, STR_LEN_MED, "Fiódor Dostoyevski");
    snprintf(db->books[4].cover_path, STR_LEN_MED, "/mnt/us/koreader/cache/covers/large/El jugador - Fiodor Dostoyevski.azw3.jpg");
    db->books[4].current_page = 0;
    db->books[4].total_pages = 210;
    db->books[4].percent = 0;
    db->books[4].rating = 4;

    db->active_book_idx = 0;

    /* 2. Seed curated literary quotes */
    db->quote_count = 4;

    snprintf(db->quotes[0].text, STR_LEN_LARGE, "El misterio de la existencia humana no radica solo en vivir, sino en encontrar algo por lo que vivir.");
    snprintf(db->quotes[0].author, STR_LEN_MED, "Fiódor Dostoyevski");
    snprintf(db->quotes[0].book, STR_LEN_MED, "Los hermanos Karamázov");
    db->quotes[0].is_user_highlight = false;

    snprintf(db->quotes[1].text, STR_LEN_LARGE, "En las profundidades del invierno finalmente aprendí que dentro de mí habitaba un verano invencible.");
    snprintf(db->quotes[1].author, STR_LEN_MED, "Albert Camus");
    snprintf(db->quotes[1].book, STR_LEN_MED, "El verano");
    db->quotes[1].is_user_highlight = false;

    snprintf(db->quotes[2].text, STR_LEN_LARGE, "A partir de cierto punto no hay retorno. Ese es el punto que hay que alcanzar.");
    snprintf(db->quotes[2].author, STR_LEN_MED, "Franz Kafka");
    snprintf(db->quotes[2].book, STR_LEN_MED, "Aforismos");
    db->quotes[2].is_user_highlight = false;

    snprintf(db->quotes[3].text, STR_LEN_LARGE, "Cada libro tiene un alma. El alma de quien lo escribió y de quienes lo leyeron y soñaron con él.");
    snprintf(db->quotes[3].author, STR_LEN_MED, "Carlos Ruiz Zafón");
    snprintf(db->quotes[3].book, STR_LEN_MED, "La sombra del viento");
    db->quotes[3].is_user_highlight = true;
}

aura_db_t* aura_db_init(const char *storage_path) {
    aura_db_t *db = (aura_db_t*)calloc(1, sizeof(aura_db_t));
    if (!db) return NULL;

    if (!storage_path) {
        storage_path = "/mnt/us/aurabooks_data.bin";
    }
    snprintf(db->db_path, STR_LEN_MED, "%s", storage_path);

    FILE *fp = fopen(db->db_path, "rb");
    if (fp) {
        size_t read_bytes = fread(db, 1, sizeof(aura_db_t), fp);
        fclose(fp);
        if (read_bytes == sizeof(aura_db_t) && db->book_count > 0) {
            printf("[aura_db] Loaded %d books, %d quotes from %s\n", db->book_count, db->quote_count, db->db_path);
            return db;
        }
    }

    /* Seed default data on fresh start */
    seed_default_data(db);
    aura_db_save(db);
    return db;
}

void aura_db_close(aura_db_t *db) {
    if (!db) return;
    aura_db_save(db);
    free(db);
}

bool aura_db_save(aura_db_t *db) {
    if (!db || db->db_path[0] == '\0') return false;

    FILE *fp = fopen(db->db_path, "wb");
    if (!fp) return false;

    size_t written = fwrite(db, 1, sizeof(aura_db_t), fp);
    fclose(fp);
    return (written == sizeof(aura_db_t));
}

int aura_db_get_book_count(const aura_db_t *db) {
    return db ? db->book_count : 0;
}

aura_book_t* aura_db_get_book(aura_db_t *db, int index) {
    if (!db || index < 0 || index >= db->book_count) return NULL;
    return &db->books[index];
}

aura_book_t* aura_db_get_active_book(aura_db_t *db) {
    if (!db || db->book_count == 0) return NULL;
    int idx = db->active_book_idx;
    if (idx < 0 || idx >= db->book_count) idx = 0;
    return &db->books[idx];
}

void aura_db_set_rating(aura_db_t *db, const char *filename, int rating) {
    if (!db || !filename) return;
    for (int i = 0; i < db->book_count; i++) {
        if (strcmp(db->books[i].filename, filename) == 0) {
            db->books[i].rating = rating;
            aura_db_save(db);
            return;
        }
    }
}

void aura_db_update_progress(aura_db_t *db, const char *filename, int current_page, int total_pages) {
    if (!db || !filename) return;
    for (int i = 0; i < db->book_count; i++) {
        if (strcmp(db->books[i].filename, filename) == 0) {
            db->books[i].current_page = current_page;
            db->books[i].total_pages = total_pages;
            db->books[i].percent = (total_pages > 0) ? (int)((float)current_page / total_pages * 100) : 0;
            db->active_book_idx = i;
            aura_db_save(db);
            return;
        }
    }
}

int aura_db_get_quote_count(const aura_db_t *db) {
    return db ? db->quote_count : 0;
}

aura_quote_t* aura_db_get_quote(aura_db_t *db, int index) {
    if (!db || index < 0 || index >= db->quote_count) return NULL;
    return &db->quotes[index];
}

void aura_db_add_user_quote(aura_db_t *db, const char *text, const char *author, const char *book) {
    if (!db || !text || db->quote_count >= MAX_QUOTES) return;

    aura_quote_t *q = &db->quotes[db->quote_count++];
    snprintf(q->text, STR_LEN_LARGE, "%s", text);
    snprintf(q->author, STR_LEN_MED, "%s", author ? author : "Autor");
    snprintf(q->book, STR_LEN_MED, "%s", book ? book : "Libro");
    q->is_user_highlight = true;
    aura_db_save(db);
}
