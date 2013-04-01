#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "actores.h"



Direccion frontController;
Direccion impresora;
Direccion raiz;
int soyRaiz;



typedef
  struct instruccion {
    enum selectorInstruccion
      { SI_INVALIDO
      , SI_MUERE
      , SI_TERMINE
      , SI_PROMPT
      , SI_IMPRIME
      , SI_IMPRIMEYPROMPT
      , SI_RM
      , SI_LS
      , SI_MKDIR
      , SI_RMDIR
      , SI_CAT
      , SI_FIND
      , SI_CP
      , SI_MV
      , SI_WRITE
      , SI_WRITEYBORRA
      , SI_ERROR
      , SI_ERRORYPROMPT
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
      struct dosPath {
        char * origen;
        char * origenAbsoluto;
        char * destino;
      } dosPath;
      struct datosUnPath {
        char * camino;
        ssize_t longitud;
        char * texto;
      } datosUnPath;
      struct datosDosPath {
        char * destino;
        char * origen;
        ssize_t longitud;
        char * texto;
      } datosDosPath;
      struct error {
        char * texto;
        int codigo;
      } error;
    } argumentos;
  }
  Instruccion
;

enum formatoInstruccion
  { FI_INVALIDO
  , FI_VACIO
  , FI_DATOS
  , FI_UNPATH
  , FI_DOSPATH
  , FI_DATOSUNPATH
  , FI_DATOSDOSPATH
  , FI_ERROR
  }
;

enum formatoInstruccion formatoInstruccion(enum selectorInstruccion selector) {
  switch (selector) {
    default:
    case SI_INVALIDO:
      return FI_INVALIDO;

    case SI_MUERE:
    case SI_TERMINE:
    case SI_PROMPT:
      return FI_VACIO;

    case SI_IMPRIME:
    case SI_IMPRIMEYPROMPT:
      return FI_DATOS;

    case SI_RM:
    case SI_LS:
    case SI_MKDIR:
    case SI_RMDIR:
    case SI_CAT:
      return FI_UNPATH;

    case SI_CP:
    case SI_MV:
    case SI_FIND:
      return FI_DOSPATH;

    case SI_WRITE:
      return FI_DATOSUNPATH;

    case SI_WRITEYBORRA:
      return FI_DATOSDOSPATH;

    case SI_ERROR:
    case SI_ERRORYPROMPT:
      return FI_ERROR;
  }
}

