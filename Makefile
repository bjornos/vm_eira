CC=gcc
CFLAGS=-c -Wall -g

all: vm_eira

vm_eira: main.o display.o utils.o
	$(CC) main.o display.o utils.o -o vm_eira

main.o: main.c
	$(CC) $(CFLAGS) main.c

display.o: display.c
	$(CC) $(CFLAGS) display.c

utils.o: utils.c
	$(CC) $(CFLAGS) utils.c

clean:
	rm -v *.o vm_eira

