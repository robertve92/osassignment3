#
#	Makefile for 2INC0 Interprocess Communication
#
#	(c) Fontys 2010, Joris Geurts
#

BINARIES = prodcons

CC = gcc
CFLAGS = -g -c -std=c99
LDLIBS = -lpthread

all:	$(BINARIES)

clean:
	rm -f *.o $(BINARIES)

prodcons: prodcons.o

prodcons.o: prodcons.c prodcons.h
