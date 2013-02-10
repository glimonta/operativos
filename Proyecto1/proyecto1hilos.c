#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#ifdef DEVELOPMENT
#  define ALLOC(cuantos, tam) calloc((cuantos),(tam))
#else
#  define ALLOC(cuantos, tam) malloc((cuantos)*(tam))
#endif

enum parametros
  { P_NUMENTEROS = 1
  , P_NUMNIVELES
  , P_ARCHIVODEENTEROSDESORDENADOS
  , P_ARCHIVOSDEENTEROSORDENADOS
  }
;

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
};

void merge(int const inicio, int const mitad, int const fin) {
  int i;
  int l = inicio;
  int r = mitad + 1;

  for (i = inicio; i < fin; ++i) {
    if (desordenados[l] > desordenados[r]) {
      ordenados[i] = desordenados[l];
      ++l;
    } else {
      ordenados[i] = desordenados[r];
      ++r;
    }
  }
  memcpy(&desordenados[inicio], &ordenados[inicio], (fin - inicio) * sizeof(int));
}

void quick_sort (int *a, int ini, int fin) {
  if (n < 2) return;
  int p = a[fin - ini / 2];
  int * l = ini;
  int * r = fin -1;

  while (l <= r) {
    if (*l < p) {
      l++;
      continue;
    }
    if (*r > p) {
      r--;
      continue;
    }
    int t = *l;
    *l++ = *r;
    *r-- = t;
  }
  quick_sort(a, ini, r);
  quick_sort(a, l, fin);
}

void * nodo(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;
  int penultimoNivel = (config.numNiveles - 1 == datos_nodo.nivel);
  int mitad = (fin - inicio) / 2;
  void * (*funcionHijos)(void * ) = penultimoNivel ? &hoja : &nodo;

  pthread_t izq;
  pthread_t der;

  struct datos_nodo datos_izq = {inicio   , mitad, datos_nodo->nivel + 1}; //inicializacion de agregados
  struct datos_nodo datos_der = {mitad + 1, fin  , datos_nodo->nivel + 1};

  if (0 != pthread_create(izq, NULL, funcionHijos, &datos_izq)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }

  if (0 != pthread_create(der, NULL, funcionHijos, &datos_der)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }

  if (0 != pthread_join(izq, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }

  if (0 != pthread_join(der, NULL)) {
    perror("pthread_join");
    exit(EX_OSERR);
  }

  merge(inicio, mitad, fin);

  pthread_exit(NULL);

}

////////////////////////////////////////////////////////////////////////////////

void * hoja(void * datos) {
  struct datos_nodo * datos_nodo = (struct datos_nodo *)datos;

  quick_sort(desordenados, datos_nodo->inicio, datos_nodo->fin);

  pthread_exit(NULL);
}

///////////////////////////////////////////////////////////////////////////////

void * principal(FILE * archivo, void * datos) {
  esordenados = ALLOC(config.numEnteros, sizeof(int));
  ordenados   = ALLOC(config.numEnteros, sizeof(int));
  pthread_t raiz;
  struct datos_nodo datos_raiz = {0, config.numEnteros, 1};

  if (NULL == desordenados || NULL == ordenados) {
    fprintf(stderr,"Error de reserva de memoria");
    exit(EX_OSERR);
  }

  if (config.numEnteros != fread(desordenado, config.numEnteros, sizeof(int), archivo)) {
    perror("fopen");
    exit(EX_IOERR);
  }

  if (0 != pthread_create(&raiz, NULL, funcionHijos, datos_raiz)) {
    perror("pthread_create");
    exit(EX_OSERR);
  }

}


void * apertura(void * datos, const char * nombre, void * (*funcion)(FILE *)) {
  // Abrimos el archivo para lectura.
  FILE * archivo = fopen(nombre, "r");
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
  config.archivoOrdenado = argv[P_ARCHIVOSDEENTEROSORDENADOS];

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  apertura(argv[1], principal, NULL);

  return 0;
}
