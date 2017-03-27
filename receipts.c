#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "receipts.h"
#include "libdb.c"

// TODO: Catch signals like Ctrl-C, Ctrl-D.
// TODO: sqlite3 finalize() calls.

void *add_cols_to_sql_query(char *s, char *q) {
    int i, len, d;
    // Note that to do a proper store name (INNER JOIN) lookup that "receipts.store_id" needs to be mapped to "stores.store".
//     char *cols[] = { "receipts.id, ", "stores.store, ", "receipts.total_cost, ", "date(receipts.date, 'unixepoch'), ", 0 };
    char *cols[] = { "receipts.id, ", "stores.store AS Store, ", "receipts.total_cost, ", "receipts.date AS Date, ", 0 };

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
                printf("%15s", sqlite3_column_name(stmt, i));

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                int i;

                printf("\n");

                for (i = 0; i < ncols; ++i)
                    printf("%15s", sqlite3_column_text(stmt, i));
            }

            printf("\n");

            char store_id[3], total_cost[VALUE_MAX], year[VALUE_MAX], month[VALUE_MAX], day[VALUE_MAX];

            required("\n\tSelect store id: ", store_id);

            // Get the items and only enter in the db after the receipt after the receipt has been entered
            // so we can get its receipt_id.
            char *items[ROWS][COLS];
            int nrows = get_receipt_items(items, 0);

            required("\tTotal cost: ", total_cost);

            if (total_cost[0] == '$') {
                // TODO: Here we're avoiding using a temp var.  Is this good practice or worth it?
                strncpy(total_cost, &total_cost[1], strlen(total_cost) - 1);
                total_cost[strlen(total_cost) - 1] = '\0';
            }

            required("\tMonth of purchase (mm): ", month);
            required("\tDay of purchase (dd): ", day);
            required("\tYear of purchase (yyyy): ", year);

            // Create date string in the format `yyyy-mm-dd`.
            char *date = year;
            strncat(date, "-", 2);
            strncat(date, month, 3);
            strncat(date, "-", 2);
            strncat(date, day, 3);
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
                    int last_insert_id;

                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);

                    last_insert_id = sqlite3_last_insert_rowid(db);

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
                            fprintf(stderr, "[ERROR] Bad shit happened %s.\n", sqlite3_errmsg(db));
                    }
                } else
                    sqlite3_finalize(stmt);

                clear();

                printf("\n[SUCCESS] Added receipt and items.\n\n");
            } else
                fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
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

void query(sqlite3 *db) {
    clear();

    if (has_rows(db, "receipts")) {
        sqlite3_stmt *stmt;
        char sql[QUERY_MAX] = "SELECT * FROM receipts";
        int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

        if (rc == SQLITE_OK) {
            int fn(void *_, int argc, char **argv, char **col_name) {
                int i;

                for (i = 0; i < argc; i++)
                    printf("%s = %s\n", col_name[i], argv[i] ? argv[i] : "NULL");

                printf("\n");

                return 0;
            }

            int i, ncols = sqlite3_column_count(stmt);
            char cols[QUERY_MAX], q[QUERY_MAX] = "SELECT ";
            char q_end[] = " FROM receipts INNER JOIN stores ON receipts.store_id = stores.id;";

            printf("Select one or more comma-delimited columns, or * for all.\n");
            printf("\nColumns:\n\n");

            for (i = 0; i < ncols; ++i)
                printf("\t%d %s\n", i, sqlite3_column_name(stmt, i));

            printf("\nSelect columns: ");
            strip_newline(fgets(cols, QUERY_MAX, stdin), '\0');
            add_cols_to_sql_query(cols, q);

            // Include the terminating null byte.
            strncat(q, q_end, strlen(q_end) + 1);

            // For debugging, print out query.
            printf("\n%s\n\n", q);

            rc = sqlite3_prepare_v2(db, q, -1, &stmt, 0);

            if (rc == SQLITE_OK) {
                int ncols = sqlite3_column_count(stmt);
                int i;

                for (i = 0; i < ncols; ++i)
                    printf("%15s", sqlite3_column_name(stmt, i));

                while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                    printf("\n");

                    for (i = 0; i < ncols; ++i)
                        printf("%15s", sqlite3_column_text(stmt, i));
                }

                printf("\n\n");

                show_items_menu(db);
            } else {
                fprintf(stderr, "[ERROR] Bad shit happened %s.\n", sqlite3_errmsg(db));
            }

            sqlite3_finalize(stmt);
        } else {
            sqlite3_finalize(stmt);
            printf("[WARN] The query could not be performed.\n");
        }
    } else
        printf("[WARN] There is no data. No data, no queries.\n\n");
}

void show_items_menu(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char buf[5];
    int i;

    printf(
        "More:\n\n"
        "\t1. Show items\n"
        "\t2. New query\n"
        "\t3. Quit query\n"
    );

    required("\nSelect: ", buf);
    // Convert to digit. (Is there a better way?)
    i = *buf - '0';

    if (i == 1) {
        char *sql;
        int rc;

        required("Select receipt id: ", buf);
        // Convert to digit. (Is there a better way?)
        i = *buf - '0';

        sql = "SELECT items.item AS Item, items.cost AS Cost, items.quantity AS Quantity FROM items INNER JOIN receipts ON items.receipt_id = receipts.id WHERE items.receipt_id = ?";
        rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

        if (rc == SQLITE_OK) {
            int ncols;

            sqlite3_bind_int(stmt, 1, i);
            ncols = sqlite3_column_count(stmt);

            printf("\n");

            for (i = 0; i < ncols; ++i)
                printf("\t%s", sqlite3_column_name(stmt, i));

            while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
                printf("\n");

                for (i = 0; i < ncols; ++i)
                    printf("\t%s", sqlite3_column_text(stmt, i));
            }

            sqlite3_finalize(stmt);

            printf("\n\n");
            show_items_menu(db);
        } else
            fprintf(stderr, "[ERROR] Show items query failed %s.\n", sqlite3_errmsg(db));
    } else if (i == 2)
        query(db);
    else
        clear();
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

