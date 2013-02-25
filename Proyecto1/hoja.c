/**
 * @file hoja.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de las funciones
 * que son comunes a todas las hojas.
 *
 */
#include <stdio.h>
#include <sysexits.h>
#include <unistd.h>

#include "hijos.h"
#include "ordenArchivo.h"
#include "procesos.h"

int * desordenados; /**<arreglo global en donde se guardan los desordenados*/

/**
 * Se encarga de leer todos los numeros de cierta seccion del
 * archivo y cargarlo en el arreglo de desordenados
 * @param archivo Archivo que contiene los numeros a ordenar
 * @param datos estructura que contiene los parametros para la lectura
 */
void * lectura(FILE * archivo, void * datos) {
  int numEnteros = *((int *)datos);
  fseek(archivo, configuracion.inicio * sizeof(int), SEEK_SET);
  desordenados = (int *)alloc(numEnteros, sizeof(int));
  if ((size_t)numEnteros != fread(desordenados, sizeof(int), numEnteros, archivo)) {
    perror("fread");
    exit(EX_IOERR);
  }
  return NULL;
}


/**
 * Se encarga de escribir todos los numeros ordenados de cierta seccion
 * del arreglo desordenados
 * @param archivo Archivo en donde se escribiran los numeros ordenados
 * @param datos estructura que contiene los parametros para la escritura
 */
void * escritura(FILE * archivo, void * datos) {
  int numEnteros = *((int *)datos);
  int i;
  for (i = 0; i < numEnteros; ++i) {
    fprintf(archivo, "%d ", desordenados[i]);
  }
  return NULL;
}


/**
 * Es el main del programa, se encarga de llamar a apertura dos veces
 * una para leer y trabajar con los datos y otra para escribir en el archivo,
 * su funcion mas importante es aplicar el quicksort.
 */
int main(int argc, char * argv[]) {
  //se llama al procedimiento que verifica los parametros de la llamada al main
  configurar(argc, argv);
  //variable que permite tener almacenada la cantidad de enteros con los que se trabaja
  int numEnteros = configuracion.fin - configuracion.inicio;
  //variable usada para generar los nombres temporales de los archivos intermedios
  char * nombreArch;
  //se genera el nombre del archivo de escritura para los archivos temporales
  asprintf(&nombreArch, "%d.txt", getpid());
  //se abre el archivo y se leen los datos
  apertura(&numEnteros, M_LECTURA, configuracion.archivoDesordenado, lectura);
  //se aplica el quicksort en la seccion de los datos
  quicksort(desordenados, 0, configuracion.fin - configuracion.inicio - 1);
  //se genera el archivo con los numeros de la seccion ya ordenados
  apertura(&numEnteros, M_ESCRITURA, nombreArch, escritura);
  //se libera la memoria reservada para el nombre del archivo
  free(nombreArch);

  return 0;
}
