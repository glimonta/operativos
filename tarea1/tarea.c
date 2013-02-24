/**
* @file tarea.c
* @author Gabriela Limonta 10-10385
* @author John Delgado 10-10196
*
* @section Grupo 09
*
* Programa que dada una secuencia de arboles binarios, imprime
* un recorrido por nivel de cada arbol.
*/
#include "arbol.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Se encarga de abrir el archivo de texto, llamar a una funcion que
 * trabaja con el y cerrarlo luego de terminar.
 * @param nombrecito Nombre del archivo con el que se va a trabajar.
 * @param funcioncita Funcion que va a trabajar con el archivo indicado.
 * @return Retorna NULL cuando ocurre un error.
 */
void * apertura(const char * nombrecito, void * (*funcioncita)(FILE *)) {
  // Abrimos el archivo para lectura.
  FILE * archivito = fopen(nombrecito, "r");
  // Si el archivo retorna NULL es porque hubo un error en fopen
  // se detiene la ejecucion del programa.
  if (NULL == archivito) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  // Llamamos a la funcion que va a trabajar con el archivo.
  void * retornito = funcioncita(archivito);

  // Cerramos el archivo.
  fclose(archivito);
  // Retornamos lo que devuelve la ejecucion de la función.
  return retornito;
}

Nodo * arbolitoJG = NULL;

/**
 * Se encarga de leer el archivo e ir construyendo el arbol
 * binario con el que se va a trabajar.
 * @param archivito Archivo de entrada de donde leeremos la
 * informacion.
 */
void * principal(FILE * archivito) {
  // El ciclo de lectura va a continuar mientras lea un formato correcto
  // (que inicie en "("), no haya algun error al leer el archivo o sea
  // fin del archivo.
  while (fscanf(archivito, " ("), !ferror(archivito) && !feof(archivito)) {
    {
      // Leemos el siguiente caracter del archivo.
      char c = fgetc(archivito);
      // Si es un ")" entonces es el final de un arbol.
      if (')' == c) {
        // En caso de no ser un arbol completo, imprimimos incompleto.
        if (0 == revisarCompleto(arbolitoJG)) printf("incompleto\n");
        // Si es un arbol valido, lo imprimimos en pantalla.
        else {
          int nivel;
          // Imprimimos todos los niveles del arbol desde el 0 hasta el
          // ultimo nivel.
          for (nivel = 0; imprimirNivel(arbolitoJG, nivel); ++nivel);
          puts("");
        }

        //Eliminamos el arbol con el que ya trabajamos.
        eliminarCompleto(arbolitoJG);
        // Reinicializamos a NULL el apuntador al arbol.
        arbolitoJG = NULL;
        continue;
      }
      // Si no es el fin del arbol devolvemos el caracter que leimos
      // y procedemos a leer el valor y camino del nodo.
      ungetc(c, archivito);
    }

    int valorcito;

    // Leemos el valor que debe tener el nodo. En caso de que haya
    // un error leyendo, retornamos null.
    if (1 != fscanf(archivito, "%d,", &valorcito)) {
      fprintf(stderr, "El formato de entrada es incorrecto\n");
      return NULL;
    }

    Camino caminito;
    {
      // Obtenemos el proximo caracter del archivo.
      char c = fgetc(archivito);
      // Si es un ")" es porque el camino es vacio (es la raiz)
      if (')' == c) {
        caminito = "";
      } else {
        //Sino devolvemos el caracter y procedemos a leer el camino.
        ungetc(c, archivito);
        // Leemos la cadena de Is y Ds que debe seguir el nodo para
        // posicionarse en el lugar del arbol que va y lo guardamos
        // en caminito. En caso de que haya un error leyendo esto
        // retornamos NULL.
        if (1 != fscanf(archivito, "%m[ID]) ", &caminito)) {
          fprintf(stderr, "El formato de entrada es incorrecto\n");
          return NULL;
        }
      }
    }

    // Procedemos a insertar el nodo en el arbol.
    insertar(valorcito, caminito, &arbolitoJG);
    if (*caminito) free(caminito);
  }
  return NULL;
}

/**
 * Main del programa.
 * @param argc Cantidad de elementos del arreglo de entrada
 * @param argv Arreglo que contiene los argumentos con los
 * que fue llamado el programa.
 */
int main(int argc, char * argv[]) {
  // Si el numero de argumentos con el que se invoca al programa
  // es incorrecto se imprime un mensaje de error indicandole al
  // usuario como invocar al programa.
  if (2 != argc) {
    fprintf(stderr, "Numero de argumentos incorrecto, invoque el programa con: %s archivo_de_entrada\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Abrimos el archivo y le pasamos a la funcion
  // principal para que trabaje con el mismo.
  apertura(argv[1], principal);

  return 0;
}
