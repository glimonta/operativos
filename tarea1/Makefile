#
# @file Makefile
# @author Gabriela Limonta 10-10385
# @author John Delgado 10-10196
#
# @section Grupo 09
#
# Makefile de la tarea de operativos.
#

CC = gcc
COPTS = -Wall -D_GNU_SOURCE -ggdb

.PHONY: all clean

all: tarea

clean: ; rm -f tarea.o arbol.o tarea

tarea: tarea.o arbol.o; $(CC) $(COPTS) -o $@ $^

arbol.o: arbol.c; $(CC) $(COPTS) -c $<

tarea.o: tarea.c; $(CC) $(COPTS) -c $<
