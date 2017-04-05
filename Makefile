CC      = gcc
LDLIBS  = -lsqlite3
WARN    = -W -Wall

.PHONY: all clean install

all: db receipts

db: db.o
	$(CC) $(WARN) -o db db.c $(LDLIBS)

receipts: receipts.o receipts.h
	$(CC) $(WARN) -o receipts receipts.c $(LDLIBS)

clean:
	rm db receipts *.o

install:
	./db