#if DEBUG
void mostrarInstruccion(Instruccion instruccion) {
  switch (instruccion.selector) {
    case SI_INVALIDO      : { printf("Instruccion { .selector = inválido"       " }\n"); } break;
    case SI_MUERE         : { printf("Instruccion { .selector = muere"          " }\n"); } break;
    case SI_TERMINE       : { printf("Instruccion { .selector = termine"        " }\n"); } break;
    case SI_PROMPT        : { printf("Instruccion { .selector = prompt"         " }\n"); } break;
    case SI_IMPRIME       : { printf("Instruccion { .selector = imprime"        ", .argumentos.datos = { .longitud = %d, .texto = %s } }\n", (int)instruccion.argumentos.datos.longitud, instruccion.argumentos.datos.texto); } break;
    case SI_IMPRIMEYPROMPT: { printf("Instruccion { .selector = imprimeyprompt" ", .argumentos.datos = { .longitud = %d, .texto = %s } }\n", (int)instruccion.argumentos.datos.longitud, instruccion.argumentos.datos.texto); } break;
    case SI_CAT           : { printf("Instruccion { .selector = cat"            ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_MKDIR         : { printf("Instruccion { .selector = mkdir"          ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_LS            : { printf("Instruccion { .selector = ls"             ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_RMDIR         : { printf("Instruccion { .selector = rmdir"          ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_RM            : { printf("Instruccion { .selector = rm"             ", .argumentos.unPath = { .camino = %s }\n", instruccion.argumentos.unPath.camino); } break;
    case SI_FIND          : { printf("Instruccion { .selector = find"           ", .argumentos.dosPath = { .origen = %s, .origenAbsoluto = %s, destino = %s }\n", instruccion.argumentos.dosPath.origen, instruccion.argumentos.dosPath.origenAbsoluto, instruccion.argumentos.dosPath.destino); } break;
    case SI_MV            : { printf("Instruccion { .selector = mv"             ", .argumentos.dosPath = { .origen = %s, .origenAbsoluto = %s, destino = %s }\n", instruccion.argumentos.dosPath.origen, instruccion.argumentos.dosPath.origenAbsoluto, instruccion.argumentos.dosPath.destino); } break;
    case SI_CP            : { printf("Instruccion { .selector = cp"             ", .argumentos.dosPath = { .origen = %s, .origenAbsoluto = %s, destino = %s }\n", instruccion.argumentos.dosPath.origen, instruccion.argumentos.dosPath.origenAbsoluto, instruccion.argumentos.dosPath.destino); } break;
    case SI_WRITE         : { printf("Instruccion { .selector = write"          ", .argumentos.datosUnPath = { .camino = %s, .longitud = %d, .texto = %s } }\n", instruccion.argumentos.datosUnPath.camino, (int)instruccion.argumentos.datosUnPath.longitud, instruccion.argumentos.datosUnPath.texto); } break;
    case SI_WRITEYBORRA   : { printf("Instruccion { .selector = writeyborra"    ", .argumentos.datosDosPath = { .destino = %s, .origen = %s, .longitud = %d, .texto = %s } }\n", instruccion.argumentos.datosDosPath.destino, instruccion.argumentos.datosDosPath.origen, (int)instruccion.argumentos.datosDosPath.longitud, instruccion.argumentos.datosDosPath.texto); } break;
    case SI_ERROR         : { printf("Instruccion { .selector = error"          ", .argumentos.error.codigo = %d }\n", instruccion.argumentos.error.codigo); } break;
    case SI_ERRORYPROMPT  : { printf("Instruccion { .selector = erroryprompt"   ", .argumentos.error.codigo = %d }\n", instruccion.argumentos.error.codigo); } break;
  }
}
#endif

Mensaje serializar(Instruccion instruccion) {
  Mensaje mensaje = {};

  agregaMensaje(&mensaje, &instruccion.selector, sizeof(enum selectorInstruccion));
  switch (formatoInstruccion(instruccion.selector)) {
    case FI_DATOS:
      agregaMensaje(&mensaje, &instruccion.argumentos.datos.longitud             , sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datos.texto                 , instruccion.argumentos.datos.longitud * sizeof(char));
      break;

    case FI_UNPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.unPath.camino               , (1 + strlen(instruccion.argumentos.unPath.camino)) * sizeof(char));
      break;

    case FI_DOSPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.origen              , (1 + strlen(instruccion.argumentos.dosPath.origen)) * sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.origenAbsoluto      , (1 + strlen(instruccion.argumentos.dosPath.origenAbsoluto)) * sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.destino             , (1 + strlen(instruccion.argumentos.dosPath.destino)) * sizeof(char));
      break;

    case FI_DATOSUNPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.datosUnPath.camino          , (1 + strlen(instruccion.argumentos.datosUnPath.camino)) * sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.datosUnPath.longitud       , sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datosUnPath.texto           , instruccion.argumentos.datosUnPath.longitud * sizeof(char));
      break;

    case FI_DATOSDOSPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.datosDosPath.destino        , (1 + strlen(instruccion.argumentos.datosDosPath.destino)) * sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.datosDosPath.origen         , (1 + strlen(instruccion.argumentos.datosDosPath.origen)) * sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.datosDosPath.longitud      , sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datosDosPath.texto          , instruccion.argumentos.datosDosPath.longitud * sizeof(char));
      break;

    case FI_ERROR:
      agregaMensaje(&mensaje, instruccion.argumentos.error.texto                 , (1 + strlen(instruccion.argumentos.error.texto)) * sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.error.codigo               , sizeof(int));
      break;

    default: break;
  }

  return mensaje;
}

Instruccion deserializar(Mensaje mensaje) {
  Instruccion instruccion = { .selector = SI_INVALIDO };
  if (!mensaje.contenido) return instruccion;

  instruccion.selector = *(enum selectorInstruccion *)mensaje.contenido;
  mensaje.contenido += sizeof(enum selectorInstruccion);
  switch (formatoInstruccion(instruccion.selector)) {
    case FI_DATOS:
      instruccion.argumentos.datos.longitud               = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datos.texto                  = (char *)    mensaje.contenido; mensaje.contenido += instruccion.argumentos.datos.longitud * sizeof(char);
      break;

    case FI_UNPATH:
      instruccion.argumentos.unPath.camino                = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      break;

    case FI_DOSPATH:
      instruccion.argumentos.dosPath.origen               = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.dosPath.origenAbsoluto       = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.dosPath.destino              = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      break;

    case FI_DATOSUNPATH:
      instruccion.argumentos.datosUnPath.camino           = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.datosUnPath.longitud         = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datosUnPath.texto            = (char *)    mensaje.contenido; mensaje.contenido += instruccion.argumentos.datosUnPath.longitud * sizeof(char);
      break;

    case FI_DATOSDOSPATH:
      instruccion.argumentos.datosDosPath.destino         = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.datosDosPath.origen          = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.datosDosPath.longitud        = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datosDosPath.texto           = (char *)    mensaje.contenido; mensaje.contenido += instruccion.argumentos.datosDosPath.longitud * sizeof(char);
      break;

    case FI_ERROR:
      instruccion.argumentos.error.texto                  = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.error.codigo                 = *(int *)    mensaje.contenido; mensaje.contenido += sizeof(int);
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

void orden(Instruccion instruccion) {
  enviarInstruccion(frontController, instruccion);
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

Instruccion instruccionDosPath(enum selectorInstruccion selector, char * origen, char * origenAbsoluto, char * destino) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos =
      { .dosPath.origen = origen
      , .dosPath.origenAbsoluto = origenAbsoluto
      , .dosPath.destino = destino
      }
    }
  ;
  return instruccion;
}

Instruccion instruccionDatosUnPath(enum selectorInstruccion selector, char * camino, ssize_t longitud, char * texto) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.datosUnPath =
      { .camino = camino
      , .longitud = longitud
      , .texto = texto
      }
    }
  ;
  return instruccion;
}

Instruccion instruccionDatosDosPath(enum selectorInstruccion selector, char * destino, char * origen, ssize_t longitud, char * texto) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.datosDosPath =
      { .destino = destino
      , .origen = origen
      , .longitud = longitud
      , .texto = texto
      }
    }
  ;
  return instruccion;
}

Instruccion instruccionError(enum selectorInstruccion selector, char * texto, int codigo) {
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.error =
      { .texto = texto
      , .codigo = codigo
      }
    }
  ;
  return instruccion;
}

