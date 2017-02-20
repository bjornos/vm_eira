CC=gcc
CFLAGS=-c -Wall -g

all: vm_eira

vm_eira: main.o utils.o
	$(CC) main.o utils.o -o vm_eira

main.o: main.c
	$(CC) $(CFLAGS) main.c

utils.o: utils.c
	$(CC) $(CFLAGS) utils.c

clean:
	rm *o vm_eira

