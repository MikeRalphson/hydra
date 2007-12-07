# makefile for karl's PD fortune cookie program

all: cookie cookhash

cookie: cookie.h
	cc -O -o cookie cookie.c

cookhash:
	cc -O -o cookhash cookhash.c

install:
	cp cookie cookhash /usr/local/bin
