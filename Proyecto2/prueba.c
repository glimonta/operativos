#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "actores.h"

Actor funcionCul(Mensaje mensajito, void * datos) {
  int * i = (int *)datos;
  printf("%d\n", *i);

  if (10 == *i) return finActor();
  ++*i;

  enviarme(mensajito);
  //Direccion yo = miDireccion();
  //enviar(yo, mensajito);
  //liberaDireccion(yo);

  return mkActor(funcionCul, datos);
}

int main(int argc, char * argv[]) {
  Actor manuel = { funcionCul, calloc(1, sizeof(int)) };
  Direccion manuelDir = crearSinEnlazar(manuel);
  enviar(manuelDir, mkMensaje(0, NULL));
  esperar(manuelDir);
  exit(EX_OK);
}
