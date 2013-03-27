#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "actores.h"


typedef
  struct instruccion {
    enum selectorInstruccion
      { SI_INVALIDO
      , SI_MUERE
      , SI_IMPRIME
      , SI_RM
      , SI_LS
      , SI_MKDIR
      , SI_RMDIR
      , SI_CAT
      , SI_FIND
      , SI_CP
      , SI_MV
      , SI_WRITE
      , SI_ERROR
      }
      selector
    ;

    union argumentos {
      struct datos {
        ssize_t longitud;
        char * texto;
      } datos;
      struct unPath {
        char * camino;
      } unPath;
      struct datosPath {
        char * camino;
        ssize_t longitud;
        char * texto;
      } datosPath;
      struct dosPath {
        char * fuente;
        char * destino;
      } dosPath;
      struct error {
        int codigo;
      } error;
    } argumentos;
  }
  Instruccion
;

void mostrarInstruccion(Instruccion instruccion) {
  switch (instruccion.selector) {
    case SI_INVALIDO: { printf("Instruccion { .selector = inválido" " }\n"); } break;
    case SI_MUERE   : { printf("Instruccion { .selector = muere"    " }\n"); } break;
    case SI_IMPRIME : { printf("Instruccion { .selector = imprime"  ", .argumentos.datos = { .longitud = %d, .texto = %s } }\n", (int)instruccion.argumentos.datos.longitud, instruccion.argumentos.datos.texto); } break;
    case SI_FIND    : { printf("Instruccion { .selector = find"     ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_CAT     : { printf("Instruccion { .selector = cat"      ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_MKDIR   : { printf("Instruccion { .selector = mkdir"    ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_LS      : { printf("Instruccion { .selector = ls"       ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_RMDIR   : { printf("Instruccion { .selector = rmdir"    ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_RM      : { printf("Instruccion { .selector = rm"       ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_MV      : { printf("Instruccion { .selector = mv"       ", .argumentos.dosPath = { .fuente = %s, destino = %s }\n",instruccion.argumentos.dosPath.fuente, instruccion.argumentos.dosPath.destino); } break;
    case SI_CP      : { printf("Instruccion { .selector = cp"       ", .argumentos.dosPath = { .fuente = %s, destino = %s }\n",instruccion.argumentos.dosPath.fuente, instruccion.argumentos.dosPath.destino); } break;
    case SI_WRITE   : { printf("Instruccion { .selector = write"    ", .argumentos.datosPath = { .camino = %s, .longitud = %d, .texto = %s } }\n", instruccion.argumentos.datosPath.camino, (int)instruccion.argumentos.datosPath.longitud, instruccion.argumentos.datosPath.texto); } break;
    case SI_ERROR   : { printf("Instruccion { .selector = inválido" ", .argumentos.error.codigo = %d }\n", instruccion.argumentos.error.codigo); } break;
  }
}

Mensaje serializar(Instruccion instruccion) {
  Mensaje mensaje = {};

  agregaMensaje(&mensaje, &instruccion.selector, sizeof(enum selectorInstruccion));
  switch (instruccion.selector) {
    case SI_IMPRIME: {
      agregaMensaje(&mensaje, &instruccion.argumentos.datos.longitud, sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datos.texto, instruccion.argumentos.datos.longitud * sizeof(char));
    } break;

    case SI_RM   :
    case SI_RMDIR:
    case SI_MKDIR:
    case SI_LS   :
    case SI_FIND :
    case SI_CAT  :
      agregaMensaje(&mensaje, instruccion.argumentos.unPath.camino, strlen(instruccion.argumentos.unPath.camino) * sizeof(char));
      agregaMensaje(&mensaje, "\0", sizeof(char));
      break;

    case SI_MV:
    case SI_CP:
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.fuente, strlen(instruccion.argumentos.dosPath.fuente) * sizeof(char));
      agregaMensaje(&mensaje, "\0", sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.destino, strlen(instruccion.argumentos.dosPath.destino) * sizeof(char));
      agregaMensaje(&mensaje, "\0", sizeof(char));
      break;

    case SI_WRITE: {
      agregaMensaje(&mensaje, instruccion.argumentos.datosPath.camino, strlen(instruccion.argumentos.datosPath.camino) * sizeof(char));
      agregaMensaje(&mensaje, "\0", sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.datosPath.longitud, sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datosPath.texto, instruccion.argumentos.datos.longitud * sizeof(char));
    } break;

    case SI_ERROR: {
      agregaMensaje(&mensaje, &instruccion.argumentos.error.codigo, sizeof(int));
    } break;

    default: break;
  }

  return mensaje;
}

Instruccion deserializar(Mensaje mensaje) {
  Instruccion instruccion = { .selector = SI_INVALIDO };
  if (!mensaje.contenido) return instruccion;

  instruccion.selector = *(enum selectorInstruccion *)mensaje.contenido;
  mensaje.contenido += sizeof(enum selectorInstruccion);
  switch (instruccion.selector) {
    case SI_IMPRIME:
      instruccion.argumentos.datos.longitud = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datos.texto = (char *)mensaje.contenido; mensaje.contenido += instruccion.argumentos.datos.longitud * sizeof(char);
      break;

    case SI_RM   :
    case SI_RMDIR:
    case SI_LS   :
    case SI_MKDIR:
    case SI_FIND :
    case SI_CAT  :
      instruccion.argumentos.unPath.camino = (char *)mensaje.contenido; mensaje.contenido += strlen(instruccion.argumentos.unPath.camino) * sizeof(char);
      mensaje.contenido += 1;
      break;

    case SI_MV:
    case SI_CP:
      instruccion.argumentos.dosPath.fuente = (char *)mensaje.contenido; mensaje.contenido += strlen(mensaje.contenido) * sizeof(char);
      mensaje.contenido += 1;
      instruccion.argumentos.dosPath.destino = (char *)mensaje.contenido; mensaje.contenido += strlen(mensaje.contenido) * sizeof(char);
      mensaje.contenido += 1;
      break;

    case SI_WRITE:
      instruccion.argumentos.datosPath.camino = (char *)mensaje.contenido; mensaje.contenido += strlen(mensaje.contenido) * sizeof(char);
      mensaje.contenido += 1;
      instruccion.argumentos.datosPath.longitud = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datosPath.texto = (char *)mensaje.contenido; mensaje.contenido += instruccion.argumentos.datosPath.longitud * sizeof(char);
      break;

    case SI_ERROR:
      instruccion.argumentos.error.codigo = *(int *)mensaje.contenido; mensaje.contenido += sizeof(int);
      break;

    default: break;
  }

  return instruccion;
}

void enviarInstruccion(Direccion direccion, Instruccion instruccion) {
  Mensaje mensaje = serializar(instruccion);
  if (-1 == enviar(direccion, mensaje)) {
    perror("enviar");
    exit(EX_IOERR);
  }
  free(mensaje.contenido);
}



Instruccion instruccionSimple(enum selectorInstruccion selector) {
  Instruccion instruccion =
    { .selector = selector
    }
  ;
  return instruccion;
}

Instruccion instruccionDatos(enum selectorInstruccion selector, ssize_t longitud, char * texto) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.datos =
      { .longitud = longitud
      , .texto = texto
      }
    }
  ;
  return instruccion;
}

Instruccion instruccionUnPath(enum selectorInstruccion selector, char * camino) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.unPath.camino = camino
    }
  ;
  return instruccion;
}

Instruccion instruccionDosPath(enum selectorInstruccion selector, char * fuente, char * destino) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos =
      { .dosPath.fuente = fuente
      , .dosPath.destino = destino
      }
    }
  ;
  return instruccion;
}

Instruccion instruccionDatosPath(enum selectorInstruccion selector, char * camino, ssize_t longitud, char * texto) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.datosPath =
      { .camino = camino
      , .longitud = longitud
      , .texto = texto
      }
    }
  ;
  return instruccion;
}

Instruccion instruccionError(enum selectorInstruccion selector, int codigo) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.error =
      { .codigo = codigo
      }
    }
  ;
  return instruccion;
}

Instruccion c_muere  ()                                              { return instruccionSimple   (SI_MUERE                           ); }
Instruccion c_imprime(char * texto)                                  { return instruccionDatos    (SI_IMPRIME, strlen(texto), texto   ); }
Instruccion c_ls     (char * camino)                                 { return instruccionUnPath   (SI_LS     , camino                 ); }
Instruccion c_mkDir  (char * camino)                                 { return instruccionUnPath   (SI_MKDIR  , camino                 ); }
Instruccion c_rm     (char * camino)                                 { return instruccionUnPath   (SI_RM     , camino                 ); }
Instruccion c_rmdir  (char * camino)                                 { return instruccionUnPath   (SI_RMDIR  , camino                 ); }
Instruccion c_find   (char * camino)                                 { return instruccionUnPath   (SI_FIND   , camino                 ); }
Instruccion c_cat    (char * camino)                                 { return instruccionUnPath   (SI_CAT    , camino                 ); }
Instruccion c_mv     (char * fuente, char * destino)                 { return instruccionDosPath  (SI_MV     , fuente, destino        ); }
Instruccion c_cp     (char * fuente, char * destino)                 { return instruccionDosPath  (SI_CP     , fuente, destino        ); }
Instruccion c_write  (char * camino, ssize_t longitud, char * texto) { return instruccionDatosPath(SI_WRITE  , camino, longitud, texto); }
Instruccion c_error  (int codigo)                                    { return instruccionError    (SI_ERROR  , codigo                 ); }



Direccion impresora;
Direccion raiz;

void muere() {
  enviarInstruccion(raiz, c_muere());
  enviarInstruccion(impresora, c_muere());
}


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

Direccion buscarLibreta(char * nombre) {
  struct libreta * entLibreta;
  for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
    if (!strcmp(entLibreta->nombre, nombre)) return entLibreta->direccion;
  }

  return NULL;
}


