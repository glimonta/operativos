#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>


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


void * principal(FILE * archivo, void * datos) {
  int cantidad;
  if (!sscanf((char *)datos, "%d", &cantidad)) {
    fprintf(stderr, "No existe numero en el parametro dado\n");
    return NULL;
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

  // Revisamos que el numero de enteros sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= argv[P_NUMENTEROS]) {
    fprintf(stderr, "La cantidad de enteros debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Revisamos que el numero de niveles sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= argv[P_NUMNIVELES]) {
    fprintf(stderr, "La cantidad de niveles debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int hojas = pow(2, argv[P_NUMNIVELES] - 1);
  int nodos = pow(2, argv[P_NUMNIVELES]) - 1;

  // Verificamos que el numero de enteros sea mayor al numero
  // de hojas del arbol.
  if (hojas > argv[P_NUMENTEROS]) {
    fprintf(stderr, "La cantidad de enteros es menor al numero de hojas del arbol, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  apertura(argv[1], principal, &argv[P_NUMENTEROS]);

  return 0;
}
