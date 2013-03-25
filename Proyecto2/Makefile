.PHONY: all clean
COPTS = -Wall -Werror -ggdb -D_GNU_SOURCE

ejecutables = fssh prueba

all: $(ejecutables)
clean: ; rm -f $(ejecutables) ./*.o

%.o: %.c ; $(CC) $(COPTS) -c $<
$(executables): %: ; $(CC) $(COPTS) -o $@ $^

prueba: actores.o prueba.o
fssh  : actores.o main.o

 prueba.o: prueba.c actores.h
   main.o:   main.c actores.h
