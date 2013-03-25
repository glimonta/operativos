#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>

#include "actores.h"


typedef
  struct comando {
    enum selector
      { S_INVALIDO
      , S_MUERE
      , S_IMPRIME
      , S_LS
      }
      selector
    ;

    union argumentos {
      struct imprime {
        ssize_t longitud;
        char * texto;
      } imprime;
    } argumentos;
  }
  Comando
;

void mostrarComando(Comando comando) {
  switch (comando.selector) {
    case S_INVALIDO: {
      printf("Comando { .selector = inválido }\n");
    } break;
    case S_MUERE: {
      printf("Comando { .selector = muere }\n");
    } break;
    case S_IMPRIME: {
      printf("Comando { .selector = imprime, .argumentos.imprime = { .longitud = %d, .texto = %s } }\n", (int)comando.argumentos.imprime.longitud, comando.argumentos.imprime.texto);
    } break;
  }
}

Mensaje serializar(Comando comando) {
  Mensaje mensaje = {};

  switch (comando.selector) {
    case S_MUERE: {
      agregaMensaje(&mensaje, "m", sizeof(char));
    } break;

    case S_IMPRIME: {
      agregaMensaje(&mensaje, "i", sizeof(char));
      agregaMensaje(&mensaje, &comando.argumentos.imprime.longitud, sizeof(int));
      agregaMensaje(&mensaje, comando.argumentos.imprime.texto, comando.argumentos.imprime.longitud * sizeof(char));
    } break;

    default: break;
  }

  return mensaje;
}

Comando deserializar(Mensaje mensaje) {
  Comando comando = { .selector = S_INVALIDO };

  if (!mensaje.contenido) return comando;

  switch(((char *)mensaje.contenido)[0]) {
    case 'm':
      comando.selector = S_MUERE;
      break;

    case 'i':
      comando.selector = S_IMPRIME;
      comando.argumentos.imprime.longitud = *(int *)((char *)mensaje.contenido + sizeof(char));
      comando.argumentos.imprime.texto = (char *)(mensaje.contenido + sizeof(char) + sizeof(int));
      break;

    default:
      break;
  }

  return comando;
}

void enviarComando(Direccion direccion, Comando comando) {
  Mensaje mensaje = serializar(comando);
  enviar(direccion, mensaje);
  free(mensaje.contenido);
}

Comando muere() {
  Comando comando =
    { .selector = S_MUERE
    }
  ;
  return comando;
}

Comando imprime(char * texto) {
  Comando comando =
    { .selector = S_IMPRIME
    , .argumentos.imprime =
      { .longitud = strlen(texto)
      , .texto = texto
      }
    }
  ;
  return comando;
}



Direccion impresora;

struct libreta {
  char * nombre;
  Direccion direccion;
  struct libreta * siguiente;
};

struct libreta * libreta;

struct libreta * insertarLibreta(char * nombre, Direccion direccion) {
  struct libreta * nueva = calloc(1, sizeof(struct libreta));
  nueva->nombre = nombre;
  nueva->direccion = direccion;
  nueva->siguiente = libreta;
  libreta = nueva;
  return nueva;
}

void liberarLibreta() {
  if (!libreta) return;
  struct libreta * siguiente = libreta->siguiente;
  free(libreta->nombre);
  liberaDireccion(libreta->direccion);
  free(libreta);
  libreta = siguiente;
  liberarLibreta();
}

int filter(struct dirent const * dir);

Actor despachar(Mensaje mensaje, void * datos) {
  puts("HOLIIIIIIIIIS DESPACHADOOOOR :D");
  Comando comando = deserializar(mensaje);
  switch (comando.selector) {
    case S_MUERE: {
      struct libreta * entLibreta;
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        enviarComando(entLibreta->direccion, muere());
      }
      liberarLibreta();
      puts("CHAUUUUUUUS DESPACHADOOOOR :D");
      return finActor();
    }

    default: break;
  }
  enviarComando(impresora, imprime(mensaje.contenido));
  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

Actor contratar(Mensaje mensaje, void * datos) {
  puts("HOLIIIIIIIS CONTRATAAAR :D");
  if (-1 == chdir(mensaje.contenido)) {
    return finActor();
  }

  printf("El contenido es: %s\n", (char *) mensaje.contenido);
  enviarComando(impresora, imprime(mensaje.contenido));
  printf("El contenido es: %s\n", (char *) mensaje.contenido);

  struct dirent ** listaVacia;
  if (-1 == scandir(".", &listaVacia, filter, NULL)) {
    perror("scandir");
    exit(EX_IOERR);
  }

  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

Actor manejarPapa(Mensaje mensaje, void * datos);

int filter(struct dirent const * dir) {
  struct stat infoArchivo;
  if ((strcmp(dir->d_name, ".") == 0) || (strcmp(dir->d_name, "..") == 0)) return 0;
  if (-1 == stat(dir->d_name, &infoArchivo)) {
    perror("stat");
    exit(EX_IOERR);
  }

  if (S_ISDIR(infoArchivo.st_mode)) {
    char * nombre;
    asprintf(&nombre, "%s", dir->d_name);
    enviar
      ( insertarLibreta
          ( nombre
          , crear(mkActor(manejarPapa, NULL))
          )
        ->direccion
      , mkMensaje
          ( strlen(dir->d_name)
          , (char *)dir->d_name
          )
      )
    ;
  }

  return 0;
}



Actor manejarPapa(Mensaje mensaje, void * datos) {
  liberarLibreta();

  insertarLibreta("..", deserializarDireccion(mensaje));
  free(mensaje.contenido);
  return mkActor(contratar, datos);
}



Actor imprimir(Mensaje mensaje, void * datos) {
  Comando comando = deserializar(mensaje);
  mostrarComando(comando);
  switch (comando.selector) {
    case S_MUERE: {
      puts("CHAUUUUUUUS IMPRESORAAA :D");
      return finActor();
    }

    case S_IMPRIME: {
      printf("%s\n", comando.argumentos.imprime.texto);
    }

    default:
      break;
  }
  free(mensaje.contenido);
  return mkActor(imprimir, datos);
}



int main(int argc, char * argv[]) {
  if (2 != argc) {
    fprintf(stderr, "Uso: %s <directorio>\n", argv[0]);
    exit(EX_USAGE);
  }
  impresora = crear(mkActor(imprimir, NULL));
  Direccion raiz = crear(mkActor(contratar, NULL));
  enviar(raiz, mkMensaje(strlen(argv[1]), argv[1]));
  enviarComando(raiz, muere());
  enviarComando(impresora, muere());
  exit(EX_OK);
}
