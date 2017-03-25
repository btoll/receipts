#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "receipts.h"

// TODO: Catch signals like Ctrl-C, Ctrl-D.
// TODO: Should date be timestamp or just yyyy-mm-dd string?
// TODO: sqlite3 finalize() calls.

void add_receipt(sqlite3 *db) {
    char *sql = "SELECT id, store, street, city, state FROM stores";
    char *err_msg = 0;

    if (has_rows(db, "stores")) {
        sqlite3_stmt *stmt;

        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

        printf("\nKnown stores:\n\n");

        if (rc == SQLITE_OK) {
            int ncols = sqlite3_column_count(stmt);
            int i;

            for (i = 0; i < ncols; ++i)
                printf("\t%s", sqlite3_column_name(stmt, i));

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                int i;

                printf("\n");

                for (i = 0; i < ncols; ++i)
                    printf("\t%s", sqlite3_column_text(stmt, i));
            }

            printf("\n");

            char store_id[3];
            char total_cost[VALUE_MAX];
            char *year = (char *) malloc(5);
            char *month = (char *) malloc(3);
            char *day = (char *) malloc(3);

            required("\n\tSelect store id: ", store_id);

            // Get the items and only enter in the db after the receipt after the receipt has been entered
            // so we can get its receipt_id.
            char *items[ROWS][COLS];
            int nrows = get_receipt_items(items, 0);

            required("\tTotal cost (no dollar sign): ", total_cost);
            required("\tMonth of purchase (MM): ", month);
            required("\tDay of purchase (DD): ", day);
            required("\tYear of purchase (YYYY): ", year);

            char *date = year;
            strncat(date, "-", 1);
            strncat(date, month, 2);
            strncat(date, "-", 1);
            strncat(date, day, 2);
            date[10] = '\0';

            sql = "INSERT INTO receipts VALUES(NULL, cast(? as int), cast(? as real), cast(strftime('%s', ?) as int));";
            rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

            if (rc == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, store_id, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, total_cost, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, date, -1, SQLITE_STATIC);

                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                sql = "INSERT INTO receipts VALUES(NULL, cast(? as int), ?, cast(? as real));";
                rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

                if (rc == SQLITE_OK) {
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);

                    int last_insert_id = sqlite3_last_insert_rowid(db);

                    for (i = 0; i < nrows; ++i) {
                        sql = "INSERT INTO items VALUES(NULL, ?, ?, cast(? as real), cast(? as real));";
                        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

                        if (rc == SQLITE_OK) {
                            sqlite3_bind_int(stmt, 1, last_insert_id);
                            sqlite3_bind_text(stmt, 2, items[i][0], -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt, 3, items[i][1], -1, SQLITE_STATIC);
                            sqlite3_bind_text(stmt, 4, items[i][2], -1, SQLITE_STATIC);

                            sqlite3_step(stmt);
                            sqlite3_finalize(stmt);
                        } else
                            // TODO
                            fprintf(stderr, "[ERROR] Bad shit happened: %s.\n", sqlite3_errmsg(db));
                    }
                } else
                    sqlite3_finalize(stmt);

                clear();

                printf("\n[SUCCESS] Added receipt and items.\n\n");
            } else
                fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));

            free(year);
            free(month);
            free(day);
        } else {
            sqlite3_finalize(stmt);
            fprintf(stderr, "Could not select data: %s\n", sqlite3_errmsg(db));
        }
    } else
        fprintf(stderr, "\n\tThere are no stores. There must be at least one store before a receipt can be entered.\n\n");

    sqlite3_free(err_msg);
}

void add_store(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char store[VALUE_MAX], street[VALUE_MAX], city[VALUE_MAX], state[VALUE_MAX], zip[VALUE_MAX], phone[VALUE_MAX];

    required("\n\tStore name: ", store);

    printf("\tStreet (optional): ");
    strip_newline(fgets(street, VALUE_MAX, stdin), '\0');

    required("\tCity: ", city);
    required("\tState (2 letters): ", state);

    printf("\tZip (optional): ");
    strip_newline(fgets(zip, VALUE_MAX, stdin), '\0');

    printf("\tPhone (optional): ");
    strip_newline(fgets(phone, VALUE_MAX, stdin), '\0');

    char *sql = "INSERT INTO stores VALUES(NULL, ?, ?, ?, ?, ?, ?);";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, store, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, street, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, city, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, state, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 5, zip, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 6, phone, -1, SQLITE_STATIC);

        sqlite3_step(stmt);

        clear();
        printf("\n[SUCCESS] Added store.\n\n");
    } else
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));

    sqlite3_finalize(stmt);
}

void *build_cols(char *s, char *q) {
    int i, len, d;
    // Note that to do a proper store name (INNER JOIN) lookup that "receipts.store_id" needs to be mapped to "stores.store".
    char *cols[] = { "receipts.id, ", "stores.store, ", "receipts.item, ", "receipts.total_cost, ", "date(receipts.date, 'unixepoch'), ", 0 };

    if (s[0] == '*')
        for (i = 0; cols[i]; ++i)
            strncat(q, cols[i], strlen(cols[i]));
    else
        for (i = 0, len = strlen(s); i < len; ++i)
            if (IS_DIGIT(s[i])) {
                d = s[i] - '0';
                strncat(q, cols[d], strlen(cols[d]));
            }

    // Is there an easier way to "remove" the last comma in the string?
    char *last = strrchr(q, ',');
    *last = '\0';

    return 0;
}

