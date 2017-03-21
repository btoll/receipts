receipts.o: receipts.c
	gcc -Wall -o receipts receipts.c -lsqlite3
	gcc -Wall -o create_db create_db.c -lsqlite3

install:
	./create_db

clean:
	rm -rf receipts create_db