Instruccion c_imprimeReal       (int longitud, char * texto)                                    { return instruccionDatos       (SI_IMPRIME       , longitud     , texto            ); }
Instruccion c_imprimeRealyprompt(int longitud, char * texto)                                    { return instruccionDatos       (SI_IMPRIMEYPROMPT, longitud     , texto            ); }
Instruccion c_muere             (void)                                                          { return instruccionSimple      (SI_MUERE                                           ); }
Instruccion c_termine           (void)                                                          { return instruccionSimple      (SI_TERMINE                                         ); }
Instruccion c_prompt            (void)                                                          { return instruccionSimple      (SI_PROMPT                                          ); }
Instruccion c_imprime           (char * texto)                                                  { return instruccionDatos       (SI_IMPRIME       , strlen(texto), texto            ); }
Instruccion c_imprimeyprompt    (char * texto)                                                  { return instruccionDatos       (SI_IMPRIMEYPROMPT, strlen(texto), texto            ); }
Instruccion c_ls                (char * camino)                                                 { return instruccionUnPath      (SI_LS            , camino                          ); }
Instruccion c_mkdir             (char * camino)                                                 { return instruccionUnPath      (SI_MKDIR         , camino                          ); }
Instruccion c_rm                (char * camino)                                                 { return instruccionUnPath      (SI_RM            , camino                          ); }
Instruccion c_rmdir             (char * camino)                                                 { return instruccionUnPath      (SI_RMDIR         , camino                          ); }
Instruccion c_cat               (char * camino)                                                 { return instruccionUnPath      (SI_CAT           , camino                          ); }
Instruccion c_find              (char * origen, char * origenAbsoluto, char * destino)          { return instruccionDosPath     (SI_FIND          , origen, origenAbsoluto, destino ); }
Instruccion c_cp                (char * origen, char * origenAbsoluto, char * destino)          { return instruccionDosPath     (SI_CP            , origen, origenAbsoluto, destino ); }
Instruccion c_mv                (char * origen, char * origenAbsoluto, char * destino)          { return instruccionDosPath     (SI_MV            , origen, origenAbsoluto, destino ); }
Instruccion c_write             (char * camino, ssize_t longitud, char * texto)                 { return instruccionDatosUnPath (SI_WRITE         , camino, longitud, texto         ); }
Instruccion c_writeyborra       (char * destino, char * origen, ssize_t longitud, char * texto) { return instruccionDatosDosPath(SI_WRITEYBORRA   , destino, origen, longitud, texto); }
Instruccion c_error             (char * texto, int codigo)                                      { return instruccionError       (SI_ERROR         , texto, codigo                   ); }
Instruccion c_erroryprompt      (char * texto, int codigo)                                      { return instruccionError       (SI_ERRORYPROMPT  , texto, codigo                   ); }

