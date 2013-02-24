#ifndef HIJOS_H
#define HIJOS_H

enum parametros
  { PH_INICIO = 1
  , PH_FIN
  , PH_NIVEL
  , PH_ID
  , PH_NUMNIVELES
  , PH_ARCHIVODEENTEROSDESORDENADOS
  }
;

struct configuracion {
  int inicio;
  int fin;
  int nivel;
  int id;
  int numNiveles;
  char const * archivoDesordenado;
};

extern struct configuracion configuracion;

void configurar(int argc, char * argv[]);

#endif
