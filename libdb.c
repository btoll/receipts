#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include "receipts.h"

// TODO: For now, this file is just a dumping ground of everything not in receipts.c.

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