void do_rm(char * camino) {
  if (-1 == unlink(camino)) {
    enviarInstruccion(impresora, c_error(errno));

    switch (errno) {
      case EFAULT:
      case ENOMEM:
        enviarInstruccion(raiz, c_muere());
        break;

      default: break;
    }
  }
}

void do_rmdir(char * camino) {
  if (-1 == rmdir(camino)) {
    enviarInstruccion(impresora, c_error(errno));

    switch (errno) {
      case EFAULT:
      case ENOMEM:
        enviarInstruccion(raiz, c_muere());
        break;

      default: break;
    }
  }
}

char * chomp(char * s) {
  char * pos = strrchr(s, '\n');
  if (pos) *pos = '\0';
  return s;
}

void do_ls(char * camino) {
  struct stat stats;
  if (-1 == stat(camino, &stats)) {
    enviarInstruccion(impresora, c_error(errno));

    switch (errno) {
      case EFAULT:
      case ENOMEM:
        enviarInstruccion(raiz, c_muere());
        break;

      default: break;
    }
  }

  char * user;
  if (!getpwuid(stats.st_uid)) {
    //FIXME verificar errores
    asprintf(&user, "%s", getpwuid(stats.st_uid)->pw_name);
  } else {
    //FIXME verificar errores
    asprintf(&user, "%d", stats.st_uid);
  }

  char * group;
  if (!getgrgid(stats.st_gid)) {
    //FIXME verificar errores
    asprintf(&group, "%s", getgrgid(stats.st_gid)->gr_name);
  } else {
    //FIXME verificar errores
    asprintf(&group, "%d", stats.st_gid);
  }

  char * texto;
  //FIXME verificar errores
  asprintf
    ( &texto
    , "%c%c%c%c%c%c%c%c%c%c %d %s %s %d %s %s\n"
    , S_ISDIR(stats.st_mode)  ? 'd' : '-'
    , S_IRUSR & stats.st_mode ? 'r' : '-'
    , S_IWUSR & stats.st_mode ? 'w' : '-'
    , S_IXUSR & stats.st_mode ? 'x' : '-'
    , S_IRGRP & stats.st_mode ? 'r' : '-'
    , S_IWGRP & stats.st_mode ? 'w' : '-'
    , S_IXGRP & stats.st_mode ? 'x' : '-'
    , S_IROTH & stats.st_mode ? 'r' : '-'
    , S_IWOTH & stats.st_mode ? 'w' : '-'
    , S_IXOTH & stats.st_mode ? 'x' : '-'
    , (int)stats.st_nlink
    , user
    , group
    , (int)stats.st_blocks
    , chomp(ctime(&stats.st_mtime))
    , camino
    )
  ;
  free(user);
  free(group);
  enviarInstruccion(impresora, c_imprime(texto));
  free(texto);
}

