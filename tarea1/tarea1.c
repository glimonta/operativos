#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef char * Camino;

typedef struct NodoT {
  struct NodoT * izq;
  struct NodoT * der;
  int valor;
} Nodo;

enum resultaditosInsertar
  { exitoInsertar
  , errorInsertar
  , incompletoInsertar
  }
;

enum resultaditosInsertar insertar(int valor, Camino caminito, Nodo * * john){
  if (NULL == john || NULL == caminito) return errorInsertar;
  if (NULL == *john) {
    *john = calloc(1, sizeof(Nodo)); // TODO: check errors
    (*john)->valor = -1;
  }

  switch (*caminito) {
    case '\0': return
      ((*john)->valor == -1)
      ? ((*john)->valor = valor, exitoInsertar)
      : incompletoInsertar
    ;

    case 'I': return insertar(valor, caminito + 1, &((*john)->izq));
    case 'D': return insertar(valor, caminito + 1, &((*john)->der));

    default: return errorInsertar; // not reached
  }
}



void * apertura(const char * nombrecito, void * (*funcioncita)(FILE *)) {
  FILE * archivito = fopen(nombrecito, "r");
  if (NULL == archivito) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  void * retornito = funcioncita(archivito);

  fclose(archivito);
  return retornito;
}



int revisarCompleto(Nodo * noditoAct) {
  return
    !noditoAct || (
      noditoAct->valor >= 0
      && revisarCompleto(noditoAct->izq)
      && revisarCompleto(noditoAct->der)
    )
  ;
}

void eliminarCompleto(Nodo * noditoAct) {
  if (!noditoAct) return;
  if (noditoAct->izq) eliminarCompleto(noditoAct->izq);
  if (noditoAct->der) eliminarCompleto(noditoAct->der);
  free(noditoAct);
}

int imprimirNivel(Nodo * noditoAct, int nivel) {
  if (NULL == noditoAct) return 0;

  if (0 == nivel) {
    printf("%d ",noditoAct->valor);
    return 1;
  }

  return
      imprimirNivel(noditoAct->izq, nivel - 1)
    | imprimirNivel(noditoAct->der, nivel - 1)
  ;
}


Nodo * arbolitoJG = NULL;

void * gabbi(FILE * archivito) {
  while (fscanf(archivito, " ("), !ferror(archivito) && !feof(archivito)) {
    {
      char c = fgetc(archivito);
      if (')' == c) {
        if (0 == revisarCompleto(arbolitoJG)) printf("incompleto\n");
        else {
          int nivel;
          for (nivel = 0; imprimirNivel(arbolitoJG, nivel); ++nivel);
          puts("");
        }
        eliminarCompleto(arbolitoJG);
        arbolitoJG = NULL;
        continue;
      }
      ungetc(c, archivito);
    }

    int valorcito;
    if (1 != fscanf(archivito, "%d,", &valorcito)) {
      fprintf(stderr, "El formato de entrada es incorrecto\n");
      return NULL;
    }

    Camino caminito;
    {
      char c = fgetc(archivito);
      if (')' == c) {
        caminito = "";
      } else {
        ungetc(c, archivito);
        if (1 != fscanf(archivito, "%m[ID]) ", &caminito)) {
          fprintf(stderr, "El formato de entrada es incorrecto\n");
          return NULL;
        }
      }
    }

    insertar(valorcito, caminito, &arbolitoJG); // TODO: check errors
  }
  return NULL;
}


int main(int argc, char * argv[]) {
  if (2 != argc) {
    fprintf(stderr, "Numero de argumentos incorrecto, invoque el programa con: %s archivo_de_entrada\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  apertura(argv[1], gabbi);

  return 0;
}
