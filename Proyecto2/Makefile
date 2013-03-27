SHELL = bash

.PHONY: all clean test
COPTS = -Wall -Werror -ggdb -D_GNU_SOURCE -pthread

ejecutables = fssh
pruebas = prueba

all: $(ejecutables)
clean: ; rm -f $(ejecutables) $(pruebas) ./*.o
test: $(pruebas)
	diff <(./prueba) <(seq 0 10) || { echo 'TEST FAILED: prueba'; false; }

%.o: %.c ; $(CC) $(COPTS) -c $<
$(ejecutables): %: ; $(CC) $(COPTS) -o $@ $^

prueba: actores.o prueba.o
fssh  : actores.o main.o

 prueba.o: prueba.c actores.h
   main.o:   main.c actores.h
