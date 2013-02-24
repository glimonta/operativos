#ifndef PROCESOS_H
#define PROCESOS_H

#include "ordenArchivo.h"

enum hijo
  { H_NODO
  , H_HOJA
  }
;

char const * tipoHijo(enum hijo hijo);
pid_t child_create(enum hijo hijo, struct datos_nodo * parametros);
void child_wait();

#endif
