#ifndef HIJOS_H
#define HIJOS_H

enum parametros
  { P_INICIO = 1
  , P_FIN
  , P_NIVEL
  , P_ID
  , P_NUMNIVELES
  , P_ARCHIVODEENTEROSDESORDENADOS
  }
;

struct configuracion {
  int inicio;
  int fin;
  int nivel;
  int id;
  int numNiveles;
  char * archivoDesordenado;
};

extern struct configuracion configuracion;

void configurar(int argc, char * argv[]);

#endif
