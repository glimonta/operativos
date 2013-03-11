#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "actores.h"

#define CONTINUA(funcion, datos) { Actor actor = { (funcion), (datos) }; return actor; }
#define TERMINA CONTINUA(NULL, NULL)
#define ENVIAR(direccion, longitud, contenido) { Mensaje mensaje = { longitud, contenido }; enviar(direccion, mensaje); }

Actor funcionCul(Mensaje mensajito, void * datos) {
  int * i = (int *) datos;
  printf("%d\n", *i);
  if (10 == *i) TERMINA;
  ++*i;
  ENVIAR(yo, 0, NULL);
  CONTINUA(funcionCul, datos);
}

int main(int argc, char * argv[]) {
  Actor manuel = { funcionCul, calloc(1, sizeof(int)) };
  Direccion manuelDir = crear(manuel);
  ENVIAR(manuelDir, 0, NULL);
  esperar(manuelDir);
  exit(EX_OK);
}
