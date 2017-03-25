#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

int main(void) {
    sqlite3 *db;
    char *err_msg;
    FILE *fp;

    fp = fopen("./.receipts.db", "r");

    if (fp)
        printf("[WARN] Database already exists, will not overwrite.\n");
    else {
        int res = sqlite3_open("./.receipts.db", &db);

        if (res != SQLITE_OK) {
            fprintf(stderr, "Failed to create database: %s\n", sqlite3_errmsg(db));
            sqlite3_close(db);
            exit(1);
        }

        // Is it worth using transactions here?
        char *sql =
            "DROP TABLE IF EXISTS stores;"
            "DROP TABLE IF EXISTS receipts;"
            "DROP TABLE IF EXISTS items;"

            "CREATE TABLE stores (id INTEGER PRIMARY KEY, store TEXT NOT NULL, street TEXT, city TEXT, state TEXT, zip TEXT, phone TEXT);"
            "CREATE TABLE receipts (id INTEGER PRIMARY KEY, store_id INTEGER, total_cost REAL NOT NULL, date INTEGER NOT NULL, FOREIGN KEY(store_id) REFERENCES stores(id));"
            "CREATE TABLE items (id INTEGER PRIMARY KEY, receipt_id INTEGER, item TEXT NOT NULL, cost REAL NOT NULL, quantity REAL NOT NULL, FOREIGN KEY(receipt_id) REFERENCES receipts(id));";

        res = sqlite3_exec(db, sql, 0, 0, &err_msg);

        if (res != SQLITE_OK) {
            fprintf(stderr, "Failed to create database tables: %s\n", err_msg);
            sqlite3_free(err_msg);
            sqlite3_close(db);
            exit(1);
        }

        printf("[SUCCESS] Database created.\n");
    }

    return 0;
}

