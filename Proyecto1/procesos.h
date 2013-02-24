/**
 * @file procesos.h
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene las funciones que son comunes a
 * todos los procesos.
 *
 */
#ifndef PROCESOS_H
#define PROCESOS_H

#include "ordenArchivo.h"

/**
 * Tipo enumerado que indica si el hijo es un nodo
 * o una hoja
 */
enum hijo
  { H_NODO
  , H_HOJA
  }
;

/**
 * Se encarga de transformar el tipo enumerado hijo
 * a un string que indica el tipo de hijo que se va
 * a crear.
 * @param hijo Indica el tipo de hijo que queremos
 * crear (nodo/hoja)
 * @return Retorna el string que indica el tipo de
 * hijo (nodo/hoja)
 */
char const * tipoHijo(enum hijo hijo);


/**
 * Se encarga de crear los procesos hijos.
 * @param hijo Indica que tipo de hijo vamos a crear.
 * @param datos_nodo Datos que se le van a pasar al hijo.
 * @param numNiveles Numero de niveles el arbol de procesos.
 * @param archivoDesordenado Nombre del archivo de donde va
 * a leer el hijo.
 * @return Retorna el pid del hijo.
 */
pid_t child_create(enum hijo hijo, struct datos_nodo * datos_nodo, int numNiveles, char * archivoDesordenado);

/**
 * Se encarga de hacer wait a los procesos hijos.
 */
void child_wait();

#endif