void descender(void (*accion)(char *), Instruccion instruccion) {
  char * colaDeCamino = strchr(instruccion.argumentos.unPath.camino, '/');
  if (!colaDeCamino) {
    accion(instruccion.argumentos.unPath.camino);
  } else {
    *colaDeCamino = '\0';
    ++colaDeCamino;
    Direccion subDirectorio;
    if ((subDirectorio = buscarLibreta(instruccion.argumentos.unPath.camino))) {
      enviarInstruccion(subDirectorio, c_rm(colaDeCamino));
    } else {
      enviarInstruccion(impresora, c_imprime("No se encuentra el archivo\n"));
      // FIXME: usar errno
    }
  }
}

int filter(struct dirent const * dir);

Actor despachar(Mensaje mensaje, void * datos) {
  Instruccion instruccion = deserializar(mensaje);

  switch (instruccion.selector) {
    case SI_MUERE: {
      struct libreta * entLibreta;
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        enviarInstruccion(entLibreta->direccion, c_muere());
      }
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        esperar(entLibreta->direccion);
      }
      liberarLibreta();
      return finActor();
    }

    case SI_RM   : descender(do_rm   , instruccion); break;
    case SI_RMDIR: descender(do_rmdir, instruccion); break;
    case SI_LS   : mostrarInstruccion(instruccion); descender(do_ls   , instruccion); break;


    case SI_FIND:
      //aqui va el codigo manejador del find
    break;

    case SI_CAT:
      //aqui va el codigo manejador del cat
    break;

    case SI_CP:
      //aqui va el codigo manejador del cp
    break;

    case SI_MV:
      //aqui va el codigo manejador del mv
    break;

    default: break;
  }
  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

