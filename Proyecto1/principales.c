#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "principales.h"

struct configuracion configuracion;

void configurar(int argc, char * argv[]) {
  // Si el numero de argumentos con el que se invoca al programa
  // es incorrecto se imprime un mensaje de error indicandole al
  // usuario como invocar al programa.
  if (5 != argc) {
    fprintf(stderr, "Numero de argumentos incorrecto, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EX_USAGE);
  }

  if (1 != sscanf(argv[P_NUMENTEROS], "%d", &configuracion.numEnteros)) {
    fprintf(stderr, "La cantidad de enteros debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EX_USAGE);
  }

  // Revisamos que el numero de enteros sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= configuracion.numEnteros) {
    fprintf(stderr, "La cantidad de enteros debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EX_USAGE);
  }

  if (1 != sscanf(argv[P_NUMNIVELES], "%d", &configuracion.numNiveles)) {
    fprintf(stderr, "La cantidad de niveles debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EX_USAGE);
  }

  // Revisamos que el numero de niveles sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= configuracion.numNiveles) {
    fprintf(stderr, "La cantidad de niveles debe ser un entero positivo, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EX_USAGE);
  }

  // Verificamos que el numero de enteros sea mayor al numero
  // de hojas del arbol.
  if (pow(2, configuracion.numNiveles - 1) > configuracion.numEnteros) {
    fprintf(stderr, "La cantidad de enteros es menor al numero de hojas del arbol, invoque el programa con: %s num_enteros num_niveles archivo_enteros_deseordenado archivo_de_enteros_ordenado\n", argv[0]);
    exit(EX_USAGE);
  }

  configuracion.archivoDesordenado = argv[P_ARCHIVODEENTEROSDESORDENADOS];
  configuracion.archivoOrdenado    = argv[P_ARCHIVODEENTEROSORDENADOS   ];
}
