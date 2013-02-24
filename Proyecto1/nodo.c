#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "hijos.h"
#include "ordenArchivo.h"
#include "procesos.h"



int * desordenados;
int * ordenados;



struct opcionesLectura {
  int cantidad;
  int inicio;
};

void * lectura(FILE * archivo, void * datos) {
  struct opcionesLectura opcionesLectura = *((struct opcionesLectura *)datos);
  int i;
  for (i = 0; i < opcionesLectura.cantidad; ++i) {
    if (1 != fscanf(archivo, " %d ", &desordenados[opcionesLectura.inicio + i])) {
      perror("fscanf");
      exit(EX_IOERR);
    }
  }
  return NULL;
}



void * escritura(FILE * archivo, void * datos) {
  int i;
  for (i = 0; i < *((int *)datos); ++i) {
    fprintf(archivo, "%d ", ordenados[i]);
  }
  return NULL;
}



int main(int argc, char * argv[]) {
  configurar(argc, argv);

  int numEnteros = configuracion.fin - configuracion.inicio;

  desordenados = (int *)ALLOC(numEnteros, sizeof(int));

  int mitad = configuracion.inicio + (configuracion.fin - configuracion.inicio) / 2;

  struct datos_nodo datos_izq = {configuracion.inicio, mitad            , configuracion.nivel + 1, configuracion.id * 2 - 1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad               , configuracion.fin, configuracion.nivel + 1, configuracion.id * 2    };

  enum hijo funcionHijos = configuracion.numNiveles - 1 == configuracion.nivel ? H_HOJA : H_NODO;

  char * nombreArch;
  char * nombreArchIzq;
  char * nombreArchDer;
  asprintf(&nombreArch   , "%d.txt", getpid());
  asprintf(&nombreArchIzq, "%d.txt", child_create(funcionHijos, &datos_izq, configuracion.numNiveles, configuracion.archivoDesordenado));
  asprintf(&nombreArchDer, "%d.txt", child_create(funcionHijos, &datos_der, configuracion.numNiveles, configuracion.archivoDesordenado));

  child_wait();
  child_wait();

  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);

  struct opcionesLectura opcionesLectura;

  opcionesLectura.cantidad = mitad - configuracion.inicio;
  opcionesLectura.inicio   = 0;
  apertura(&opcionesLectura, M_LECTURA, nombreArchIzq, lectura);

  opcionesLectura.cantidad = configuracion.fin - mitad;
  opcionesLectura.inicio   = mitad - configuracion.inicio;
  apertura(&opcionesLectura, M_LECTURA, nombreArchDer, lectura);

  ordenados = (int *)ALLOC(numEnteros, sizeof(int));
  merge(desordenados, ordenados, 0, numEnteros/2, numEnteros);
  free(ordenados);

  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  printf
    ( "Tiempo del proceso %d del nivel %d: %lf μs\n"
    , configuracion.id
    , configuracion.nivel
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;

  apertura(&numEnteros, M_ESCRITURA, nombreArch, escritura);
  free(desordenados);

  return 0;
}
