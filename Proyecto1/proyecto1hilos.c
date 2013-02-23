#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>


#if DEVELOPMENT
#  define ALLOC(cuantos, tam) (calloc((cuantos),(tam)))
#else
#  define ALLOC(cuantos, tam) (malloc((cuantos)*(tam)))
#endif



enum parametros
  { P_NUMENTEROS = 1
  , P_NUMNIVELES
  , P_ARCHIVODEENTEROSDESORDENADOS
  , P_ARCHIVOSDEENTEROSORDENADOS
  }
;



enum modo
  { M_LECTURA
  , M_ESCRITURA
  }
;

char const * modoArchivo(enum modo modo) {
  switch (modo) {
    case M_LECTURA  : return "r" ;
    case M_ESCRITURA: return "w+";
  }
}



int * desordenados;
int * ordenados;



struct configuracion {
  int numEnteros;
  int numNiveles;
  char const * archivoDesordenado;
  char const * archivoOrdenado;
} config;



struct datos_nodo {
  int inicio;
  int fin;
  int nivel;
  int id;
};



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



void merge(int const inicio, int const mitad, int const fin) {
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
  void * (*funcionHijos)(void * ) = config.numNiveles - 1 == datos_nodo->nivel ? &hoja : &nodo;

  pthread_t izq;
  pthread_t der;

  struct datos_nodo datos_izq = {datos_nodo->inicio, mitad          , datos_nodo->nivel + 1, datos_nodo->id * 2 -1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad             , datos_nodo->fin, datos_nodo->nivel + 1, datos_nodo->id * 2};

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
  merge(datos_nodo->inicio, mitad, datos_nodo->fin);
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
  struct configuracion * configuracion = (struct configuracion *) datos;
  int i;
  for (i = 0; i < configuracion->numEnteros; ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
}



void * principal(FILE * archivo, void * datos) {
  desordenados = (int *)ALLOC(config.numEnteros, sizeof(int));
  ordenados    = (int *)ALLOC(config.numEnteros, sizeof(int));

  if (NULL == desordenados || NULL == ordenados) {
    fprintf(stderr,"Error de reserva de memoria");
    exit(EX_OSERR);
  }

  pthread_t raiz;
  struct datos_nodo datos_raiz = {0, config.numEnteros, 1, 1};

  if (config.numEnteros != fread(desordenados, sizeof(int), config.numEnteros, archivo)) {
    perror("fread");
    exit(EX_IOERR);
  }

  void * (*funcionHijos)(void * ) = config.numNiveles == 1 ? &hoja : &nodo;

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



void * apertura(void * datos, enum modo const modo, const char * nombre, void * (*funcion)(FILE *, void *)) {
  // Abrimos el archivo para lectura.
  FILE * archivo = fopen(nombre, modoArchivo(modo));
  // Si el archivo retorna NULL es porque hubo un error en fopen
  // se detiene la ejecucion del programa.
  if (NULL == archivo) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  // Llamamos a la funcion que va a trabajar con el archivo.
  void * retorno = funcion(archivo, datos);

  // Cerramos el archivo.
  fclose(archivo);
  // Retornamos lo que devuelve la ejecucion de la función.
  return retorno;
}



int main(int argc, char * argv[]) {
  // Si el numero de argumentos con el que se invoca al programa
  // es incorrecto se imprime un mensaje de error indicandole al
  // usuario como invocar al programa.
  if (5 != argc) {
    fprintf(stderr, "Numero de argumentos incorrecto, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (1 != sscanf(argv[P_NUMENTEROS], "%d", &config.numEnteros)) {
    fprintf(stderr, "La cantidad de enteros debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Revisamos que el numero de enteros sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= config.numEnteros) {
    fprintf(stderr, "La cantidad de enteros debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (1 != sscanf(argv[P_NUMNIVELES], "%d", &config.numNiveles)) {
    fprintf(stderr, "La cantidad de niveles debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Revisamos que el numero de niveles sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= config.numNiveles) {
    fprintf(stderr, "La cantidad de niveles debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int hojas = pow(2, config.numNiveles - 1);
  int nodos = pow(2, config.numNiveles) - 1;

  // Verificamos que el numero de enteros sea mayor al numero
  // de hojas del arbol.
  if (hojas > config.numEnteros) {
    fprintf(stderr, "La cantidad de enteros es menor al numero de hojas del arbol, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  config.archivoDesordenado = argv[P_ARCHIVODEENTEROSDESORDENADOS];
  config.archivoOrdenado    = argv[P_ARCHIVOSDEENTEROSORDENADOS  ];

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  apertura(NULL   , M_LECTURA  , config.archivoDesordenado, principal);
  apertura(&config, M_ESCRITURA, config.archivoOrdenado   , escritura);
  free(desordenados);

  return 0;
}
