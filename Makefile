CC=gcc
CFLAGS=-Wall

all: servidor

servidor: servidor.c
	$(CC) $(CFLAGS) servidor.c -o servidor

run-server:
	./servidor

run-client:
	python3 ./client.py -s localhost -p 4200

clean:
	rm -f servidor