Instruccion (*constructorInstruccion(enum selectorInstruccion selectorInstruccion))() {
  switch (selectorInstruccion) {
    case SI_MUERE         : return c_muere         ;
    case SI_TERMINE       : return c_termine       ;
    case SI_PROMPT        : return c_prompt        ;
    case SI_IMPRIME       : return c_imprime       ;
    case SI_IMPRIMEYPROMPT: return c_imprimeyprompt;
    case SI_LS            : return c_ls            ;
    case SI_MKDIR         : return c_mkdir         ;
    case SI_RM            : return c_rm            ;
    case SI_RMDIR         : return c_rmdir         ;
    case SI_FIND          : return c_find          ;
    case SI_CAT           : return c_cat           ;
    case SI_MV            : return c_mv            ;
    case SI_CP            : return c_cp            ;
    case SI_WRITE         : return c_write         ;
    case SI_WRITEYBORRA   : return c_writeyborra   ;
    case SI_ERROR         : return c_error         ;
    case SI_ERRORYPROMPT  : return c_erroryprompt  ;
    default: return NULL;
  }
}



void muere() {
  orden(c_muere());
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

void liberarLibreta(void) {
  if (!libreta) return;
  struct libreta * siguiente = libreta->siguiente;
  free(libreta->nombre);
  liberaDireccion(libreta->direccion);
  free(libreta);
  libreta = siguiente;
  liberarLibreta();
}

void eliminarCeldaLibreta(char const * nombre) {
  struct libreta ** actual;
  for (actual = &libreta; *actual; actual = &(*actual)->siguiente) {
    if (!strcmp((*actual)->nombre, nombre)) {
      struct libreta * respaldo = (*actual)->siguiente;
      free((*actual)->nombre);
      enviarInstruccion((*actual)->direccion, c_muere());
      liberaDireccion((*actual)->direccion);
      free(*actual);
      *actual = respaldo;
      return;
    };
  }
}

Direccion buscarLibreta(char const * nombre) {
  struct libreta * entLibreta;
  for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
    if (!strcmp(entLibreta->nombre, nombre)) return entLibreta->direccion;
  }

  return NULL;
}

int contarLibreta(struct libreta * libreta, int acumulador) {
  if (!libreta) return acumulador;
  if (strcmp(libreta->nombre, ".") && strcmp(libreta->nombre, "..")) ++acumulador;
  return contarLibreta(libreta->siguiente, acumulador);
}

int numeroHijos(void) {
  return contarLibreta(libreta, 0);
}



void procesarSubdirectorio(char const * dir);

void do_mkdir(Instruccion instruccion) {
  if (-1 == mkdir(instruccion.argumentos.unPath.camino, S_IRWXU | S_IRWXG | S_IRWXO)) {
    switch (errno) {
      case EFAULT:
      case ENOMEM:
        orden(c_error("mkdir", errno));
        muere();
        return;

      default:
        orden(c_erroryprompt("mkdir", errno));
        return;
    }
  }
  procesarSubdirectorio(instruccion.argumentos.unPath.camino);
  orden(c_prompt());
}

void do_rm(Instruccion instruccion) {
  if (-1 == unlink(instruccion.argumentos.unPath.camino)) {
    switch (errno) {
      case EFAULT:
      case ENOMEM:
        orden(c_error("rm: unlink", errno));
        muere();
        return;

      default:
        orden(c_erroryprompt("rm: unlink", errno));
        return;
    }
  }
  orden(c_prompt());
}

void do_rmdir(Instruccion instruccion) {
  if (-1 == rmdir(instruccion.argumentos.unPath.camino)) {
    switch (errno) {
      case EFAULT:
      case ENOMEM:
        orden(c_error("rmdir", errno));
        muere();
        return;

      default:
        orden(c_erroryprompt("rmdir", errno));
        return;
    }
  }
  eliminarCeldaLibreta(instruccion.argumentos.unPath.camino);
  orden(c_prompt());
}

char * chomp(char * s) {
  char * pos = strrchr(s, '\n');
  if (pos) *pos = '\0';
  return s;
}

void base_ls(char const * camino) {
  struct stat stats;
  if (-1 == stat(camino, &stats)) {
    switch (errno) {
      case EFAULT:
      case ENOMEM:
        orden(c_error("ls: stat", errno));
        muere();
        return;

      default:
        orden(c_erroryprompt("ls: stat", errno));
        return;
    }
  }

  char * user;
  if (!getpwuid(stats.st_uid)) {
    if (-1 == asprintf(&user, "%s", getpwuid(stats.st_uid)->pw_name)) {
      orden(c_erroryprompt("ls: getpwuid", errno));
      return;
    }
  } else {
    if (-1 == asprintf(&user, "%d", stats.st_uid)) {
      orden(c_erroryprompt("ls: asprintf", errno));
      return;
    }
  }

  char * group;
  if (!getgrgid(stats.st_gid)) {
    if (0 > asprintf(&group, "%s", getgrgid(stats.st_gid)->gr_name)) {
      orden(c_erroryprompt("ls: getgrid", errno));
      free(user);
      return;
    }
  } else {
    if (0 > asprintf(&group, "%d", stats.st_gid)) {
      orden(c_erroryprompt("ls: asprintf", errno));
      free(user);
      return;
    }
  }

  char * texto;
  if (0 >
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
  ) {
    orden(c_erroryprompt("ls: asprintf", errno));
  } else {
    orden(c_imprime(texto));
    free(texto);
  }
  free(user);
  free(group);
}

