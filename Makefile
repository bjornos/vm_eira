CC=gcc
CFLAGS=-c -Wall -g

all: vm_eira

vm_eira: main.o
	$(CC) main.o -o vm_eira

main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm *o vm_eira

