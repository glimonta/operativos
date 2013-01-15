CC = gcc
COPTS = -Wall -D_GNU_SOURCE -ggdb

.PHONY: all clean

all: tarea

clean: ; rm -f tarea.o tarea

tarea: tarea.o; $(CC) $(COPTS) -o $@ $^

tarea.o: tarea.c; $(CC) $(COPTS) -c $<
