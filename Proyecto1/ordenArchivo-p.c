/**
 * @file ordenArchivo-t.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion del programa
 * utilizando procesos. El programa recibe una
 * secuencia de enteros en un archivo binario
 * y se encarga de crear un arbol de hilos cuyas
 * hojas se encargan de ordenar una parte de la
 * secuencia con quicksort y los nodos se encargan
 * de hacer merge de las secuencias ordenadas de sus
 * dos hijos y finalmente se escribe el resultado
 * ordenado en un archivo de texto.
 *
 */
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


/**
 * Es el main del programa se encarga de desencadenar todos los hilos que deben usarse para ordenar el archivo
 * ya sea haciendo merge o quicksort, tambien se mide el tiempo del hilo principal
 */
int main(int argc, char * argv[]) {
  //se verifica y carga la configuracion de los archivos que se le pasan como parametros al main
  configurar(argc, argv);
  //se determina si se crea una hoja o un nodo
  enum hijo funcionHijos = configuracion.numNiveles == 1 ? H_HOJA : H_NODO;
  struct datos_nodo datos_hijo = {0, configuracion.numEnteros, 1, 1};
  //se declaran las variables que se usan para medir el tiempo
  struct timespec tiempoInicio;
  struct timespec tiempoFinal;
  //se obtiene el tiempo antes de que se ejecute el hilo principal
  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);
  //se carga el nombre del archivo en donde se va a guardar el resultado final y se llama a la creacion del hilo principal
  char * nombreArch;
  asprintf(&nombreArch, "%d.txt", child_create(funcionHijos, &datos_hijo, configuracion.numNiveles, configuracion.archivoDesordenado));
  //se espera a que los hijos de principal terminen
  child_wait();
  //se toma el tiempo en donde el hilo principal termina
  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);
  //se imprime el total de tiempo 
  printf
    ( "Tiempo total: %lf μs\n"
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;
  //se renombra el archivo final con el nombre dado por el usuario
  rename(nombreArch, configuracion.archivoOrdenado);
  //se libera la variable auxiliar en donde se guardo el nombre
  free(nombreArch);

  return 0;
}
