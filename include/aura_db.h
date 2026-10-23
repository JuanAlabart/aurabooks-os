#ifndef AURA_DB_H
#define AURA_DB_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_BOOKS       128
#define MAX_QUOTES      256
#define STR_LEN_SMALL   64
#define STR_LEN_MED     128
#define STR_LEN_LARGE   512

typedef struct {
    char filename[STR_LEN_MED];
    char title[STR_LEN_MED];
    char author[STR_LEN_MED];
    char cover_path[STR_LEN_MED];
    int current_page;
    int total_pages;
    int percent;
    int rating; /* 0 to 5 */
} aura_book_t;

typedef struct {
    char text[STR_LEN_LARGE];
    char author[STR_LEN_MED];
    char book[STR_LEN_MED];
    bool is_user_highlight;
} aura_quote_t;

typedef struct {
    aura_book_t books[MAX_BOOKS];
    int book_count;
    aura_quote_t quotes[MAX_QUOTES];
    int quote_count;
    int active_book_idx;
    char db_path[STR_LEN_MED];
} aura_db_t;

/* Lifecycle */
aura_db_t* aura_db_init(const char *storage_path);
void       aura_db_close(aura_db_t *db);
bool       aura_db_save(aura_db_t *db);

/* Book Operations */
int         aura_db_get_book_count(const aura_db_t *db);
aura_book_t* aura_db_get_book(aura_db_t *db, int index);
aura_book_t* aura_db_get_active_book(aura_db_t *db);
void        aura_db_set_rating(aura_db_t *db, const char *filename, int rating);
void        aura_db_update_progress(aura_db_t *db, const char *filename, int current_page, int total_pages);

/* Quote Operations */
int          aura_db_get_quote_count(const aura_db_t *db);
aura_quote_t* aura_db_get_quote(aura_db_t *db, int index);
void         aura_db_add_user_quote(aura_db_t *db, const char *text, const char *author, const char *book);

#ifdef __cplusplus
}
#endif

#endif /* AURA_DB_H */
