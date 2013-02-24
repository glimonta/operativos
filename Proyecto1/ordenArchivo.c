/**
 * @file ordenArchivo.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de las funciones que son comunes
 * a ambos tipos de ordenamientos, el que utiliza hilos y el
 * que utiliza procesos.
 *
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "ordenArchivo.h"


/**
 * Se encarga de transformar el tipo enumerado modo
 * a un string que indica el modo en que se abre un
 * archivo.
 * @param modo Indica el modo en que queremos escribir
 * el archivo (lectura/escritura)
 * @return Retorna el string que indica el modo (r/w+)
 */
char const * modoArchivo(enum modo modo) {
  switch (modo) {
    case M_LECTURA  : return "r" ;
    case M_ESCRITURA: return "w+";
    default         : return NULL;
  }
}

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
void * apertura(void * datos, enum modo const modo, const char * nombre, void * (*funcion)(FILE *, void *)) {
  // Abrimos el archivo para lectura.
  FILE * archivo = fopen(nombre, modoArchivo(modo));
  // Si el archivo retorna NULL es porque hubo un error en fopen
  // se detiene la ejecucion del programa.
  if (NULL == archivo) {
    fprintf(stderr, "fopen: %s: ", nombre);
    perror("");
    exit(EX_IOERR);
  }

  // Llamamos a la funcion que va a trabajar con el archivo.
  void * retorno = funcion(archivo, datos);

  // Cerramos el archivo.
  fclose(archivo);
  // Retornamos lo que devuelve la ejecucion de la función.
  return retorno;
}



/**
 * Se encarga de hacer merge de un arreglo cuyas mitades estan ordenadas
 * @param desordenados Arreglo que contiene los elementos a los cuales
 * haremos merge.
 * @param ordenados arreglo temporal que usaremos para ordenar los elementos.
 * @param inicio Entero que indica la posicion inicial del arreglo.
 * @param mitad Entero que indica la posicion media del arreglo.
 * @oaram fin Entero que indica la posicion final del arreglo.
 */
void merge(int * desordenados, int * ordenados, int const inicio, int const mitad, int const fin) {
  int i = inicio; // Posicion del arreglo ordenados.
  int l = inicio; // Posicion de la primera mitad de desordenados
  int r = mitad;  // Posicion de la segunda mitad de desordenados

  /* Mientras nos mantengamos dentro de los limites del arreglo
   * se asigna a ordenados[i] el menor entre desordenados[l] y
   * desordenados[r].
   */
  while (l < mitad && r < fin) {
    ordenados[i++]
      = desordenados[l] < desordenados[r]
      ? desordenados[l++]
      : desordenados[r++];
      ;
  }

  /* Si la primera mitad del arreglo aun no acababa de recorrerse
   * entonces se copian los enteros restantes al arreglo de ordenados
   * lo mismo se hace cuando la segunda mitad del arreglo no habia
   * terminado de recorrerse.
   */
  while (l < mitad) ordenados[i++] = desordenados[l++];
  while (r < fin  ) ordenados[i++] = desordenados[r++];

  // Se copia en desordenados el arreglo ordenado que se encuentra en ordenados.
  memcpy(&desordenados[inicio], &ordenados[inicio], (fin - inicio) * sizeof(int));
}



/**
 * Se encarga de cambiar dos elementos de posicion en un arreglo
 * @param a Arreglo donde se va a hacer el swapping
 * @param i Posicion 1.
 * @param j Posicion 2.
 */
void swap(int * a, int i, int j) {
   int t = a[i]; // Almacenamos en un temporal el elemento de la posicion i.
   a[i] = a[j];  // Copiamos el elemento de la posicion j en la posicion i.
   a[j] = t;     // Copiamos en la posicion j el elemento de la posicion i.
}

/**
 * Selecciona el ultimo elemento del arreglo como pivote y posiciona
 * todos los elementos menores a el a su izquierda y los mayores a
 * la derecha.
 * @param a Arreglo que se va a particionar.
 * @param ini Posicion inicial del arreglo.
 * @param fin Posicion final del arreglo.
 * @return Retorna la posicion del pivote.
 */
int particion(int * a, int ini, int fin) {
  // Seleccionamos un pivote
  int pivo = a[fin];
  int i, j;
  // Ordenamos los elementos con respecto al pivote
  // poniendo todos los menores a el a su izquierda
  for(i = ini - 1, j = ini; j <= fin - 1; ++j) {
    if (a[j] <= pivo) {
      ++i;
      swap(a, i, j);
    }
  }
  ++i;
  // Hacemos swap del pivote con el elemento i, asi quedan
  // los menores al pivote a su izquierda y los mayores a
  // la derecha
  swap(a, i, fin);
  // Retorna la posicion del pivote.
  return i;
}

/**
 * Se encarga de hacer un quicksort sobre el arreglo a.
 * @oaram a Arreglo que vamos a ordenar.
 * @param ini Posicion inicial del arreglo.
 * @param fin Posicion final del arreglo.
 */
void quicksort(int * a, int ini, int fin) {
  // Si las posiciones del arreglo no son validas, retorna.
  if (ini >= fin) return;
  // Particiona el arreglo q
  int q = particion(a, ini, fin);
  // Se llama recursivamente a si misma para ordenar desde
  // el inicio al pivote y del pivote al final.
  quicksort(a, ini, q - 1);
  quicksort(a, q + 1, fin);
}
