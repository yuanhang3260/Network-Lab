CC=gcc
CFLAGS=-Wall -Werror -O2

all: ping test_ping

ping: ping.c
		$(CC) ping.c -o ping

test_ping: test_ping.c
		$(CC) test_ping.c -o test_ping

clean:
	 	rm -rf test_ping ping
