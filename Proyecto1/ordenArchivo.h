#ifndef ORDENARCHIVO_H
#define ORDENARCHIVO_H

#include <stdio.h>
#include <stdlib.h>



#if DEVELOPMENT
#  define ALLOC(cuantos, tam) (calloc((cuantos),(tam)))
#else
#  define ALLOC(cuantos, tam) (malloc((cuantos)*(tam)))
#endif



struct datos_nodo {
  int inicio;
  int fin;
  int nivel;
  int id;
};



enum modo
  { M_LECTURA
  , M_ESCRITURA
  }
;

void * apertura
  ( void * datos
  , enum modo const modo
  , const char * nombre
  , void * (*funcion)(FILE *, void *)
  )
;



void merge
  ( int * desordenados
  , int * ordenados
  , int const inicio
  , int const mitad
  , int const fin
  )
;

void quicksort
  ( int * a
  , int ini
  , int fin
  )
;

#endif
