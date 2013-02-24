#include <errno.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ordenArchivo.h"
#include "procesos.h"



char const * tipoHijo(enum hijo hijo) {
  switch (hijo) {
    case H_NODO: return "./nodo";
    case H_HOJA: return "./hoja";
    default    : return NULL    ;
  }
}



pid_t child_create(enum hijo hijo, struct datos_nodo * datos_nodo, int numNiveles, char * archivoDesordenado) {
  pid_t child = fork();
  char * inicio;
  char * fin;
  char * nivel;
  char * id;
  char * numNiv;

  switch (child) {
    case -1:
      perror("fork");
      exit(EX_OSERR);

    case 0:
      asprintf(&inicio, "%d", datos_nodo->inicio);
      asprintf(&fin   , "%d", datos_nodo->fin   );
      asprintf(&nivel , "%d", datos_nodo->nivel );
      asprintf(&id    , "%d", datos_nodo->id    );
      asprintf(&numNiv, "%d", numNiveles        );
      execlp
        ( tipoHijo(hijo)
        , tipoHijo(hijo)
        , inicio
        , fin
        , nivel
        , id
        , numNiv
        , archivoDesordenado
        , (char *)NULL
        )
      ;
      perror("execlp");
      exit(EX_OSERR);

    default:
      return child;
  }
}



void child_wait() {
    int status;

    if (-1 == wait(&status)) {
      perror("wait");
      exit(EX_OSERR);
    }

    if (WIFEXITED(status)) {
      if (EX_OK != WEXITSTATUS(status)) {
        exit(WEXITSTATUS(status));
      }
    } else {
      fprintf(stderr, "Abortando por terminacion indebida de hijo.\n");
      exit(EX_SOFTWARE);
    }
}