int filterLs(struct dirent const * dir) {
  base_ls(dir->d_name);
  return 0;
}

void do_ls(Instruccion instruccion) {
  struct stat stats;
  if (-1 == stat(instruccion.argumentos.unPath.camino, &stats)) {
    switch (errno) {
      case EFAULT:
      case ENOMEM:
        orden(c_error("ls: stat", errno));
        muere();
        return;

      default:
        orden(c_erroryprompt("ls: stat", errno));
        return;
    }
  }

  if (S_ISDIR(stats.st_mode)) {
    struct dirent ** listaVacia;
    if (-1 == scandir(instruccion.argumentos.unPath.camino, &listaVacia, filterLs, NULL)) {
      if (ENOMEM == errno) {
        orden(c_error("scandir", errno));
        muere();
      } else {
        orden(c_erroryprompt("scandir", errno));
      }
      return;
    }
  } else {
    base_ls(instruccion.argumentos.unPath.camino);
  }
  orden(c_prompt());

}

void conContenido(char * camino, void (*funcion)(int longitud, char * buffer, void * datos), void * datos) {
  int fd = open(camino, O_RDONLY);
  if (-1 == fd) {
    switch (errno) {
      case EACCES:
      case EINTR:
      case ELOOP:
      case ENAMETOOLONG:
      case ENODEV:
      case ENOENT:
      case ENOTDIR:
      case EOVERFLOW:
      case EPERM:
        orden(c_erroryprompt("open", errno));
        break;

      default:
        orden(c_error("open", errno));
        muere();
        break;
    }
    return;
  }

  int longitud = 0;
  char * buffer = NULL;
  while (1) {
    char * nuevoBuffer = realloc(buffer, longitud + 1024);
    if (!nuevoBuffer) {
      free(buffer);
      //realloc define errno si falla y entonces llamamos a muere
      orden(c_error("realloc", errno));
      close(fd);
      muere();
      return;
    }
    buffer = nuevoBuffer;

    int leido;
    while (1) {
      leido = read(fd, buffer + longitud, 1024);
      if (-1 == leido) {
        switch (errno) {
          case EISDIR:
          case EINVAL:
          case EINTR:
            orden(c_erroryprompt("read", errno));
            break;

          default:
            orden(c_error("read", errno));
            muere();
            break;
        }
        free(buffer);
        close(fd);
        return;
      }
      break;
    }
    if (0 == leido) break;
    longitud += leido;
  }

  close(fd);

  {
    char * nuevoBuffer = realloc(buffer, longitud);
    if (!nuevoBuffer) {
      free(buffer);
      //realloc define errno si falla y entonces llamamos a muere
      orden(c_error("realloc", errno));
      muere();
      return;
    }
    buffer = nuevoBuffer;
  }

  funcion(longitud, buffer, datos);

  free(buffer);
}

void hacerCat(int longitud, char * buffer, void * datos) {
  orden(c_imprimeRealyprompt(longitud, buffer));
}

void do_cat(Instruccion instruccion) {
  conContenido(instruccion.argumentos.unPath.camino, hacerCat, NULL);
}

void hacerCP(int longitud, char * buffer, void * datos) {
  Instruccion * instruccion = (Instruccion *)datos;
  orden(c_write(instruccion->argumentos.dosPath.destino, longitud, buffer));
}

void hacerMV(int longitud, char * buffer, void * datos) {
  Instruccion * instruccion = (Instruccion *)datos;
  orden(c_writeyborra(instruccion->argumentos.dosPath.destino, instruccion->argumentos.dosPath.origenAbsoluto, longitud, buffer));
}

void do_cp(Instruccion instruccion) {
  conContenido(instruccion.argumentos.dosPath.origen, hacerCP, &instruccion);
}

void do_mv(Instruccion instruccion) {
  conContenido(instruccion.argumentos.dosPath.origen, hacerMV, &instruccion);
}

