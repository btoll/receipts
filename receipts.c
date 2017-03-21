#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>

#define MAX 50

sqlite3 *get_db() {
    sqlite3 *db;

    if (!fopen("./.receipts.db", "r")) {
        fprintf(stderr, "[ERROR] No database, run `make install`.\n");
        exit(1);
    }

    int res = sqlite3_open("./.receipts.db", &db);

    if (res != SQLITE_OK) {
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    return db;
}

int main(void) {
    int n;
    sqlite3 *db = get_db();

    printf("What do you want to do?\n\n");
    printf("\t1. Enter new receipt\n");
    printf("\t2. Enter new store\n");
    printf("\t3. Other\n");
    printf("\nSelect: ");

    scanf("%d%*c", &n);

    if (n == 2) {
        char store[MAX], street[MAX], city[MAX], state[MAX], zip[MAX], phone[MAX];
        sqlite3_stmt *res;

        printf("Store name: ");
        fgets(store, MAX, stdin);

        printf("Street: ");
        fgets(street, MAX, stdin);

        printf("City: ");
        fgets(city, MAX, stdin);

        printf("State: ");
        fgets(state, MAX, stdin);

        printf("Zip: ");
        fgets(zip, MAX, stdin);

        printf("Phone: ");
        fgets(phone, MAX, stdin);

        char *sql = "INSERT INTO stores VALUES(NULL, ?, ?, ?, ?, ?, ?);";
        int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);

        if (rc != SQLITE_OK)
            fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));

        sqlite3_bind_text(res, 1, store, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 2, street, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 3, city, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 4, state, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 5, zip, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 6, phone, -1, SQLITE_STATIC);

        sqlite3_step(res);
        sqlite3_finalize(res);
        sqlite3_close(db);
    }

    return 0;
}

