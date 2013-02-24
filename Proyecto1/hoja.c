#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>

#include "hijos.h"
#include "ordenArchivo.h"
#include "procesos.h"

int * desordenados;

void * lectura(FILE * archivo, void * datos) {
  int numEnteros = *((int *)datos);
  fseek(archivo, configuracion.inicio * sizeof(int), SEEK_SET);
  desordenados = (int *)ALLOC(numEnteros, sizeof(int));
  if ((size_t)numEnteros != fread(desordenados, sizeof(int), numEnteros, archivo)) {
    perror("fread");
    exit(EX_IOERR);
  }
  return NULL;
}

void * escritura(FILE * archivo, void * datos) {
  int numEnteros = *((int *)datos);
  int i;
  for (i = 0; i < numEnteros; ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
  return NULL;
}

int main(int argc, char * argv[]) {
  configurar(argc, argv);
  int numEnteros = configuracion.fin - configuracion.inicio;

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.

  char * nombreArch;
  asprintf(&nombreArch, "%d.txt", getpid());

  apertura(&numEnteros, M_LECTURA, configuracion.archivoDesordenado, lectura);
  quicksort(desordenados, 0, configuracion.fin - configuracion.inicio - 1);
  apertura(&numEnteros, M_ESCRITURA, nombreArch, escritura);

  free(nombreArch);

  return 0;
}
