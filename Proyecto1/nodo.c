/**
 * @file nodo.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de las funciones
 * que son comunes a todos los nodos.
 *
 */
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



int * desordenados; /**<Variable global que mantiene los numeros desordenados*/


/**
 * Estructura que se utiliza para guardar las opciones de lectura
 */
struct opcionesLectura {
  int cantidad; /**< cantidad de enteros a leer*/
  int inicio; /**< posicion de done se inicia la lectura*/
};


/**
 * Se encarga de leer todos los numeros de cierta seccion del
 * archivo y cargarlo en el arreglo de desordenados
 * @param archivo Archivo que contiene los numeros a ordenar
 * @param datos estructura que contiene los parametros para la lectura
 */
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


/**
 * Se encarga de escribir todos los numeros ordenados de cierta seccion
 * del arreglo desordenados
 * @param archivo Archivo en donde se escribiran los numeros ordenados
 * @param datos estructura que contiene los parametros para la escritura
 */
void * escritura(FILE * archivo, void * datos) {
  int i;
  for (i = 0; i < *((int *)datos); ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
  return NULL;
}


/**
 * Es el main del programa, se encarga de crear los hijos ya sean nodos o hojas del arbol
 * se encarga de hacer merge entre los archivos a medida de que se va subiendo. 
 */
int main(int argc, char * argv[]) {
  //se carga la configuracion de los parametros pasados al main
  configurar(argc, argv);
  //se calcula el numero total de enteros
  int numEnteros = configuracion.fin - configuracion.inicio;
  //se reserva el espacio para el arreglo de desordenados
  desordenados = (int *)alloc(numEnteros, sizeof(int));
  //se calcula la mitad
  int mitad = configuracion.inicio + (configuracion.fin - configuracion.inicio) / 2;
  //se calculan y almacenan los datos del hijo tanto izquierdo como el derecho
  struct datos_nodo datos_izq = {configuracion.inicio, mitad            , configuracion.nivel + 1, configuracion.id * 2 - 1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad               , configuracion.fin, configuracion.nivel + 1, configuracion.id * 2    };
  //se determina si el hijo es un nodo o una hoja
  enum hijo funcionHijos = configuracion.numNiveles - 1 == configuracion.nivel ? H_HOJA : H_NODO;
  //se declaran tres variables que contienen los nombres de los archivos que se van a generar
  char * nombreArch;
  char * nombreArchIzq;
  char * nombreArchDer;
  asprintf(&nombreArch   , "%d.txt", getpid());
//en caso de que se quiera correr el proceso de manera secuencial en vez de concurrente, esto es para el debugging
#if SEQUENTIAL
  asprintf(&nombreArchIzq, "%d.txt", child_create(funcionHijos, &datos_izq, configuracion.numNiveles, configuracion.archivoDesordenado));
  child_wait();
  asprintf(&nombreArchDer, "%d.txt", child_create(funcionHijos, &datos_der, configuracion.numNiveles, configuracion.archivoDesordenado));
  child_wait();
#else
  asprintf(&nombreArchIzq, "%d.txt", child_create(funcionHijos, &datos_izq, configuracion.numNiveles, configuracion.archivoDesordenado));
  asprintf(&nombreArchDer, "%d.txt", child_create(funcionHijos, &datos_der, configuracion.numNiveles, configuracion.archivoDesordenado));
  child_wait();
  child_wait();
#endif


  //se declaran las estructuras que almacenan tiempos
  struct timespec tiempoInicio;
  struct timespec tiempoFinal;
  // se obtiene el tiempo antes de hacer las funciones del nodo
  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);
  //variable usada para almacenar las opciones de lectura
  struct opcionesLectura opcionesLectura;
  //se calculan los nuevos datos para la proxima lectura
  opcionesLectura.cantidad = mitad - configuracion.inicio;
  opcionesLectura.inicio   = 0;
  apertura(&opcionesLectura, M_LECTURA, nombreArchIzq, lectura);
  //de nuevo se calculan los datos para la otra lectura
  opcionesLectura.cantidad = configuracion.fin - mitad;
  opcionesLectura.inicio   = mitad - configuracion.inicio;
  apertura(&opcionesLectura, M_LECTURA, nombreArchDer, lectura);

  //se genera un arreglo temporal para poder hacer el merge
  int * ordenados = (int *)alloc(numEnteros, sizeof(int));
  //se llama al merge y se obtiene en desordenados los enteros ya ordenados
  merge(desordenados, ordenados, 0, numEnteros/2, numEnteros);
  //se libera el espacio reservado para ordenados
  free(ordenados);
  //se toma el tiempo despues de las acciones propias del nodo
  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);
  //se imprime el total de tiempo en el que se ejecuta el proceso
  printf
    ( "Tiempo del proceso %d del nivel %d: %lf μs\n"
    , configuracion.id
    , configuracion.nivel
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;
  //se escriben los resultados de la operacion del nodo
  apertura(&numEnteros, M_ESCRITURA, nombreArch, escritura);
  //se libera el espacio reservado para el tiempo
  free(desordenados);

  return 0;
}
