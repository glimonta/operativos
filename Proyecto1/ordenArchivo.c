#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "ordenArchivo.h"



char const * modoArchivo(enum modo modo) {
  switch (modo) {
    case M_LECTURA  : return "r" ;
    case M_ESCRITURA: return "w+";
    default         : return NULL;
  }
}

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



void merge(int * desordenados, int * ordenados, int const inicio, int const mitad, int const fin) {
  int i = inicio;
  int l = inicio;
  int r = mitad;

  while (l < mitad && r < fin) {
    ordenados[i++]
      = desordenados[l] < desordenados[r]
      ? desordenados[l++]
      : desordenados[r++];
      ;
  }

  while (l < mitad) ordenados[i++] = desordenados[l++];
  while (r < fin  ) ordenados[i++] = desordenados[r++];

  memcpy(&desordenados[inicio], &ordenados[inicio], (fin - inicio) * sizeof(int));
}



void swap(int * a, int i, int j) {
  int t = a[i];
  a[i] = a[j];
  a[j] = t;
}

int particion(int * a, int ini, int fin) {
  int pivo = a[fin];
  int i, j;
  for(i = ini - 1, j = ini; j <= fin - 1; ++j) {
    if (a[j] <= pivo) {
      ++i;
      swap(a, i, j);
    }
  }
  ++i;
  swap(a, i, fin);
  return i;
}

void quicksort(int * a, int ini, int fin) {
  if (ini >= fin) return;
  int q = particion(a, ini, fin);
  quicksort(a, ini, q - 1);
  quicksort(a, q + 1, fin);
}
