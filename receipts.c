#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX 50

// TODO: Catch signals like Ctrl-C, Ctrl-D.

void add_receipt(sqlite3 *db) {
    int n = 1;
    int cb(void *_, int argc, char **argv, char **col_name) {
        int i;

        for (i = 0; i < argc; ++i)
            printf("\t%d: %s\n", n++, argv[i]);

        return 0;
    }

    char *sql = "SELECT name FROM stores";
    char *err_msg = 0;
    int res;

    printf("\nKnown stores:\n");
    res = sqlite3_exec(db, sql, cb, 0, &err_msg);

    if (res == SQLITE_OK) {
        int n = 0;
        char item[MAX], amount[MAX];
        char month[2], day[2], year[4];

        printf("\n\tSelect store: ");
        scanf("%d%*c", &n);

        printf("\tItem: ");
        fgets(item, MAX, stdin);
        item[strcspn(item, "\n")] = '\0';

        printf("\tAmount (without dollar sign): ");
        fgets(amount, MAX, stdin);
        amount[strcspn(amount, "\n")] = '\0';

        printf("\tMonth of purchase (MM): ");
        fgets(month, MAX, stdin);
        month[strcspn(month, "\n")] = '\0';

        printf("\tDay of purchase (DD): ");
        fgets(day, MAX, stdin);
        day[strcspn(day, "\n")] = '\0';

        printf("\tYear of purchase (YYYY): ");
        fgets(year, MAX, stdin);
        year[strcspn(year, "\n")] = '\0';
    } else
        fprintf(stderr, "Could not select data: %s\n", sqlite3_errmsg(db));

    sqlite3_free(err_msg);
}

void add_store(sqlite3 *db) {
    sqlite3_stmt *res;
    char store[MAX], street[MAX], city[MAX], state[MAX], zip[MAX], phone[MAX];

    printf("\n\tStore name: ");
    fgets(store, MAX, stdin);
    store[strcspn(store, "\n")] = '\0';

    printf("\tStreet (optional): ");
    fgets(street, MAX, stdin);
    street[strcspn(street, "\n")] = '\0';

    printf("\tCity: ");
    fgets(city, MAX, stdin);
    city[strcspn(city, "\n")] = '\0';

    printf("\tState: ");
    fgets(state, MAX, stdin);
    state[strcspn(state, "\n")] = '\0';

    printf("\tZip: ");
    fgets(zip, MAX, stdin);
    zip[strcspn(zip, "\n")] = '\0';

    printf("\tPhone (optional): ");
    fgets(phone, MAX, stdin);
    phone[strcspn(phone, "\n")] = '\0';

    char *sql = "INSERT INTO stores VALUES(NULL, ?, ?, ?, ?, ?, ?);";
    int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);

    if (rc == SQLITE_OK) {
        sqlite3_bind_text(res, 1, store, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 2, street, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 3, city, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 4, state, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 5, zip, -1, SQLITE_STATIC);
        sqlite3_bind_text(res, 6, phone, -1, SQLITE_STATIC);

        sqlite3_step(res);
        sqlite3_finalize(res);
    } else
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
}

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

    do {
        printf("What do you want to do?\n\n");
        printf("\t1. Enter new receipt\n");
        printf("\t2. Enter new store\n");
        printf("\t3. Exit\n");
        printf("\nSelect: ");

        scanf("%d%*c", &n);

        if (n == 1)
            add_receipt(db);
        else if (n == 2)
            add_store(db);
        else if (n == 3) {
            printf("Goodbye.\n");
            n = -1;
        } else {
            printf("Unrecognized selection %d\n", n);
            n = -1;
        }
    } while (n != -1);

    sqlite3_close(db);

    return 0;
}

