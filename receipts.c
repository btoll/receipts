#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX 50

// TODO: Catch signals like Ctrl-C, Ctrl-D.

int has_rows(sqlite3 *db);
void required(char *field, char *buf);

void add_receipt(sqlite3 *db) {
    char *sql = "SELECT id, name, street, city FROM stores";
    char *err_msg = 0;

    if (has_rows(db)) {
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

            sqlite3_stmt *res;
            int store_id = 0;
            char item[MAX], amount[MAX];
            char *year = (char *) malloc(5);
            char *month = (char *) malloc(3);
            char *day = (char *) malloc(3);

            printf("\n\tSelect store: ");
            scanf("%d%*c", &store_id);

            required("\tItem: ", item);
            item[strcspn(item, "\n")] = '\0';

            required("\tAmount (without dollar sign): ", amount);
            amount[strcspn(amount, "\n")] = '\0';

            required("\tMonth of purchase (MM): ", month);
            month[strcspn(month, "\n")] = '\0';

            required("\tDay of purchase (DD): ", day);
            day[strcspn(day, "\n")] = '\0';

            required("\tYear of purchase (YYYY): ", year);
            year[strcspn(year, "\n")] = '\0';

            char *date = year;
            strncat(date, "-", 1);
            strncat(date, month, 2);
            strncat(date, "-", 1);
            strncat(date, day, 2);
            date[10] = '\0';

            char *sql = "INSERT INTO items VALUES(NULL, ?, ?, ?, cast(strftime('%s', ?) as int));";
            int rc = sqlite3_prepare_v2(db, sql, -1, &res, 0);

            if (rc == SQLITE_OK) {
                sqlite3_bind_int(res, 1, store_id);
                sqlite3_bind_text(res, 2, item, -1, SQLITE_STATIC);
                sqlite3_bind_text(res, 3, amount, -1, SQLITE_STATIC);
                sqlite3_bind_text(res, 4, date, -1, SQLITE_STATIC);

                sqlite3_step(res);
                sqlite3_finalize(res);

                printf("\n\tEntered!\n\n");
            } else
                fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));

            free(year);
            free(month);
            free(day);
        } else
            fprintf(stderr, "Could not select data: %s\n", sqlite3_errmsg(db));
    } else
        fprintf(stderr, "\n\tThere are no stores. There must be at least one store before a receipt can be entered.\n\n");

    sqlite3_free(err_msg);
}

void add_store(sqlite3 *db) {
    sqlite3_stmt *res;
    char store[MAX], street[MAX], city[MAX], state[MAX], zip[MAX], phone[MAX];

    required("\n\tStore name: ", store);
    store[strcspn(store, "\n")] = '\0';

    printf("\tStreet (optional): ");
    fgets(street, MAX, stdin);
    street[strcspn(street, "\n")] = '\0';

    required("\tCity: ", city);
    city[strcspn(city, "\n")] = '\0';

    required("\tState: ", state);
    state[strcspn(state, "\n")] = '\0';

    printf("\tZip (optional): ");
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

        printf("\n\tEntered!\n\n");
    } else
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
}

sqlite3 *get_db() {
    sqlite3 *db;

    if (!fopen("./.receipts.db", "r")) {
        fprintf(stderr, "[ERROR] No database, run `make db`.\n");
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

int has_rows(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char *count = "SELECT COUNT(*) AS records FROM stores";
    int rc = sqlite3_prepare_v2(db, count, -1, &stmt, 0);

    if (rc == SQLITE_OK) {
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW) {
            const unsigned char *text = sqlite3_column_text(stmt, 0);

            if (text[0] != '0')
                return 1;
        }
    } else
        fprintf(stderr, "Could not select data: %s\n", sqlite3_errmsg(db));

    return 0;
}

void required(char *field, char *buf) {
    printf(field, buf);
    fgets(buf, MAX, stdin);

    if (strlen(buf) == 1) {                         // Only contains newline.
        fprintf(stderr, "\t\tCannot be blank\n");
        required(field, buf);
    }
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

