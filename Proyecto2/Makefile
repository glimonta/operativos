.PHONY: all clean
COPTS = -Wall -ggdb

ejecutables = prueba

all: $(ejecutables)
clean: ; rm -f $(ejecutables) ./*.o

prueba: actores.o prueba.o
%.o: %.c ; $(CC) $(COPTS) -c $<
$(ejecutables): %: ; $(CC) $(COPTS) -o $@ $^
