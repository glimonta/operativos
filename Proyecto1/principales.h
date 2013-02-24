#ifndef PRINCIPALES_H
#define PRINCIPALES_H

enum parametros
  { P_NUMENTEROS = 1
  , P_NUMNIVELES
  , P_ARCHIVODEENTEROSDESORDENADOS
  , P_ARCHIVODEENTEROSORDENADOS
  }
;

struct configuracion {
  int numEnteros;
  int numNiveles;
  char const * archivoDesordenado;
  char const * archivoOrdenado;
};

extern struct configuracion configuracion;

void configurar(int argc, char * argv[]);

#endif