Actor contratar(Mensaje mensaje, void * datos) {
  if (-1 == chdir(mensaje.contenido)) {
    return finActor();
  }

  struct dirent ** listaVacia;
  if (-1 == scandir(".", &listaVacia, filter, NULL)) {
    enviarInstruccion(impresora, c_error(errno)); // TODO: enviar "scandir" vía c_error
    enviarInstruccion(yo, c_muere());
    free(mensaje.contenido);
    return mkActor(despachar, datos);
  }

  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

Actor manejarPapa(Mensaje mensaje, void * datos);

int filter(struct dirent const * dir) {
  struct stat infoArchivo;
  if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) return 0;
  if (-1 == stat(dir->d_name, &infoArchivo)) {
    enviarInstruccion(impresora, c_error(errno)); // TODO: enviar "stat" vía c_error
    muere();
    // TODO: hacer que scandir no continúe
  }

  if (S_ISDIR(infoArchivo.st_mode)) {
    char * nombre;
    asprintf(&nombre, "%s", dir->d_name);

    Direccion subdirectorio = crear(mkActor(manejarPapa, NULL));
    if (!subdirectorio) {
      perror("crear");
      exit(EX_IOERR);
    }

    insertarLibreta(nombre, subdirectorio);

    int retorno = enviar
      ( subdirectorio
      , mkMensaje(strlen(dir->d_name), (char *)dir->d_name)
      )
    ;
    if (-1 == retorno) {
      perror("enviar");
      exit(EX_IOERR);
    }
  }

  return 0;
}



Actor manejarPapa(Mensaje mensaje, void * datos) {
  liberarLibreta();

  char * puntopunto;
  asprintf(&puntopunto, "..");
  insertarLibreta(puntopunto, deserializarDireccion(mensaje));
  free(mensaje.contenido);
  return mkActor(contratar, datos);
}



Actor imprimir(Mensaje mensaje, void * datos) {
  Instruccion instruccion = deserializar(mensaje);

  switch (instruccion.selector) {
    case SI_MUERE: {
      return finActor();
    }

    case SI_IMPRIME: {
      fwrite(instruccion.argumentos.datos.texto, sizeof(char), instruccion.argumentos.datos.longitud, stdout);
      fflush(stdout);
    } break;

    case SI_ERROR: {
      errno = instruccion.argumentos.error.codigo;
      perror(NULL);
    } break;

    default:
      break;
  }
  free(mensaje.contenido);
  return mkActor(imprimir, datos);
}



void avanzaHasta(int (*predicado)(int c), char ** cursor) {
  if (!cursor || !*cursor || '\0' == **cursor || predicado(**cursor)) return;
  ++*cursor; avanzaHasta(predicado, cursor);
}

void avanzaHastaNo(int (*predicado)(int c), char ** cursor) {
  if (!cursor || !*cursor || '\0' == **cursor || !predicado(**cursor)) return;
  ++*cursor; avanzaHasta(predicado, cursor);
}



typedef
  struct comando {
    enum selectorComando
      { SC_INVALIDO = 0
      , SC_LS
      , SC_CAT
      , SC_CP
      , SC_MV
      , SC_FIND
      , SC_RM
      , SC_MKDIR
      , SC_RMDIR
      , SC_QUIT
      }
      selector
    ;

    char * argumento1;
    char * argumento2;

    char * fuente;
    char * destino;
  }
  Comando
;

