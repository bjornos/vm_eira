CC=gcc
CFLAGS=-c -Wall -g

all: vm_eira asm2bin

asm2bin: asm2bin.o
	$(CC) asm2bin.o -o asm2bin

vm_eira: main.o display.o utils.o
	$(CC) main.o display.o utils.o -o vm_eira

main.o: main.c
	$(CC) $(CFLAGS) main.c

asm2bin.o: asm2bin.c
	$(CC) $(CFLAGS) asm2bin.c

display.o: display.c
	$(CC) $(CFLAGS) display.c

utils.o: utils.c
	$(CC) $(CFLAGS) utils.c

clean:
	rm -fv *.o vm_eira asm2bin

