#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

void main(void) {
    sqlite3 *db;
    char *err_msg;

    int res = sqlite3_open("./.receipts.db", &db);

    if (res != SQLITE_OK) {
        fprintf(stderr, "Failed to create database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    char *sql =
        "DROP TABLE IF EXISTS stores;"
        "DROP TABLE IF EXISTS items;"
        "CREATE TABLE stores (id INTEGER PRIMARY KEY, name TEXT, street TEXT, city TEXT, state TEXT, zip TEXT, phone TEXT);"
        "CREATE TABLE items (id INTEGER PRIMARY KEY, item TEXT, amount REAL);";

    res = sqlite3_exec(db, sql, 0, 0, &err_msg);

    if (res != SQLITE_OK) {
        fprintf(stderr, "Failed to create database tables: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(1);
    }
}

