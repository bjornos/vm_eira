CC=gcc
CFLAGS=-c -Wall -g
LFLAGS=-lpthread

all: vm_eira asm2bin

asm2bin: asm2bin.o
	$(CC) asm2bin.o -o asm2bin

vm_eira: main.o display.o utils.o
	$(CC) main.o display.o utils.o -o vm_eira $(LFLAGS)

main.o: main.c
	$(CC) $(CFLAGS) main.c $(LFLAGS)

asm2bin.o: asm2bin.c
	$(CC) $(CFLAGS) asm2bin.c $(LFLAGS)

display.o: display.c
	$(CC) $(CFLAGS) display.c $(LFLAGS)

utils.o: utils.c
	$(CC) $(CFLAGS) utils.c $(FLAGS)

clean:
	rm -fv *.o vm_eira asm2bin

