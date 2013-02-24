#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <time.h>

#include "ordenArchivo.h"
#include "principales.h"

int * ordenados;
int * desordenados;

#if DEVELOPMENT
  void lock(pthread_mutex_t * mutex) {
    int s = pthread_mutex_lock(mutex);
    if (s != 0) {
      fprintf(stderr, "Error intentando entrar en una sección crítica; pthread_mutex_unlock: ");
      errno = s;
      perror(NULL);
      exit(EX_SOFTWARE);
    }
  }

  void unlock(pthread_mutex_t * mutex) {
    int s = pthread_mutex_unlock(mutex);
    if (s != 0) {
      fprintf(stderr, "Error intentando salir de una sección crítica; pthread_mutex_unlock: ");
      errno = s;
      perror(NULL);
      exit(EX_SOFTWARE);
    }
  }

  pthread_mutex_t mutexSalida = PTHREAD_MUTEX_INITIALIZER;

  void thread_fprintf(struct datos_nodo * datos_nodo, FILE * file, char const * format, ...) {
    va_list arguments;
    va_start(arguments, format);
    lock(&mutexSalida);
    {
      int i;
      fprintf(file, "El hilo {%d, %d, %d} dice: ", datos_nodo->inicio, datos_nodo->fin, datos_nodo->nivel);
      vfprintf(file, format, arguments);
      for (i = datos_nodo->inicio; i < datos_nodo->fin; ++i) {
        fprintf(file, "%d ", desordenados[i]);
      }
      fprintf(file, "\n");
    }
    unlock(&mutexSalida);
    va_end(arguments);
  }
#endif



void * hoja(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "hoja desordenada; ");
#endif

  quicksort(desordenados, datos_nodo->inicio, datos_nodo->fin - 1);

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "hoja ordenada; ");
#endif

  return NULL;
}



void * nodo(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;
  int mitad = datos_nodo->inicio + (datos_nodo->fin - datos_nodo->inicio) / 2;
  void * (*funcionHijos)(void * ) = configuracion.numNiveles - 1 == datos_nodo->nivel ? &hoja : &nodo;

  pthread_t izq;
  pthread_t der;

  struct datos_nodo datos_izq = {datos_nodo->inicio, mitad          , datos_nodo->nivel + 1, datos_nodo->id * 2 - 1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad             , datos_nodo->fin, datos_nodo->nivel + 1, datos_nodo->id * 2    };

#if SEQUENTIAL
  funcionHijos(&datos_izq);
#else
  if (0 != pthread_create(&izq, NULL, funcionHijos, &datos_izq)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }
#endif

#if SEQUENTIAL
  funcionHijos(&datos_der);
#else
  if (0 != pthread_create(&der, NULL, funcionHijos, &datos_der)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }
#endif

#if !SEQUENTIAL
  if (0 != pthread_join(izq, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }

  if (0 != pthread_join(der, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }
#endif

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "nodo desmergeado; mitad: %d; ", mitad);
#endif

  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);
  merge(desordenados, ordenados, datos_nodo->inicio, mitad, datos_nodo->fin);
  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  printf
    ( "Tiempo del hilo %d del nivel %d: %lf μs\n"
    , datos_nodo->id
    , datos_nodo->nivel
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "nodo mergeado; mitad: %d; ", mitad);
#endif

  return NULL;
}



void * escritura(FILE * archivo, void * datos) {
  struct configuracion * configuracion = (struct configuracion *)datos;
  int i;
  for (i = 0; i < configuracion->numEnteros; ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
}



void * principal(FILE * archivo, void * datos) {
  desordenados = (int *)ALLOC(configuracion.numEnteros, sizeof(int));
  ordenados    = (int *)ALLOC(configuracion.numEnteros, sizeof(int));

  if (NULL == desordenados || NULL == ordenados) {
    fprintf(stderr,"Error de reserva de memoria");
    exit(EX_OSERR);
  }

  pthread_t raiz;
  struct datos_nodo datos_raiz = {0, configuracion.numEnteros, 1, 1};

  if (configuracion.numEnteros != fread(desordenados, sizeof(int), configuracion.numEnteros, archivo)) {
    perror("fread");
    exit(EX_IOERR);
  }

  void * (*funcionHijos)(void * ) = configuracion.numNiveles == 1 ? &hoja : &nodo;

  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);

#if SEQUENTIAL
  funcionHijos(&datos_raiz);
#else
  if (0 != pthread_create(&raiz, NULL, funcionHijos, &datos_raiz)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }

  if (0 != pthread_join(raiz, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }
#endif

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
  configurar(argc, argv);

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  apertura(NULL          , M_LECTURA  , configuracion.archivoDesordenado, principal);
  apertura(&configuracion, M_ESCRITURA, configuracion.archivoOrdenado   , escritura);
  free(desordenados);

  return 0;
}
