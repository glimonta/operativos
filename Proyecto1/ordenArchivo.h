/**
 * @file ordenArchivo.h
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 *  Contiene las funciones que son comunes a ambos tipos de ordenamientos,
 * el que utiliza hilos y el que utiliza procesos.
 *
 */
#ifndef ORDENARCHIVO_H
#define ORDENARCHIVO_H

#include <stdio.h>
#include <stdlib.h>


#if DEVELOPMENT
#  define ALLOC(cuantos, tam) (calloc((cuantos),(tam)))
#else
#  define ALLOC(cuantos, tam) (malloc((cuantos)*(tam)))
#endif


/**
 * Estructura que se utiliza para guardar informacion de
 * cada uno de los nodos.
 */
struct datos_nodo {
  int inicio;
  int fin;
  int nivel;
  int id;
};


/**
 * Tipo enumerado que indica el modo de apertura de un archivo
 * si es para lectura o escritura
 */
enum modo
  { M_LECTURA
  , M_ESCRITURA
  }
;

/**
 * Se encarga de abrir un archivo para que una funcion trabaje con el
 * y luego cerrar el archivo
 * @param datos estructura que contiene los datos que se pasaran como
 * parametros a la funcion dada.
 * @param modo Indica el modo en el que se abrira el archivo, bien sea
 * para lectura o para escritura
 * @param nombre Indica el nombre del archivo con el que se trabajara
 * @param funcion Funcion que trabajara con el archivo luego de que
 * apertura se encargue de abrirlo
 * @return Retorna lo que devuelve la ejecucion de la funcion
 */
void * apertura
  ( void * datos
  , enum modo const modo
  , const char * nombre
  , void * (*funcion)(FILE *, void *)
  )
;


/**
 * Se encarga de hacer merge de un arreglo cuyas mitades estan ordenadas
 * @param desordenados Arreglo que contiene los elementos a los cuales
 * haremos merge.
 * @param ordenados arreglo temporal que usaremos para ordenar los elementos.
 * @param inicio Entero que indica la posicion inicial del arreglo.
 * @param mitad Entero que indica la posicion media del arreglo.
 * @oaram fin Entero que indica la posicion final del arreglo.
 */
void merge
  ( int * desordenados
  , int * ordenados
  , int const inicio
  , int const mitad
  , int const fin
  )
;

/**
 * Se encarga de hacer un quicksort sobre el arreglo a.
 * @oaram a Arreglo que vamos a ordenar.
 * @param ini Posicion inicial del arreglo.
 * @param fin Posicion final del arreglo.
 */
void quicksort
  ( int * a
  , int ini
  , int fin
  )
;

#endif
