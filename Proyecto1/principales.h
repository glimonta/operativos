/**
 * @file principales.h
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de las funciones
 * que son comunes a todas las verificaciones de
 * pases de argumentos por el main hacia las
 * estructuras de configuraciones
 *
 */
#ifndef PRINCIPALES_H
#define PRINCIPALES_H

/**
 *Tipo enumerado que permite hacer mas facil la identificacion de indices
 *para los parametros del main
 */
enum parametros
  { P_NUMENTEROS = 1
  , P_NUMNIVELES
  , P_ARCHIVODEENTEROSDESORDENADOS
  , P_ARCHIVODEENTEROSORDENADOS
  }
;

/**
 *Estructura que almacena la configuracion del main ya parseada y lista para ser usada
 *por procedimientos internos
 */
struct configuracion {
  int numEnteros; /**<cantidad de enteros*/
  int numNiveles; /**<numero de niveles*/
  char * archivoDesordenado; /**<nombre de archivo desordenado*/
  char * archivoOrdenado; /**<nombre de archivo ordenado*/
};

//se declara una variable que no requiere definicion y se usa para codigo externo, no genera
//codigo y se usa para el compilador
extern struct configuracion configuracion;

//firma de la funcion configurar, se exporta aqui
void configurar(int argc, char * argv[]);

#endif
