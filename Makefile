include ./Makefile.inc

all: main server client

main:
	$(CC) -o main main.c

server:

client:

clean:
	rm -rf src/*.o
