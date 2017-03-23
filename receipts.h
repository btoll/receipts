void add_receipt(sqlite3 *db);
void add_store(sqlite3 *db);
void *build_cols(char *s, char *q);
void clear(void);
sqlite3 *get_db(void);
int has_rows(sqlite3 *db, char *table);
void query(sqlite3 *db);
void required(char *field, char *buf);
void strip_newline(char *buf);

#define QUERY_MAX 150
#define VALUE_MAX 50
#define IS_DIGIT(N) ((N) >= '0' && (N) <= '9')