void clear(void) {
    // Clear the screen.
    // http://stackoverflow.com/questions/2347770/how-do-you-clear-console-screen-in-c#7660837
    printf("\e[1;1H\e[2J");
}

sqlite3 *get_db(void) {
    sqlite3 *db;

    if (!fopen("./.receipts.db", "r")) {
        fprintf(stderr, "[ERROR] No database, run `make install`.\n");
        exit(1);
    }

    int rc = sqlite3_open("./.receipts.db", &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    return db;
}

int get_receipt_items(char *buf[][COLS], int n) {
    char *name = (char *) malloc(VALUE_MAX);
    char *cost = (char *) malloc(VALUE_MAX);
    char *quantity = (char *) malloc(VALUE_MAX);

    printf("\tItem name: ");
    strip_newline(fgets(name, VALUE_MAX, stdin), '\0');

    if (name[0] != '\0') {
        printf("\tItem cost: ");
        strip_newline(fgets(cost, VALUE_MAX, stdin), '\0');

        printf("\tItem quantity: ");
        strip_newline(fgets(quantity, VALUE_MAX, stdin), '\0');

        buf[n][0] = name;
        buf[n][1] = cost;
        buf[n][2] = quantity;

        n = get_receipt_items(buf, ++n);
    } else
        buf[n][0] = buf[n][1] = buf[n][2] = NULL;

    return n;
}

int has_rows(sqlite3 *db, char *table) {
    sqlite3_stmt *stmt;
    char count[QUERY_MAX] = "SELECT COUNT(*) AS records FROM ";
    int rc;

    strncat(count, table, strlen(table) + 1);
    rc = sqlite3_prepare_v2(db, count, -1, &stmt, 0);

    if (rc == SQLITE_OK) {
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW) {
            const unsigned char *text = sqlite3_column_text(stmt, 0);

            if (text[0] != '0')
                return 1;
        }
    } else
        fprintf(stderr, "Could not select data: %s\n", sqlite3_errmsg(db));

    sqlite3_finalize(stmt);

    return 0;
}

void query(sqlite3 *db) {
    clear();

    if (has_rows(db, "receipts")) {
        sqlite3_stmt *stmt;
        char sql[QUERY_MAX] = "SELECT * FROM receipts";
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

        if (rc == SQLITE_OK) {
            sqlite3_finalize(stmt);

            int fn(void *_, int argc, char **argv, char **col_name) {
                int i;

                for (i = 0; i < argc; i++)
                    printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");

                printf("\n");

                return 0;
            }

            int i, ncols = sqlite3_column_count(stmt);
            char cols[QUERY_MAX], query[QUERY_MAX] = "SELECT ";
            char query_end[] = " FROM receipts INNER JOIN stores ON receipts.store_id = stores.id;";

            printf("Select one or more comma-delimited columns, or * for all.\n");
            printf("\nColumns:\n\n");

            for (i = 0; i < ncols; ++i)
                printf("\t%d %s\n", i, sqlite3_column_name(stmt, i));

            printf("\nSelect columns: ");
            strip_newline(fgets(cols, QUERY_MAX, stdin), '\0');
            build_cols(cols, query);

            // Include the terminating null byte.
            strncat(query, query_end, strlen(query_end) + 1);

            printf("\n%s\n\n", query);

            rc = sqlite3_prepare_v2(db, query, -1, &stmt, 0);

            if (rc == SQLITE_OK) {
                int ncols = sqlite3_column_count(stmt);
                int i;

                for (i = 0; i < ncols; ++i)
                    printf("\t%s", sqlite3_column_name(stmt, i));

                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                    int i;

                    printf("\n");

                    for (i = 0; i < ncols; ++i)
                        printf("\t%s", sqlite3_column_text(stmt, i));

                    printf("\n\n");
                }
            }

            sqlite3_finalize(stmt);
        } else {
            sqlite3_finalize(stmt);
            printf("[WARN] The query could not be performed.\n");
        }
    } else
        printf("[WARN] There is no data. No data, no queries.\n\n");
}

void required(char *field, char *buf) {
    printf(field);
    fgets(buf, VALUE_MAX, stdin);

    if (strlen(buf) == 1) {                         // Only contains newline.
        fprintf(stderr, "\t\tCannot be blank\n");
        required(field, buf);
    }

    strip_newline(buf, '\0');
}

void strip_newline(char *buf, char swap) {
    buf[strcspn(buf, "\n")] = swap;
}

int main(void) {
    char buf[4];
    int n;
    sqlite3 *db = get_db();

    clear();

    do {
        printf(
            "What do you want to do?\n\n"
            "\t1. New receipt\n"
            "\t2. New store\n"
            "\t3. Query\n"
            "\t4. Exit\n"
        );

        required("\nSelect: ", buf);
        // Convert to digit. (Is there a better way?)
        n = *buf - '0';

        if (n == 1)
            add_receipt(db);
        else if (n == 2)
            add_store(db);
        else if (n == 3)
            query(db);
        else if (n == 4) {
            printf("Goodbye.\n");
            n = -1;
        } else {
            printf("Unrecognized selection %d.\nGoodbye.\n", n);
            n = -1;
        }
    } while (n != -1);

    sqlite3_close(db);

    return 0;
}

