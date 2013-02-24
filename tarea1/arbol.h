/**
* @file arbol.h
* @author Gabriela Limonta 10-10385
* @author John Delgado 10-10196
*
* @section Grupo 09
*
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef arbol
#define arbol

/**
 * Estructura en la cual se guardan los arboles binarios.
 */
typedef struct NodoT {
  struct NodoT * izq; /**< Subarbol izquierdo */
  struct NodoT * der; /**< Subarbol derecho */
  int valor; /**< Valor entero del nodo */
} Nodo;

/**
 * Tipo enumerado que indica cual es el resultado
 * obtenido de insertar un nodo a un arbol.
 */
enum resultaditosInsertar
  { exitoInsertar /**< La operacion de insertar fue exitosa */
  , errorInsertar /**< Hubo un error al insertar en el arbol */
  , repetidoInsertar /**< Error cuando el arbol sobreescribe un nodo ya existente */
  }
;

typedef char * Camino;

/**
 * Se encarga de construir el arbol binario.
 * @param valor Valor que va en el nodo que estamos insertando.
 * @param caminito Camino que debe seguirse para llegar a la posicion
 * correcta del nodo.
 * @param apNodito Apuntador a la direccion de un arbol.
 * @return Retorna el estado de la operacion. exitoInsertar si
 * logra agregar el nodo correctamente, errorInsertar si ocurre un
 * error al añadirlo e repetidoInsertar si consigue que esta
 * sobreescribiendo un nodo ya existente.
 */
enum resultaditosInsertar insertar(int valor, Camino caminito, Nodo * * apNodito);

/**
 * Se encarga de revisar un arbol binario para verificar que
 * el mismo sea valido.
 * @param noditoAct Arbol a revisar.
 * @return retorna 1 si el arbol es valido y cero en caso contrario.
 */
int revisarCompleto(Nodo * noditoAct);

/**
 * Se encarga de eliminar un arbol binario cuando ya no estamos
 * utilizandolo para liberar ese espacio en la memoria.
 * @param noditoAct Arbol a eliminar.
 */
void eliminarCompleto(Nodo * noditoAct);

/**
 * Se encarga de imprimir completo los valores de un
 * nivel del arbol.
 * @param noditoAct Arbol binario.
 * @param nivel Nivel del arbol que queremos imprimir.
 * @return Retorna 1 si logra imprimir en caso contrario 
 * retorna 0.
 */
int imprimirNivel(Nodo * noditoAct, int nivel);

#endif
