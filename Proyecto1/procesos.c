/**
 * @file procesos.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de las funciones
 * que son comunes a todos los procesos.
 *
 */
#include <errno.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ordenArchivo.h"
#include "procesos.h"


/**
 * Se encarga de transformar el tipo enumerado hijo
 * a un string que indica el tipo de hijo que se va
 * a crear.
 * @param hijo_ Indica el tipo de hijo que queremos
 * crear (nodo/hoja)
 * @return Retorna el string que indica el tipo de
 * hijo (nodo/hoja)
 */
char const * tipoHijo(enum hijo hijo_) {
  switch (hijo_) {
    case H_NODO: return "./nodo"; /**< Caso en el que es un nodo.*/
    case H_HOJA: return "./hoja"; /**< Caso en el que es una hoja.*/
    default    : return NULL    ; /**< caso base*/
  }
}


/**
 * Se encarga de crear los procesos hijos.
 * @param hijo_ Indica que tipo de hijo vamos a crear.
 * @param datos_nodo Datos que se le van a pasar al hijo.
 * @param numNiveles Numero de niveles el arbol de procesos.
 * @param archivoDesordenado Nombre del archivo de donde va
 * a leer el hijo.
 * @return Retorna el pid del hijo.
 */
pid_t child_create(enum hijo hijo_, struct datos_nodo * datos_nodo, int numNiveles, char * archivoDesordenado) {
  pid_t child = fork();
  // Creamos strings para pasarle los parametros
  // necesarios al exec
  char * inicio;
  char * fin;
  char * nivel;
  char * id;
  char * numNiv;

  switch (child) {
    case -1:
      // Si el fork retorna -1 es porque hubo un error.
      perror("fork");
      exit(EX_OSERR);

    case 0:
      // Si el fork devuelve 0 es porque es el hijo
      // de modo que hacemos el exec para llamar al
      // codigo del hijo.
      // Transformamos a strings los valores de
      // datos_nodo necesarios para la llamada.
      asprintf(&inicio, "%d", datos_nodo->inicio);
      asprintf(&fin   , "%d", datos_nodo->fin   );
      asprintf(&nivel , "%d", datos_nodo->nivel );
      asprintf(&id    , "%d", datos_nodo->id    );
      asprintf(&numNiv, "%d", numNiveles        );
      execlp
        ( tipoHijo(hijo_)
        , tipoHijo(hijo_)
        , inicio
        , fin
        , nivel
        , id
        , numNiv
        , archivoDesordenado
        , (char *)NULL
        )
      ;
      // La unica manera de que este codigo sea
      // alcanzable es que execlp retorne debido
      // a un error de modo que imprimimos un error.
      perror("execlp");
      exit(EX_OSERR);

    default:
      // Por defecto devuelve al padro el pid del hijo.
      return child;
  }
}


/**
 * Se encarga de hacer wait a los procesos hijos.
 */
void child_wait() {
    int status;

    // Si wait retorna -1 es porque hubo un error
    // de modo que imprimimos un mensaje.
    if (-1 == wait(&status)) {
      perror("wait");
      exit(EX_OSERR);
    }

    if (WIFEXITED(status)) {
      if (EX_OK != WEXITSTATUS(status)) {
        // Si el hijo salio y su status no fue ok,
        // hubo un error entonces imprimimos un mensaje.
        exit(WEXITSTATUS(status));
      }
    } else {
      // Si sale por alguna otra razon es porque hubo una
      // terminacion indebida de modo que imprimimos un mensaje.
      fprintf(stderr, "Abortando por terminacion indebida de hijo.\n");
      exit(EX_SOFTWARE);
    }
}