void escribe(char * camino, int longitud, char * texto) {
  int fd = open(camino, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
  if (-1 == fd) {
    switch (errno) {
      case EACCES:
      case EINTR:
      case ELOOP:
      case ENAMETOOLONG:
      case ENODEV:
      case ENOENT:
      case ENOTDIR:
      case EOVERFLOW:
      case EPERM:
        orden(c_erroryprompt("escribe: open", errno));
        break;

      default:
        orden(c_error("escribe: open", errno));
        muere();
        break;
    }
    return;
  }

  while (1) {
    while (longitud > 0) {
      int escrito = write(fd, texto, longitud);
      if (-1 == escrito) {
        if (EINTR == errno) continue;
        //puede fallar por cualquiera de las razones que falla write excepto EINTR, en ese caso se repite el ciclo hasta que haga algo
        switch (errno) {
          case EFBIG:
          case EIO:
          case ENOSPC:
          case EPIPE:
            orden(c_erroryprompt("write", errno));
            break;

          default:
            orden(c_error("write", errno));
            muere();
            break;
        }
        return;
      }
      longitud -= escrito;
      texto += escrito;
    }
    break;
  }

  close(fd);
}

void do_write(Instruccion instruccion) {
  escribe(instruccion.argumentos.datosUnPath.camino, instruccion.argumentos.datosUnPath.longitud, instruccion.argumentos.datosUnPath.texto);
  orden(c_prompt());
}

void do_writeyborra(Instruccion instruccion) {
  escribe(instruccion.argumentos.datosDosPath.destino, instruccion.argumentos.datosDosPath.longitud, instruccion.argumentos.datosDosPath.texto);
  orden(c_rm(instruccion.argumentos.datosDosPath.origen));
}



void descender(void (*accion)(Instruccion), Instruccion instruccion) {
  char * colaDeCamino = strchr(instruccion.argumentos.unPath.camino, '/');
  if (!colaDeCamino) {
    accion(instruccion);
  } else {
    *colaDeCamino = '\0';
    char * cabezaDeCamino = instruccion.argumentos.unPath.camino;
    instruccion.argumentos.unPath.camino = 1 + colaDeCamino; // OJO: esto funciona por el union, pero es medio arriesgado; la idea es que el path que se consume con descender es el que esté al principio de los argumentos, y aunque acá se modifica en el formato unPath, también se aplica al formato dosPath, datosUnPath y datosDosPath.  Quizá sería más seguro un switch o algo.
    Direccion subdirectorio;
    if ((subdirectorio = buscarLibreta(cabezaDeCamino))) {
      enviarInstruccion(subdirectorio, instruccion);
    } else {
      orden(c_erroryprompt("descender", ENOENT));
    }
  }
}

int filter(struct dirent const * dir);

Instruccion * findActual;

int buscar(struct dirent const * dirent) {
  struct stat infoArchivo;
  char const * dir = dirent->d_name;
  if (!strcmp(dir, ".") || !strcmp(dir, "..")) return 0;
  if (-1 == stat(dir, &infoArchivo)) {
    switch (errno) {
      case EFAULT:
      case ENOMEM:
      case EBADF:
        orden(c_error("stat", errno));
        muere();
        break;

      default: break;
    }
    return 0;
  }

  char * camino;
  if (-1 == asprintf(&camino, "%s/%s", findActual->argumentos.dosPath.origen, dirent->d_name)) {
    return 0;
  }

  if (strstr(camino, findActual->argumentos.dosPath.destino)) {
    char * caminoNL;
    if (-1 == asprintf(&caminoNL, "%s\n", camino)) {
      return 0;
    }
    orden(c_imprime(caminoNL));
    free(caminoNL);
  }

  if (S_ISDIR(infoArchivo.st_mode)) {
    Instruccion instruccionSubdirectorio = *findActual;
    instruccionSubdirectorio.argumentos.dosPath.origen = camino;
    enviarInstruccion(buscarLibreta(dirent->d_name), instruccionSubdirectorio);
  }

  free(camino);
  return 0;
}



Actor despachar(Mensaje mensaje, void * datos);

Actor esperarFind(Mensaje mensaje, void * datos) {
  Instruccion instruccion = deserializar(mensaje);

  int * numHijos = (int *)datos;

  switch (instruccion.selector) {
    case SI_MUERE: {
      struct libreta * entLibreta;
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        enviar(entLibreta->direccion, mensaje);
      }
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        esperar(entLibreta->direccion);
      }
      liberarLibreta();
      free(mensaje.contenido);
      free(numHijos);
      return finActor();
    }

    case SI_INVALIDO:
      --*numHijos;
      free(mensaje.contenido);

      if (0 != *numHijos) {
        return mkActor(esperarFind, numHijos);
      }

      if (soyRaiz) orden(c_prompt());
      else enviar(buscarLibreta(".."), mkMensaje(0, NULL));

      free(numHijos);
      return mkActor(despachar, NULL);

    default:
      orden(c_imprime("esperarFind: mensaje inesperado\n"));
      muere();
      return mkActor(esperarFind, numHijos);
  }
}