enum selectorComando decodificar(char * texto) {
  if (!strcmp(texto, "ls"   )) return SC_LS   ;
  if (!strcmp(texto, "cat"  )) return SC_CAT  ;
  if (!strcmp(texto, "cp"   )) return SC_CP   ;
  if (!strcmp(texto, "mv"   )) return SC_MV   ;
  if (!strcmp(texto, "find" )) return SC_FIND ;
  if (!strcmp(texto, "rm"   )) return SC_RM   ;
  if (!strcmp(texto, "mkdir")) return SC_MKDIR;
  if (!strcmp(texto, "rmdir")) return SC_RMDIR;
  if (!strcmp(texto, "quit" )) return SC_QUIT ;
  return SC_INVALIDO;
}




Comando fetch() {
  Comando comando = {};

  enviarInstruccion(impresora, c_imprime("chelito: "));

  char * linea = NULL;
  size_t longitud;
  errno = 0;
  if (-1 == getline(&linea, &longitud, stdin)) {
    if (errno) {
      enviarInstruccion(impresora, c_error(errno)); // TODO: enviar "getline" vía c_error
      muere();
    }
    return comando;
  }

  char * cursor = linea;

  avanzaHastaNo(isspace, &cursor);
  if (!*cursor) return comando;
  char * selector = cursor;
  avanzaHasta(isspace, &cursor);
  if (!*cursor) {
    comando.selector = decodificar(selector);
    return comando;
  }
  *cursor = '\0'; ++cursor;
  comando.selector = decodificar(selector);

  avanzaHastaNo(isspace, &cursor);
  if (!*cursor) return comando;
  comando.argumento1 = cursor;
  avanzaHasta(isspace, &cursor);
  if (!*cursor) return comando;
  *cursor = '\0'; ++cursor;

  avanzaHastaNo(isspace, &cursor);
  if (!*cursor) return comando;
  comando.argumento2 = cursor;
  avanzaHasta(isspace, &cursor);
  if (!*cursor) return comando;
  *cursor = '\0'; ++cursor;

  avanzaHastaNo(isspace, &cursor);
  if (!*cursor) return comando;

  comando.selector = SC_INVALIDO;
  return comando;
  // TODO: redirección
  //comando.fuente = ;
  //comando.destino = ;
}


void comandoInvalido(Comando * comando) {
  enviarInstruccion(impresora, c_imprime("Comando inválido\n"));
  comando->selector = SC_INVALIDO;
}

void repl() {
  Comando comando = fetch();

  switch (comando.selector) {
    case SC_INVALIDO:
      comandoInvalido(&comando);
      break;

    case SC_QUIT:
      if (comando.argumento1 || comando.argumento2) comandoInvalido(&comando);
      break;

    case SC_LS:
    case SC_CAT:
    case SC_FIND:
    case SC_RM:
    case SC_MKDIR:
    case SC_RMDIR:
      if (!comando.argumento1 || comando.argumento2) comandoInvalido(&comando);
      break;

    case SC_CP:
    case SC_MV:
      if (!comando.argumento1 || !comando.argumento2) comandoInvalido(&comando);
      break;
  }

  switch (comando.selector) {
    case SC_INVALIDO:
      break;

    case SC_QUIT:
      muere();
      break;

    case SC_RM:
      enviarInstruccion(raiz, c_rm(comando.argumento1));
      break;

    case SC_RMDIR:
      enviarInstruccion(raiz, c_rmdir(comando.argumento1));
      break;

    case SC_LS:
      enviarInstruccion(raiz, c_ls(comando.argumento1));
      break;

    default:
      enviarInstruccion(impresora, c_imprime("Por implementar\n")); //FIXME
      break;
  }

  if (SC_QUIT != comando.selector) repl();
}

int main(int argc, char * argv[]) {
  if (2 != argc) {
    fprintf(stderr, "Uso: %s <directorio>\n", argv[0]);
    exit(EX_USAGE);
  }

  impresora = crear(mkActor(imprimir, NULL));
  if (!impresora) {
    perror("crear");
    exit(EX_IOERR);
  }

  raiz = crear(mkActor(contratar, NULL));
  if (!raiz) {
    enviarInstruccion(impresora, c_muere());
    perror("crear");
    exit(EX_IOERR);
  }

  if (-1 == enviar(raiz, mkMensaje(strlen(argv[1]), argv[1]))) {
    perror("enviar");
    exit(EX_IOERR);
  }

  repl();

  muere();
  esperar(impresora);
  esperar(raiz);
  exit(EX_OK);
}