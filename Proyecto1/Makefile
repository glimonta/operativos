.PHONY: all clean

DEVELOPMENT=0
SEQUENTIAL=0

CC = gcc
COPTS = -ggdb -DDEVELOPMENT=$(DEVELOPMENT) -DSEQUENTIAL=$(SEQUENTIAL) -pthread
LDOPTS = -lm -lrt

all : proyecto1hilos

proyecto1hilos : proyecto1hilos.o
	$(CC) $(COPTS) -o $@ $^ $(LDOPTS)
proyecto1hilos.o: proyecto1hilos.c
	$(CC) $(COPTS) -c $<

clean :
	rm -rf proyecto1hilos ./*.o