Actor despachar(Mensaje mensaje, void * datos) {
  Instruccion instruccion = deserializar(mensaje);

  switch (instruccion.selector) {
    case SI_MUERE: {
      struct libreta * entLibreta;
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        enviar(entLibreta->direccion, mensaje);
      }
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        esperar(entLibreta->direccion);
      }
      liberarLibreta();
      free(mensaje.contenido);
      return finActor();
    }

    case SI_MKDIR      : descender(do_mkdir      , instruccion); break;
    case SI_RM         : descender(do_rm         , instruccion); break;
    case SI_RMDIR      : descender(do_rmdir      , instruccion); break;
    case SI_LS         : descender(do_ls         , instruccion); break;
    case SI_CAT        : descender(do_cat        , instruccion); break;
    case SI_CP         : descender(do_cp         , instruccion); break;
    case SI_MV         : descender(do_mv         , instruccion); break;
    case SI_WRITE      : descender(do_write      , instruccion); break;
    case SI_WRITEYBORRA: descender(do_writeyborra, instruccion); break;

    case SI_FIND: {
      struct dirent ** listaVacia;
      findActual = &instruccion;
      if (-1 == scandir(".", &listaVacia, buscar, NULL)) {
        orden(c_error("scandir", errno));
        muere();
      }
      free(mensaje.contenido);
      int * hijosEsperar = malloc(sizeof(int));
      if (!hijosEsperar) {
        orden(c_error("malloc", errno));
        muere();
      }
      *hijosEsperar = numeroHijos();
      if (0 == *hijosEsperar) {
        if (soyRaiz) orden(c_prompt());
        else enviar(buscarLibreta(".."), mkMensaje(0, NULL));

        free(hijosEsperar);
        return mkActor(despachar, datos);
      }

      return mkActor(esperarFind, hijosEsperar);
    }

    default: break;
  }
  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

Actor contratar(Mensaje mensaje, void * datos) {
  if (!libreta) {
    soyRaiz = 1;
    char * puntopunto;
    asprintf(&puntopunto, "..");
    insertarLibreta(puntopunto, miDireccion());
  }

  char * punto;
  asprintf(&punto, ".");
  insertarLibreta(punto, miDireccion());

  if (-1 == chdir(mensaje.contenido)) {
    orden(c_error("chdir", errno));
    muere();
  } else {
    struct dirent ** listaVacia;
    if (-1 == scandir(".", &listaVacia, filter, NULL)) {
      orden(c_error("scandir", errno));
      muere();
    }
  }

  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

Actor manejarPapa(Mensaje mensaje, void * datos);

void procesarSubdirectorio(char const * dir) {
  struct stat infoArchivo;
  if (!strcmp(dir, ".") || !strcmp(dir, "..")) return;
  if (-1 == stat(dir, &infoArchivo)) {
    orden(c_error("stat", errno));
    muere();
    // TODO: hacer que scandir no continúe
  }

  if (S_ISDIR(infoArchivo.st_mode)) {
    char * nombre;
    asprintf(&nombre, "%s", dir);

    Direccion subdirectorio = crear(mkActor(manejarPapa, NULL));
    if (!subdirectorio) {
      orden(c_error("crear", errno));
      muere();
      return;
    }

    insertarLibreta(nombre, subdirectorio);

    int retorno = enviar
      ( subdirectorio
      , mkMensaje(strlen(dir) + 1, (char *)dir)
      )
    ;
    if (-1 == retorno) {
      orden(c_error("enviar", errno));
      muere();
      return;
    }
  }
}

int filter(struct dirent const * dir) {
  procesarSubdirectorio(dir->d_name);
  return 0;
}



Actor manejarPapa(Mensaje mensaje, void * datos) {
  liberarLibreta();

  soyRaiz = 0;
  char * puntopunto;
  asprintf(&puntopunto, "..");
  insertarLibreta(puntopunto, deserializarDireccion(mensaje));

  free(mensaje.contenido);
  return mkActor(contratar, datos);
}



Actor imprimir(Mensaje mensaje, void * datos) {
  Instruccion instruccion = deserializar(mensaje);

  switch (instruccion.selector) {
    case SI_MUERE:
      return finActor();

    case SI_IMPRIME:
      fwrite(instruccion.argumentos.datos.texto, sizeof(char), instruccion.argumentos.datos.longitud, stdout);
      fflush(stdout);
      break;

    case SI_IMPRIMEYPROMPT:
      fwrite(instruccion.argumentos.datos.texto, sizeof(char), instruccion.argumentos.datos.longitud, stdout);
      fflush(stdout);
      orden(c_prompt());
      break;

    case SI_ERROR:
      errno = instruccion.argumentos.error.codigo;
      perror(instruccion.argumentos.error.texto);
      break;

    case SI_ERRORYPROMPT:
      errno = instruccion.argumentos.error.codigo;
      perror(instruccion.argumentos.error.texto);
      orden(c_prompt());
      break;

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
      , SC_NADA
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

    char * origen;
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
      orden(c_error("getline", errno));
      muere();
    }
    comando.selector = SC_QUIT;
    orden(c_imprime("quit\n"));
    return comando;
  }

  char * cursor = linea;

  // FIXME: no sirve ignorar espacios

  avanzaHastaNo(isspace, &cursor);
  if (!*cursor) {
    comando.selector = SC_NADA;
    return comando;
  }
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
  //comando.origen = ;
  //comando.destino = ;
}


