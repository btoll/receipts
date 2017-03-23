#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#define MAX 50

// TODO: Catch signals like Ctrl-C, Ctrl-D.

void clear(void);
int has_rows(sqlite3 *db);
void required(char *field, char *buf);
void strip_newline(char *buf);

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

            char store_id[3];
            char item[MAX], amount[MAX];
            char *year = (char *) malloc(5);
            char *month = (char *) malloc(3);
            char *day = (char *) malloc(3);

            required("\n\tSelect store: ", store_id);
            required("\tItem: ", item);
            required("\tAmount (without dollar sign): ", amount);
            required("\tMonth of purchase (MM): ", month);
            required("\tDay of purchase (DD): ", day);
            required("\tYear of purchase (YYYY): ", year);

            char *date = year;
            strncat(date, "-", 1);
            strncat(date, month, 2);
            strncat(date, "-", 1);
            strncat(date, day, 2);
            date[10] = '\0';

            char *sql = "INSERT INTO items VALUES(NULL, cast(? as int), ?, ?, cast(strftime('%s', ?) as int));";
            int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, 0);

            if (rc == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1,store_id, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, item, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 3, amount, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 4, date, -1, SQLITE_STATIC);

                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                clear();
                printf("\n[SUCCESS] Added receipt.\n\n");
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
    sqlite3_stmt *stmt;
    char store[MAX], street[MAX], city[MAX], state[MAX], zip[MAX], phone[MAX];

    required("\n\tStore name: ", store);

    printf("\tStreet (optional): ");
    fgets(street, MAX, stdin);
    strip_newline(street);

    required("\tCity: ", city);
    required("\tState (i.e., MA): ", state);

    printf("\tZip (optional): ");
    fgets(zip, MAX, stdin);
    strip_newline(zip);

    printf("\tPhone (optional): ");
    fgets(phone, MAX, stdin);
    strip_newline(phone);

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
        sqlite3_finalize(stmt);

        clear();
        printf("\n[SUCCESS] Added store.\n\n");
    } else
        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
}

void clear(void) {
    // Clear the screen.
    // http://stackoverflow.com/questions/2347770/how-do-you-clear-console-screen-in-c#7660837
    printf("\e[1;1H\e[2J");
}

sqlite3 *get_db() {
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

void query(void) {
    clear();
    printf("TODO\n");
}

void required(char *field, char *buf) {
    printf(field, buf);
    fgets(buf, MAX, stdin);

    if (strlen(buf) == 1) {                         // Only contains newline.
        fprintf(stderr, "\t\tCannot be blank\n");
        required(field, buf);
    }

    strip_newline(buf);
}

void strip_newline(char *buf) {
    buf[strcspn(buf, "\n")] = '\0';
}

int main(void) {
    char buf[4];
    int n;
    sqlite3 *db = get_db();

    clear();

    do {
        printf("What do you want to do?\n\n");
        printf("\t1. New receipt\n");
        printf("\t2. New store\n");
        printf("\t3. Query\n");
        printf("\t4. Exit\n");

        required("\nSelect: ", buf);
        // Convert to digit. (Is there a better way?)
        n = *buf - '0';

        if (n == 1)
            add_receipt(db);
        else if (n == 2)
            add_store(db);
        else if (n == 3)
            query();
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

