/**
 * @file hijos.h
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de las funciones
 * que son comunes a todos los hijos.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

#include "hijos.h"

/**
 * Estrutura que guarda la configuracion del archivo
 */

struct configuracion configuracion;

/**
 * Se encarga de rellenar el struct configuracion a
 * partir del argv recibido y de verificar que los datos
 * pasados alli sean validos.
 * @param argc Cantidad de argumentos pasados
 * @param argv Arreglo que contiene los elementos
 */
void configurar(int argc, char * argv[]) {
  // Si el numero de argumentos con el que se invoca
  // es incorrecto se imprime un mensaje de error
  if (7 != argc) {
    fprintf(stderr, "Error interno\n");
    exit(EX_USAGE);
  }

  // Si no se puede convertir inicio a entero hay un error.
  if (1 != sscanf(argv[P_INICIO], "%d", &configuracion.inicio)) {
    fprintf(stderr, "El inicio debe ser un entero no negativo\n");
    exit(EX_USAGE);
  }

  // Si no se puede convertir fin a entero el hay un error.
  if (1 != sscanf(argv[P_FIN], "%d", &configuracion.fin)) {
    fprintf(stderr, "Fin debe ser un entero no negativo\n");
    exit(EX_USAGE);
  }

  // Si no se puede convertir nivel a entero el hay un error.
  if (1 != sscanf(argv[P_NIVEL], "%d", &configuracion.nivel)) {
    fprintf(stderr, "El numero de nivel debe ser un entero positivo\n");
    exit(EX_USAGE);
  }

  // Si no se puede convertir id a entero el hay un error.
  if (1 != sscanf(argv[P_ID], "%d", &configuracion.id)) {
    fprintf(stderr, "El id debe ser un entero positivo\n");
    exit(EX_USAGE);
  }

  // Si no se puede convertir numNiveles a entero el hay un error.
  if (1 != sscanf(argv[P_NUMNIVELES], "%d", &configuracion.numNiveles)) {
    fprintf(stderr, "El numero de niveles debe ser un entero positivo\n");
    exit(EX_USAGE);
  }

  configuracion.archivoDesordenado = argv[P_ARCHIVODEENTEROSDESORDENADOS];

  // Revisamos que el numero de inicio sea un entero no negativo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 > configuracion.inicio) {
    fprintf(stderr, "El inicio debe ser un entero no negativo\n");
    exit(EX_USAGE);
  }

  // Revisamos que el numero de fin sea un entero no negativo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 > configuracion.fin) {
    fprintf(stderr, "El fin debe ser un entero no negativo\n");
    exit(EX_USAGE);
  }

  // Revisamos que el nivel sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= configuracion.nivel) {
    fprintf(stderr, "El nivel debe ser un entero positivo\n");
    exit(EX_USAGE);
  }

  // Revisamos que el numero de id sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= configuracion.id) {
    fprintf(stderr, "El ID debe ser un entero positivo\n");
    exit(EX_USAGE);
  }

  // Revisamos que el numero de niveles totales sea un entero positivo
  // en caso de que no lo sea se imprime un mensaje de error
  if (0 >= configuracion.numNiveles) {
    fprintf(stderr, "El numero de niveles debe ser un entero positivo\n");
    exit(EX_USAGE);
  }
}