void comandoInvalido(Comando * comando) {
  orden(c_imprimeyprompt("Comando inválido\n"));
  comando->selector = SC_INVALIDO;
}

void prompt() {
  Comando comando = fetch();

  switch (comando.selector) {
    case SC_INVALIDO:
      comandoInvalido(&comando);
      break;

    case SC_NADA:
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
    case SC_INVALIDO: break;

    case SC_NADA:
      orden(c_imprimeyprompt(""));
      break;

    case SC_QUIT: muere(); break;

    case SC_MKDIR: orden(c_mkdir(comando.argumento1)); break;
    case SC_RM   : orden(c_rm   (comando.argumento1)); break;
    case SC_RMDIR: orden(c_rmdir(comando.argumento1)); break;
    case SC_LS   : orden(c_ls   (comando.argumento1)); break;
    case SC_CAT  : orden(c_cat  (comando.argumento1)); break;

    case SC_CP: orden(c_cp(comando.argumento1, comando.argumento1, comando.argumento2)); break;
    case SC_MV: orden(c_mv(comando.argumento1, comando.argumento1, comando.argumento2)); break;

    case SC_FIND: orden(c_find(".", ".", comando.argumento1)); break;

    default:
      orden(c_imprimeyprompt("Instruccion no encontrada\n"));
      break;
  }
}



Actor accionFrontController(Mensaje mensaje, void * datos) {
  Instruccion instruccion = deserializar(mensaje);

  switch (instruccion.selector) {
    case SI_MUERE:
      enviar(raiz, mensaje);
      enviar(impresora, mensaje);
      esperar(raiz);
      esperar(impresora);
      free(mensaje.contenido);
      return finActor();

    case SI_IMPRIME:
    case SI_IMPRIMEYPROMPT:
    case SI_ERROR:
    case SI_ERRORYPROMPT:
      enviar(impresora, mensaje);
      break;

    case SI_PROMPT:
      prompt();
      break;

    default:
      enviar(raiz, mensaje);
      break;
  }

  free(mensaje.contenido);
  return mkActor(accionFrontController, datos);
}

Actor inicioFrontController(Mensaje mensaje, void * datos) {
  frontController = miDireccion();

  impresora = crearSinEnlazar(mkActor(imprimir, NULL));
  if (!impresora) {
    muere();
  }

  raiz = crearSinEnlazar(mkActor(contratar, NULL));
  if (!raiz) {
    orden(c_error("crearSinEnlazar", errno));
    muere();
  }

  if (-1 == enviar(raiz, mensaje)) {
    orden(c_error("enviar", errno));
    muere();
  }

  orden(c_prompt());

  free(mensaje.contenido);
  return mkActor(accionFrontController, datos);
}



void handler(int s) {
  printf("\nNo es posible matarmme con una señal jijiji >:3 tienes que usar quit\nchelito: ");
  fflush(stdout);
}

int main(int argc, char * argv[]) {
  if (2 != argc) {
    fprintf(stderr, "Uso: %s <directorio>\n", argv[0]);
    exit(EX_USAGE);
  }

  if (SIG_ERR == signal(SIGQUIT, SIG_IGN)) {
    perror("signal");
    exit(EX_OSERR);
  }

  if (SIG_ERR == signal(SIGINT, SIG_IGN)) {
    perror("signal");
    exit(EX_OSERR);
  }

  frontController = crearSinEnlazar(mkActor(inicioFrontController, NULL));
  if (!frontController) {
    perror("crear");
    exit(EX_IOERR);
  }

  if (-1 == enviar(frontController, mkMensaje(strlen(argv[1]) + 1, argv[1]))) {
    perror("enviar");
    exit(EX_IOERR);
  }

  if (SIG_ERR == signal(SIGQUIT, handler)) {
    perror("signal");
    exit(EX_OSERR);
  }

  if (SIG_ERR == signal(SIGINT, handler)) {
    perror("signal");
    exit(EX_OSERR);
  }

  esperar(frontController);
  exit(EX_OK);
}
