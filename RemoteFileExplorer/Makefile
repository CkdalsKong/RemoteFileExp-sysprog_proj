### Makefile for RemoteFileExplorer

.PHONY: clean

CC=gcc
CFLAGS=-Wall -W -g -c
OBJECTS=client.o display.o file_management.o interface.o main.o server.o sorting.o
RM = rm -f

all: RemoteFileExp

RemoteFileExp: $(OBJECTS)
	$(CC) $(OBJECTS) -o RemoteFileExp -lcurses

client.o: client.c client.h
	$(CC) $(CFLAGS) client.c -o client.o

display.o: display.c display.h
	$(CC) $(CFLAGS) display.c -o display.o

file_management.o: file_management.c file_management.h
	$(CC) $(CFLAGS) file_management.c -o file_management.o

interface.o: interface.c interface.h
	$(CC) $(CFLAGS) interface.c -o interface.o

main.o: main.c
	$(CC) $(CFLAGS) main.c -o main.o

server.o: server.c server.h
	$(CC) $(CFLAGS) server.c -o server.o

sorting.o: sorting.c sorting.h
	$(CC) $(CFLAGS) sorting.c -o sorting.o

clean:
	$(RM) *.o RemoteFileExp
