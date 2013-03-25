#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "actores.h"

Actor funcionCul(Mensaje mensajito, void * datos) {
  int * i = (int *) datos;
  printf("%d\n", *i);
  if (10 == *i) return finActor();
  ++*i;
  enviar(yo, mensajito);
  return mkActor(funcionCul, datos);
}

int main(int argc, char * argv[]) {
  Actor manuel = { funcionCul, calloc(1, sizeof(int)) };
  Direccion manuelDir = crear(manuel);
  enviar(manuelDir, mkMensaje(0, NULL));
  esperar(manuelDir);
  exit(EX_OK);
}
