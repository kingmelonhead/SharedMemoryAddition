CC=gcc
CFLAGS=-std=c99

all : master bin_adder

master: master.c
	$(CC) -o $@ $^ $(CFLAGS)

bin_adder: bin_adder.c
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm master bin_adder
