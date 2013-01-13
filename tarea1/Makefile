CC = gcc
COPTS = -Wall -D_GNU_SOURCE -ggdb

.PHONY: all clean

all: tarea1

clean: ; rm -f tarea1.o tarea1

tarea1: tarea1.o; $(CC) $(COPTS) -o $@ $^

tarea1.o: tarea1.c; $(CC) $(COPTS) -c $<
