/**
 * @file ordenArchivo-t.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion del programa
 * utilizando hilos. El programa recibe una
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

// Creamos dos arreglos, uno para almacenar la secuencia
// desordenada y otro para la secuencia ordenada.
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


/**
 * Se encarga de hacer quicksort con su parte del arreglo
 * de desordenados y almacenar los datos ordenados en el
 * arreglo de ordenados
 * @param datos Son los datos del nodo que necesita para
 * saber como va a ejecutarse.
 * @return retorna NULL
 */
void * hoja(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "hoja desordenada; ");
#endif

  // Hacemos quicksort del arreglo desordenados desde inicio hasta
  // fin - 1 (ambos indicados en datos_nodo.
  quicksort(desordenados, datos_nodo->inicio, datos_nodo->fin - 1);

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "hoja ordenada; ");
#endif

  return NULL;
}


/**
 * Se encarga de crear hilos y luego de que estos
 * terminen su ejecucion hace merge con las partes
 * del arreglo ordenado que devuelve cada uno de los
 * hilos creados.
 * @param datos Datos del nodo que necesita para saber
 * como va a ejecutarse
 * @return Retorna NULL
 */
void * nodo(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;
  int mitad = datos_nodo->inicio + (datos_nodo->fin - datos_nodo->inicio) / 2;
  void * (*funcionHijos)(void * ) = configuracion.numNiveles - 1 == datos_nodo->nivel ? &hoja : &nodo;

  pthread_t izq;
  pthread_t der;

  // Creamos los datos del hilo izquierdo y del hilo derecho
  // indicandole inicio, mitad fin y un id.
  struct datos_nodo datos_izq = {datos_nodo->inicio, mitad          , datos_nodo->nivel + 1, datos_nodo->id * 2 - 1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad             , datos_nodo->fin, datos_nodo->nivel + 1, datos_nodo->id * 2    };

#if SEQUENTIAL
  funcionHijos(&datos_izq);
#else
  // Creamos el hilo izquierdo. Si hay un error imprimimos un mensaje.
  if (0 != pthread_create(&izq, NULL, funcionHijos, &datos_izq)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }
#endif

#if SEQUENTIAL
  funcionHijos(&datos_der);
#else
  // Creamos el hilo derecho. Si hay un error imprimimos un mensaje.
  if (0 != pthread_create(&der, NULL, funcionHijos, &datos_der)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }
#endif

#if !SEQUENTIAL
  // Esperamos a que el hijo izquierdo haga join con el principal.
  // Si hay un error se imprime un mensaje.
  if (0 != pthread_join(izq, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }

  // Esperamos a que el hijo izquierdo haga join con el principal.
  // Si hay un error se imprime un mensaje.
  if (0 != pthread_join(der, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }
#endif

#if DEVELOPMENT
  thread_fprintf(datos_nodo, stderr, "nodo desmergeado; mitad: %d; ", mitad);
#endif

  // Creamos variables para medir el tiempo.
  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  // Comenzamos a medir el tiempo cuando cuando empieza a hacer su trabajo.
  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);
  // El nodo se encarga de hacer merge de los arreglos que ordenaron sus
  // hilos izquierdo y derecho.
  merge(desordenados, ordenados, datos_nodo->inicio, mitad, datos_nodo->fin);
  // Terminamos de medir el tiempo.
  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  // Imprimimos el tiempo que se tarda en hacer su trabajo el hilo.
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


/**
 * Se encarga de escribir en el archivo de salida el arreglo ya ordenado.
 * @param archivo Archivo de salida donde vamos a escribir el arreglo.
 * @param datos Configuracion que se utiliza para saber el numero de enteros
 * que vamos a escribir en el archivo.
 * @return Retorna NULL
 */
void * escritura(FILE * archivo, void * datos) {
  struct configuracion * configuracion = (struct configuracion *)datos;
  int i;
  // Ciclo para imprimir en el archivo cada uno de los elementos del arreglo.
  for (i = 0; i < configuracion->numEnteros; ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
  return NULL;
}


/**
 * Se encarga de crear la raiz del arbol de hilos, tambien mide el tiempo
 * total que tarda en ordenar los enteros del archivo y leer del archivo
 * binario los enteros y guardarlos en el arreglo de desordenados.
 */
void * principal(FILE * archivo, void * datos) {
  // Pedimos espacio dinamico para el arreglo de desordenados y ordenados
  desordenados = (int *)ALLOC(configuracion.numEnteros, sizeof(int));
  ordenados    = (int *)ALLOC(configuracion.numEnteros, sizeof(int));

  // Si hay un error, imprimimos un mensaje.
  if (NULL == desordenados || NULL == ordenados) {
    fprintf(stderr,"Error de reserva de memoria");
    exit(EX_OSERR);
  }

  pthread_t raiz;
  // Creamos el datos_nodo para la raiz
  struct datos_nodo datos_raiz = {0, configuracion.numEnteros, 1, 1};

  // Si no logra leer completo el archivo de enteros hay un error e
  // imprimimos un mensaje.
  if ((size_t)configuracion.numEnteros != fread(desordenados, sizeof(int), configuracion.numEnteros, archivo)) {
    perror("fread");
    exit(EX_IOERR);
  }

  // Si estamos en el penultimo nivel creamos hijos hojas, y si no
  // creamos nodos.
  void * (*funcionHijos)(void * ) = configuracion.numNiveles == 1 ? &hoja : &nodo;

  // Creamos variables para medir el tiempo.
  struct timespec tiempoInicio;
  struct timespec tiempoFinal;

  // Comenzamos a medir el tiempo cuando cuando empieza a hacer su trabajo.
  clock_gettime(CLOCK_MONOTONIC, &tiempoInicio);

#if SEQUENTIAL
  funcionHijos(&datos_raiz);
#else
  // Creamos la raiz del arbol.
  if (0 != pthread_create(&raiz, NULL, funcionHijos, &datos_raiz)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }

  // Esperamos a que el hilo raiz haga join con el principal.
  // Si hay un error, imprimimos un mesnaje.
  if (0 != pthread_join(raiz, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }
#endif

  // Terminamos de medir el tiempo.
  clock_gettime(CLOCK_MONOTONIC, &tiempoFinal);

  // Imprimimos el tiempo total de ejecucion.
  printf
    ( "Tiempo total de ejecución: %lf μs\n"
    , difftime(tiempoFinal.tv_sec, tiempoInicio.tv_sec)*1000000
    + (tiempoFinal.tv_nsec - tiempoInicio.tv_nsec)/10e3
    )
  ;

  // Liberamos el espacio reservado para ordenados.
  free(ordenados);

  return NULL;
}


/**
 * Es el main del programa, se encarga de llamar a apertura dos veces
 * una para leer y trabajar con los datos y otra para escribir en el archivo.
 */
int main(int argc, char * argv[]) {
  configurar(argc, argv);

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  apertura(NULL          , M_LECTURA  , configuracion.archivoDesordenado, principal);
  // Abrimos el archivo para escritura y le pasamos la
  // funcion correspondiente que escribe en el archivo.
  apertura(&configuracion, M_ESCRITURA, configuracion.archivoOrdenado   , escritura);
  // Liberamos el espacio del arreglo de desordenados.
  free(desordenados);

  return 0;
}
