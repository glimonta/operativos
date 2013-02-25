/**
 * @file hijos.h
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene las funciones que son comunes a
 * todos los hijos.
 *
 */
#ifndef HIJOS_H
#define HIJOS_H

/**
 * Tipo enumerado que indica los tipos de parametros
 * que son pasados en cada llamada a un hijo.
 */
enum parametros
  { P_INICIO = 1
  , P_FIN
  , P_NIVEL
  , P_ID
  , P_NUMNIVELES
  , P_ARCHIVODEENTEROSDESORDENADOS
  }
;

/**
 * Estructura que se utiliza para guardar las opciones pasadas
 * como argumentos por la linea de comandos, para que sean
 * utilizadas por todo el programa.
 */
struct configuracion {
  int inicio;
  int fin;
  int nivel;
  int id;
  int numNiveles;
  char * archivoDesordenado;
};

extern struct configuracion configuracion;

/**
 * Se encarga de rellenar el struct configuracion a
 * partir del argv recibido y de verificar que los datos
 * pasados alli sean validos.
 * @param argc Cantidad de argumentos pasados
 * @param argv Arreglo que contiene los elementos
 */
void configurar(int argc, char * argv[]);

#endif
