#include "hijos.h"
#include "ordenArchivo.h"

int main(int argc, char * argv[]) {
  configurar(argc, argv);

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.

  apertura(&configuracion, M_ESCRITURA, configuracion.archivoDesordenado   , lectura  );
  quicksort(desordenados, 0, configuracion.fin - configuracion.inicio - 1);
  apertura(&configuracion, M_ESCRITURA, configuracion.id                   , escritura);

  return 0;
}
