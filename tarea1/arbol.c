/**
* @file arbol.c
* @author Gabriela Limonta 10-10385
* @author John Delgado 10-10196
*
* @section Grupo 09
*
* Implementacion de un Lazy Tree.
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "arbol.h"

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
enum resultaditosInsertar insertar(int valor, Camino caminito, Nodo * * apNodito){
  // Si el camino o el apuntador a la direccion del arbol es nulo
  // no podemos insertar.
  if (NULL == apNodito || NULL == caminito) return errorInsertar;
  // Si el arbol es vacio, creamos uno nuevo.
  if (NULL == *apNodito) {
    *apNodito = calloc(1, sizeof(Nodo));
    // Si no se pudo reservar memoria entonces devolvemos ENOMEM
    if (!*apNodito) return ENOMEM;
    (*apNodito)->valor = -1;
  }

  switch (*caminito) {
    // Caso en el que el camino sea vacio (hay que insertar en la posicion
    // donde estamos
    case '\0': return
      ((*apNodito)->valor == -1)
      // Si no hemos modificado, sustituimos el valor y retornamos un exito.
      ? ((*apNodito)->valor = valor, exitoInsertar)
      // Si estamos reescribiendo un nodo, sustituimos por -2 y retornamos
      // que hubo una falla.
      : ((*apNodito)->valor = -2, repetidoInsertar)
    ;

    // En caso de ser una 'I' o una 'D' llamamos recursivamente a la
    // funcion con el camino restante y el subarbol izquierdo o
    // derecho respectivamente.
    case 'I': return insertar(valor, caminito + 1, &((*apNodito)->izq));
    case 'D': return insertar(valor, caminito + 1, &((*apNodito)->der));

    default: return errorInsertar; // codigo no alcanzable.
  }
}

/**
 * Se encarga de revisar un arbol binario para verificar que
 * el mismo sea valido.
 * @param noditoAct Arbol a revisar.
 * @return retorna 1 si el arbol es valido y cero en caso contrario.
 */
int revisarCompleto(Nodo * noditoAct) {
  // Retorna true si llegamos al final o si el valor de
  // todos los nodos es valido (mayor o igual a cero) y
  // si su hijo izquierdo y derecho son arboles validos
  // tambien.
  return
    !noditoAct || (
      noditoAct->valor >= 0
      && revisarCompleto(noditoAct->izq)
      && revisarCompleto(noditoAct->der)
    )
  ;
}

/**
 * Se encarga de eliminar un arbol binario cuando ya no estamos
 * utilizandolo para liberar ese espacio en la memoria.
 * @param noditoAct Arbol a eliminar.
 */
void eliminarCompleto(Nodo * noditoAct) {
  // Si llego al final retorna.
  if (!noditoAct) return;
  // Si tiene hijo izquierdo, lo elimina.
  if (noditoAct->izq) eliminarCompleto(noditoAct->izq);
  // Si tiene hijo derecho, lo elimina.
  if (noditoAct->der) eliminarCompleto(noditoAct->der);
  // Libera el espacio de memoria ocupado por el nodo actual.
  free(noditoAct);
}

/**
 * Se encarga de imprimir completo los valores de un
 * nivel del arbol.
 * @param noditoAct Arbol binario.
 * @param nivel Nivel del arbol que queremos imprimir.
 * @return Retorna 1 si logra imprimir en caso contrario 
 * retorna 0.
 */
int imprimirNivel(Nodo * noditoAct, int nivel) {
  // Si el arbol es vacio no imprime.
  if (NULL == noditoAct) return 0;

  // Si llegamos al nivel que queremos, imprimimos en pantalla.
  if (0 == nivel) {
    printf("%d ",noditoAct->valor);
    return 1;
  }

  // Hacemos la llamada recursiva para los subarboles
  // izquierdo y derecho si no llegamos al nivel que
  // queriamos imprimir.
  return
      imprimirNivel(noditoAct->izq, nivel - 1)
    | imprimirNivel(noditoAct->der, nivel - 1)
  ;
}
