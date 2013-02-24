#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <time.h>

#include "ordenArchivo.h"
#include "principales.h"
#include "procesos.h"



int main(int argc, char * argv[]) {
  configurar(argc, argv);

  enum hijo funcionHijos = configuracion.numNiveles == 1 ? H_HOJA : H_NODO;
  struct datos_nodo datos_hijo = {0, configuracion.numEnteros, 1, 1};

  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);

  char * nombreArch;
  asprintf(&nombreArch, "%d.txt", child_create(funcionHijos, &datos_hijo, configuracion.numNiveles, configuracion.archivoDesordenado));
  child_wait();

  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  printf
    ( "Tiempo total: %lf μs\n"
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;

  rename(nombreArch, configuracion.archivoOrdenado);

  free(nombreArch);

  return 0;
}
