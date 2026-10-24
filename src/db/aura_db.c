#include "aura_db.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

static void seed_default_data(aura_db_t *db) {
    /* Clean initial state: zero hardcoded books.
     * Library is dynamically populated from Supabase sync or documents/ scanning */
    db->book_count = 0;
    db->active_book_idx = 0;

    /* Seed curated literary quotes for Zen/Quotes tab */
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

int aura_db_scan_documents(aura_db_t *db, const char *docs_dir) {
    if (!db) return 0;
    if (!docs_dir) docs_dir = "/mnt/us/documents";

    DIR *dir = opendir(docs_dir);
    if (!dir) return 0;

    int added = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && db->book_count < MAX_BOOKS) {
        if (ent->d_name[0] == '.') continue;

        /* Check for e-book extensions */
        const char *ext = strrchr(ent->d_name, '.');
        if (!ext) continue;
        if (strcasecmp(ext, ".azw3") != 0 &&
            strcasecmp(ext, ".azw") != 0 &&
            strcasecmp(ext, ".mobi") != 0 &&
            strcasecmp(ext, ".epub") != 0 &&
            strcasecmp(ext, ".txt") != 0) {
            continue;
        }

        /* Check if already in db */
        bool exists = false;
        for (int i = 0; i < db->book_count; i++) {
            if (strcmp(db->books[i].filename, ent->d_name) == 0) {
                exists = true;
                break;
            }
        }
        if (exists) continue;

        /* Add new local book */
        aura_book_t *b = &db->books[db->book_count++];
        snprintf(b->filename, STR_LEN_MED, "%s", ent->d_name);

        /* Derive clean title from filename */
        char clean[STR_LEN_MED];
        snprintf(clean, sizeof(clean), "%s", ent->d_name);
        char *dot = strrchr(clean, '.');
        if (dot) *dot = '\0';

        char *sep = strstr(clean, " - ");
        if (sep) {
            *sep = '\0';
            snprintf(b->title, STR_LEN_MED, "%s", clean);
            snprintf(b->author, STR_LEN_MED, "%s", sep + 3);
        } else {
            snprintf(b->title, STR_LEN_MED, "%s", clean);
            snprintf(b->author, STR_LEN_MED, "Autor Local");
        }

        snprintf(b->cover_path, STR_LEN_MED, "%s.jpg", ent->d_name);
        b->current_page = 0;
        b->total_pages = 250;
        b->percent = 0;
        b->rating = 5;
        added++;
    }
    closedir(dir);

    if (added > 0) {
        aura_db_save(db);
    }
    return added;
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
        if (read_bytes == sizeof(aura_db_t) && db->quote_count > 0) {
            printf("[aura_db] Loaded %d books, %d quotes from %s\n", db->book_count, db->quote_count, db->db_path);
            aura_db_scan_documents(db, "/mnt/us/documents");
            return db;
        }
    }

    /* Seed default data on fresh start */
    seed_default_data(db);
    aura_db_scan_documents(db, "/mnt/us/documents");
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
