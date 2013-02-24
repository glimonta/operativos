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



void * hoja(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;

  quicksort(desordenados, datos_nodo->inicio, datos_nodo->fin - 1);

  return NULL;
}



void * nodo(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;
  int mitad = datos_nodo->inicio + (datos_nodo->fin - datos_nodo->inicio) / 2;
  void * (*funcionHijos)(void * ) = configuracion.numNiveles - 1 == datos_nodo->nivel ? &hoja : &nodo;

  struct datos_nodo datos_izq = {datos_nodo->inicio, mitad          , datos_nodo->nivel + 1, datos_nodo->id * 2 - 1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad             , datos_nodo->fin, datos_nodo->nivel + 1, datos_nodo->id * 2    };

  // TODO
  if (0 != child_create(&izq, NULL, funcionHijos, &datos_izq)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }

  if (0 != child_create(&der, NULL, funcionHijos, &datos_der)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }

  child_wait();
  child_wait();

  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);
  merge(datos_nodo->inicio, mitad, datos_nodo->fin);
  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  printf
    ( "Tiempo del proceso %d del nivel %d: %lf μs\n"
    , datos_nodo->id
    , datos_nodo->nivel
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;

  return NULL;
}



void * escritura(FILE * archivo, void * datos) {
  struct configuracion * configuracion = (struct configuracion *) datos;
  int i;
  for (i = 0; i < configuracion->numEnteros; ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
}



void * principal() {
  struct datos_nodo datos_raiz = {0, configuracion.numEnteros, 1, 1};

  enum hijo funcionHijos = configuracion.numNiveles == 1 ? H_HOJA : H_NODO;

  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);

  // FIXME :(
  child_create(funcionHijos, &datos_raiz);
  child_wait();

  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  printf
    ( "Tiempo total de ejecución: %lf μs\n"
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;

  free(ordenados);
}



int main(int argc, char * argv[]) {
  configurar(&configuracion, argc, argv);

  configuracion.archivoDesordenado = argv[P_ARCHIVODEENTEROSDESORDENADOS];
  configuracion.archivoOrdenado    = argv[P_ARCHIVOSDEENTEROSORDENADOS  ];

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  principal();
  apertura(&configuracion, M_ESCRITURA, configuracion.archivoOrdenado   , escritura);
  free(desordenados);

  return 0;
}
