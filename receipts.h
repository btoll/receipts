#define COLS 50
#define ROWS 50
#define QUERY_MAX 150
#define VALUE_MAX 50
#define IS_DIGIT(N) ((N) >= '0' && (N) <= '9')

void add_receipt(sqlite3 *db);
void add_store(sqlite3 *db);
void *build_cols(char *s, char *q);
void clear(void);
sqlite3 *get_db(void);
int get_receipt_items(char *buf[ROWS][COLS], int n);
int has_rows(sqlite3 *db, char *table);
void query(sqlite3 *db);
void required(char *field, char *buf);
void strip_newline(char *buf, char swap);

