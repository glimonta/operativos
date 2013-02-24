.PHONY: all clean

DEVELOPMENT = 0
SEQUENTIAL  = 0

CC     = gcc
COPTS  = -ggdb -D_GNU_SOURCE -DDEVELOPMENT="$(DEVELOPMENT)" -DSEQUENTIAL="$(SEQUENTIAL)" -pthread -Wall -Wextra -Werror -Wno-unused-parameter
LDOPTS = -lm -lrt

build-exec   = $(CC) $(COPTS) -o $@ $^ $(LDOPTS)
build-object = $(CC) $(COPTS) -c $<

executables := $(executables) ordenArchivo-t
executables := $(executables) ordenArchivo-p
executables := $(executables) nodo
executables := $(executables) hoja

all: $(executables)

clean: ; rm -rf $(executables:%="%") ./*.o $$(printf '%s\n' * | grep '^[0-9]\+.txt$$')

ordenArchivo-t: ordenArchivo-t.o principales.o ordenArchivo.o            ; $(build-exec)
ordenArchivo-p: ordenArchivo-p.o procesos.o principales.o ordenArchivo.o ; $(build-exec)
nodo          : nodo.o hijos.o procesos.o ordenArchivo.o                 ; $(build-exec)
hoja          : hoja.o hijos.o procesos.o ordenArchivo.o                 ; $(build-exec)

%.o: %.c ; $(build-object)

ordenArchivo-t.o: ordenArchivo-t.c  principales.h ordenArchivo.h
ordenArchivo-p.o: ordenArchivo-p.c     procesos.h  principales.h ordenArchivo.h
ordenArchivo.o  :   ordenArchivo.c ordenArchivo.h
principales.o   :    principales.c  principales.h
hijos.o         :          hijos.c        hijos.h
procesos.o      :       procesos.c     procesos.h ordenArchivo.h
