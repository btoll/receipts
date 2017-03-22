CC      = gcc
LDLIBS  = -lsqlite3
WARN    = -W -Wall

receipts: receipts.o
	$(CC) $(WARN) -o receipts receipts.c $(LDLIBS)

db: db.o
	$(CC) $(WARN) -o db db.c $(LDLIBS)

clean:
	rm db receipts *.o

