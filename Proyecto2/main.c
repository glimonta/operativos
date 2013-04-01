/**
 * @file main.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de un shell que permite
 * realizar operaciones basicas sobre archivos contenidos
 * en una jerarquia de directorios. Los directorios estan
 * representados por actores y la comunicacion entre ellos
 * se hace mediante el envio de mensajes.
 *
 * Las operaciones implementadas en este shell son: ls,
 * cat, cp, mv, find, rm, mkdir, rmdir y quit.
 *
 */
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


/** Actores principales
 * Existen tres actores principales en el sistema:
 * el front controller; este se encarga de distribuir los mensajes
 * recibidos y enviar mensajes a los actores correspondientes para
 * ejecutar acciones del shell, es el coordinador, luego se tiene
 * a la impresora; este actor se encarga de hacer todas las tareas
 * de impresion en pantalla y finalmente se encuentra la raiz; este
 * es el actor asociado al directorio raiz de la jerarquia.
 * Se crean variables globales con las direcciones de estos actores
 * ya que cualquier actor debe ser capaz de enviarle mensajes a
 * cualquiera de estos tres entes principales.
 */
Direccion frontController;
Direccion impresora;
Direccion raiz;
// Adicionalmente se crea una variable global que indica si el actor
// actual está asociado a un directorio que es la raiz.
int soyRaiz;



/**
 * Se define un nuevo tipo que guarda la estructura de una instruccion
 * del shell. Este contiene las distintas instrucciones que se pueden
 * ejecutar. Se conforma por un selector que indica que tipo de instruccion
 * es y segun esto se determina el siguiente contenidod el tipo. Se tiene un
 * union ya que según el tipo de instruccion que sea podemos necesitar distintos
 * campos dentro de los argumentos dependiendo de si necesitamos datos, un path,
 * dos paths, datos y un path o datos y dos paths.
 */
typedef
  struct instruccion {
    /**
     * Tipo enumerado que guarda un identificador por cada tipo
     * de instruccion que pueda ejecutarse.
     */
    enum selectorInstruccion
      { SI_INVALIDO       /**< Instruccion invalida */
      , SI_MUERE          /**< Instruccion que indica que muera */
      , SI_TERMINE        /**< Instruccion que indica que termino */
      , SI_PROMPT         /**< Instruccion que manda a impirmir un nuevo prompt */
      , SI_LSALL          /**< Instruccion que manda a hacer ls a todos los archivos de un directorio */
      , SI_IMPRIME        /**< Instruccion que manda a impirmir en pantalla */
      , SI_IMPRIMEYPROMPT /**< Instruccion que manda a impirmir en pantalla y luego un nuevo prompt */
      , SI_RM             /**< Instruccion que manda a hacer rm */
      , SI_LS             /**< Instruccion que manda a hacer ls */
      , SI_MKDIR          /**< Instruccion que manda a hacer mkdir */
      , SI_RMDIR          /**< Instruccion que manda a hacer rmdir */
      , SI_CAT            /**< Instruccion que manda a hacer cat */
      , SI_FIND           /**< Instruccion que manda a hacer find */
      , SI_CP             /**< Instruccion que manda a hacer cp */
      , SI_MV             /**< Instruccion que manda a hacer mv */
      , SI_WRITE          /**< Instruccion que manda a hacer write */
      , SI_WRITEYBORRA    /**< Instruccion que manda a hacer write y borra un archivo luego */
      , SI_ERROR          /**< Instruccion indica que hubo un error */
      , SI_ERRORYPROMPT   /**< Instruccion indica que hubo un error y luego imprime un prompt */
      }
      selector
    ;

    /**
     * Este union tiene los campos de los distintos argumentos que
     * podemos tener segun el tipo de instruccion.
     */
    union argumentos {
      /**
       * Estructura que guarda los argumentos cuando las instrucciones
       * necesitan estar acompañadas por datos.
       */
      struct datos {
        ssize_t longitud; /**< indica la longitud del texto contenido */
        char * texto;     /**< guarda el texto (datos) */
      } datos;
      /**
       * Estructura que guarda los argumentos cuando las instrucciones
       * necesitan estar acompañadas por un path.
       */
      struct unPath {
        char * camino; /**< Path necesario para la instruccion */
      } unPath;
      /**
       * Estructura que guarda los argumentos cuando las instrucciones
       * necesitan dos paths
       */
      struct dosPath {
        char * origen;         /**< Contiene el path de origen */
        char * origenAbsoluto; /**< Contiene la ruta absoluta del origen */
        char * destino;        /**< Contiene el path de destino */
      } dosPath;
      /**
       * Estructura que guarda los argumentos cuando las instrucciones necesitan
       * un path y deben estar acompañadas por datos.
       */
      struct datosUnPath {
        char * camino;    /**< Contiene el path */
        ssize_t longitud; /**< Contiene la longitud del texto contenido */
        char * texto;     /**< Contiene el texto (datos) */
      } datosUnPath;
      /**
       * Estructura que guarda los argumentos cuando las instrucciones
       * necesitan dos paths y deben estar acompañadas por datos */
      struct datosDosPath {
        char * destino;         /**< Contiene el path de destino */
        char * origen;          /**< Contiene el path de origen */
        ssize_t longitud;       /**< Contiene la longitud del texto contenido */
        char * texto;           /**< Contiene el texto (datos) */
      } datosDosPath;
      /**
       * Estructura que guarda los argumentos cuando la instruccioni es de
       * error.
       */
      struct error {
        char * texto; /**< Texto que indica que fallo */
        int codigo;   /**< Codigo de error que arroja */
      } error;
    } argumentos;
  }
  Instruccion
;


/**
 * Tipo enumerado que se utiliza para diferenciar
 * entre los distintos tipos de formatos de instruccion.
 */
enum formatoInstruccion
  { FI_INVALIDO     /**< Formato de instruccion invalida                                     */
  , FI_VACIO        /**< Formato de instruccion vacia                                        */
  , FI_DATOS        /**< Formato de instruccion que utiliza datos como argumento             */
  , FI_UNPATH       /**< Formato de instruccion que utiliza argumentos con un path           */
  , FI_DOSPATH      /**< Formato de instruccion que utiliza argumentos con dos paths         */
  , FI_DATOSUNPATH  /**< Formato de instruccion que utiliza argumentos con un path y datos   */
  , FI_DATOSDOSPATH /**< Formato de instruccion que utiliza argumentos con dos paths y datos */
  , FI_ERROR        /**< Formato de instruccion para error                                   */
  }
;

/**
 * Se encarga de indicar el formato de una instruccion dado su selector.
 * @param selector Selector de la instruccion de la que queremos saber su formato.
 * @return Retorna el formato de instruccion que tiene una instruccion con el
 * selector dado.
 */
enum formatoInstruccion formatoInstruccion(enum selectorInstruccion selector) {
  // Hacemos un switch entre los distintos tipos de valores que puede
  // tomar el selector.
  switch (selector) {
    // Cuando es un selector invalido y en el caso por defecto
    // se devuelve el formato de instruccion invalido.
    default:
    case SI_INVALIDO:
      return FI_INVALIDO;

    // Cuando es un selector de las instrucciones muere,
    // termine y prompt se devuelve el formato de instruccion vacio.
    case SI_MUERE:
    case SI_TERMINE:
    case SI_PROMPT:
    case SI_LSALL:
      return FI_VACIO;

    // Cuando es un selector de imprime o imprime y prompt, se devuelve
    // el formato de instruccion que incluye datos.
    case SI_IMPRIME:
    case SI_IMPRIMEYPROMPT:
      return FI_DATOS;

    // Cuando es un selector de las instrucciones rm, ls, mkdir,
    // rmdir o cat se devuelve el formato de instruccion que incluye
    // un solo path.
    case SI_RM:
    case SI_LS:
    case SI_MKDIR:
    case SI_RMDIR:
    case SI_CAT:
      return FI_UNPATH;

    // Cuando es un selector de las instrucciones cp, mv o find se
    // devuelve el formato de instruccion que incluye dos paths
    case SI_CP:
    case SI_MV:
    case SI_FIND:
      return FI_DOSPATH;

    // Cuando es un selector de la instruccion write, se devuelve
    // el formato de instruccion que incluye ademas de un path
    // los datos que debe escribir.
    case SI_WRITE:
      return FI_DATOSUNPATH;

    // Cuando es un selector de la instruccion write y borra, se
    // devuelve el formato de instruccion que incluye ademas de los
    // datos a escribir, dos paths.
    case SI_WRITEYBORRA:
      return FI_DATOSDOSPATH;

    // Cuando es un selector de la instruccion error o error y prompt,
    // se devuelve el formato de instruccion de error.
    case SI_ERROR:
    case SI_ERRORYPROMPT:
      return FI_ERROR;
  }
}

#if DEBUG
/**
 * Se encarga de mostrar en pantalla la instruccion y lo que contiene
 * en sus argumentos. Se utiliza al momento de debuggear el codigo.
 * @param instruccion Instruccion cuyo contenido queremos conocer.
 */
void mostrarInstruccion(Instruccion instruccion) {
  // Segun cual sea el selector de la instruccion se imprime en pantalla
  // la informacion del selector y la informacion de sus argumentos.
  switch (instruccion.selector) {
    case SI_INVALIDO      : { printf("Instruccion { .selector = inválido"       " }\n"); } break;
    case SI_MUERE         : { printf("Instruccion { .selector = muere"          " }\n"); } break;
    case SI_TERMINE       : { printf("Instruccion { .selector = termine"        " }\n"); } break;
    case SI_PROMPT        : { printf("Instruccion { .selector = prompt"         " }\n"); } break;
    case SI_LSALL         : { printf("Instruccion { .selector = lsall"          " }\n"); } break;
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

/**
 * Se encarga de serializar una instruccion para que
 * la misma pueda ser enviada como un mensaje a un actor.
 * @param instruccion Instruccion que queremos serializar
 * @return retorna el mensaje construido con la instruccion
 * dada.
 */
Mensaje serializar(Instruccion instruccion) {
  // Creamos un nuevo mensaje vacio
  Mensaje mensaje = {};

  /**
   * Segun el tipo de selector que tenga la instruccion se va
   * a tener una serializacion distinta. A continuacion tenemos
   * un switch que permite manejar cada uno de los casos correspondientes
   * a cada instruccion.
   */

  // Utilizamos la funcion auxiliar agregar mensaje para agregar
  // al mensaje el selector.
  agregaMensaje(&mensaje, &instruccion.selector, sizeof(enum selectorInstruccion));
  // Segun que formato de instruccion tenga este selector se elige
  // como se serializa la instruccion.
  switch (formatoInstruccion(instruccion.selector)) {
    // En caso de ser una instruccion que tome datos como argumentos
    // se agrega al mensaje la longitud del texto y el texto.
    case FI_DATOS:
      agregaMensaje(&mensaje, &instruccion.argumentos.datos.longitud             , sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datos.texto                 , instruccion.argumentos.datos.longitud * sizeof(char));
      break;

    // En caso de ser una instruccion que tome un path como argumento
    // se agrega al mensaje el path.
    case FI_UNPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.unPath.camino               , (1 + strlen(instruccion.argumentos.unPath.camino)) * sizeof(char));
      break;

    // En caso de ser una instruccion que tome dos paths como argumentos
    // se agregan el path de origen, el path absoluto y finalmente
    // el path de destino al mensaje.
    case FI_DOSPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.origen              , (1 + strlen(instruccion.argumentos.dosPath.origen)) * sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.origenAbsoluto      , (1 + strlen(instruccion.argumentos.dosPath.origenAbsoluto)) * sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.dosPath.destino             , (1 + strlen(instruccion.argumentos.dosPath.destino)) * sizeof(char));
      break;

    // En caso de ser una instruccion que tome un path y datos como
    // argumentos se agrega el path, la longitud del texto y el texto
    // al mensaje.
    case FI_DATOSUNPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.datosUnPath.camino          , (1 + strlen(instruccion.argumentos.datosUnPath.camino)) * sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.datosUnPath.longitud       , sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datosUnPath.texto           , instruccion.argumentos.datosUnPath.longitud * sizeof(char));
      break;

    // En caso de ser una instruccion que tome dos paths y datos como
    // argumentos se agrega el path de destino, el path absoluto de destino,
    // el path de origen, la longitud del texto y el texto al mensaje.
    case FI_DATOSDOSPATH:
      agregaMensaje(&mensaje, instruccion.argumentos.datosDosPath.destino        , (1 + strlen(instruccion.argumentos.datosDosPath.destino)) * sizeof(char));
      agregaMensaje(&mensaje, instruccion.argumentos.datosDosPath.origen         , (1 + strlen(instruccion.argumentos.datosDosPath.origen)) * sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.datosDosPath.longitud      , sizeof(ssize_t));
      agregaMensaje(&mensaje, instruccion.argumentos.datosDosPath.texto          , instruccion.argumentos.datosDosPath.longitud * sizeof(char));
      break;

    // En caso de ser una instruccion que indique un error se agrega
    // el texto del error, y el codigo al mensaje.
    case FI_ERROR:
      agregaMensaje(&mensaje, instruccion.argumentos.error.texto                 , (1 + strlen(instruccion.argumentos.error.texto)) * sizeof(char));
      agregaMensaje(&mensaje, &instruccion.argumentos.error.codigo               , sizeof(int));
      break;

    // En el caso por defecto no se hace nada.
    default: break;
  }

  // Finalmente se retorna el mensaje creado
  return mensaje;
}

/**
 * Se encarga de deserializar un mensaje que contiene una instruccion.
 * @param mensaje Mensaje que contiene una instruccion
 * @return Retorna una instruccion construida a partir del contenido
 * del mensaje dado.
 */
Instruccion deserializar(Mensaje mensaje) {
  // Creamos por defecto una instruccion cuyo selector sea
  // la instruccion invalida.
  Instruccion instruccion = { .selector = SI_INVALIDO };
  // Si el mensaje es vacio es porque es una instruccion invalida
  // de modo que retornamos la creada anteriormente.
  if (!mensaje.contenido) return instruccion;

  // Lo primero que leemos del contenido de cualquier mensaje es el
  // selector de la instruccion que posee.
  // Segun este vamos a decidir como leer el resto de la instruccion.
  instruccion.selector = *(enum selectorInstruccion *)mensaje.contenido;
  // Nos movemos en el contenido tanto como se leyo (tamaño del selector)
  mensaje.contenido += sizeof(enum selectorInstruccion);
  // Utilizamos formatoInstruccion para saber que tipo de formato
  // tiene la instruccion leida, de este modo sabremos como deserializarla.
  switch (formatoInstruccion(instruccion.selector)) {
    // Si es una instruccion que contiene datos se lee la longitud de
    // los datos y luego se lee el texto contenido y se guarda en los
    // argumentos de la instruccion.
    case FI_DATOS:
      instruccion.argumentos.datos.longitud               = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datos.texto                  = (char *)    mensaje.contenido; mensaje.contenido += instruccion.argumentos.datos.longitud * sizeof(char);
      break;

    // Si es una instruccion que contiene un path se lee el path y
    // se guarda en los argumentos de la instruccion.
    case FI_UNPATH:
      instruccion.argumentos.unPath.camino                = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      break;

    // Si es una instruccion que contiene dos paths se lee el path de
    // origen, el path absoluto de origen y el path de destino y se guardan
    // respectivamente en los argumentos de la instruccion.
    case FI_DOSPATH:
      instruccion.argumentos.dosPath.origen               = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.dosPath.origenAbsoluto       = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.dosPath.destino              = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      break;

    // Si es una instruccion que contiene datos y un path se lee el
    // path, la longitud de los datos y el texto y se guardan
    // en los argumentos de la instruccion.
    case FI_DATOSUNPATH:
      instruccion.argumentos.datosUnPath.camino           = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.datosUnPath.longitud         = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datosUnPath.texto            = (char *)    mensaje.contenido; mensaje.contenido += instruccion.argumentos.datosUnPath.longitud * sizeof(char);
      break;

    // Si es una instruccion que contiene datos y dos paths se lee
    // el path de destino, el path absoluto de destino, el path de
    // origen, la longitud de los datos y el texto y se guarda en
    // los argumentos de la instruccion.
    case FI_DATOSDOSPATH:
      instruccion.argumentos.datosDosPath.destino         = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.datosDosPath.origen          = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.datosDosPath.longitud        = *(ssize_t *)mensaje.contenido; mensaje.contenido += sizeof(ssize_t);
      instruccion.argumentos.datosDosPath.texto           = (char *)    mensaje.contenido; mensaje.contenido += instruccion.argumentos.datosDosPath.longitud * sizeof(char);
      break;

    // Si es una instruccion de error se lee el texto del error y el
    // codigo del mismo y se guarda en los argumentos de la instruccion.
    case FI_ERROR:
      instruccion.argumentos.error.texto                  = (char *)    mensaje.contenido; mensaje.contenido += (1 + strlen(mensaje.contenido)) * sizeof(char);
      instruccion.argumentos.error.codigo                 = *(int *)    mensaje.contenido; mensaje.contenido += sizeof(int);
      break;

    default: break;
  }

  // Devolvemos la instruccion creada.
  return instruccion;
}

/**
 * Se encarga de enviar una instruccion a un actor dada su
 * direccion.
 * @param direccion Direccion del actor al que le vamos a escribir.
 * @param instruccion Instruccion que le vamos a enviar al actor.
 */
void enviarInstruccion(Direccion direccion, Instruccion instruccion) {
  // Debemos serializar la instruccion que queremos enviar
  // como un mensaje
  Mensaje mensaje = serializar(instruccion);
  // Si hay un error al enviar manejamos el mismo con perror y
  // hacemos exit.
  if (-1 == enviar(direccion, mensaje)) {
    perror("enviar");
    exit(EX_IOERR);
  }
  // Liberamos el espacio de memoria utilizado por el mensaje.
  free(mensaje.contenido);
}

/**
 * Se encarga de enviarle una orden al front controller
 * @param instruccion Instruccion que queremos enviar al
 * front controller.
 */
void orden(Instruccion instruccion) {
  // Usamos la funcion enviarInstruccion para enviar
  // la instruccion dada al frontController
  enviarInstruccion(frontController, instruccion);
}


/**
 * A continuacion se encuentran constructores utilizados para crear
 * instruccionees segun el tipo de argumentos que tome
 */

/**
 * Se encarga de construir una instruccion simple.
 * @param selector Selector que indica el tipo de instruccion que es
 * @return Retorna la instruccion construida.
 */
Instruccion instruccionSimple(enum selectorInstruccion selector) {
  // Inicializamos instruccion y la retornamos
  Instruccion instruccion =
    { .selector = selector
    }
  ;
  return instruccion;
}

/**
 * Se encarga de construir una instruccion que contenga datos.
 * @param selector selector que indica el tipo de instruccion que es
 * @param longitud longitud de los datos
 * @param texto datos de los argumentos
 * @return Retorna la instruccion construida con los datos dados.
 */
Instruccion instruccionDatos(enum selectorInstruccion selector, ssize_t longitud, char * texto) {
  // Inicializamos instruccion y la retornamos
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

/**
 * Se encarga de construir una instruccion que contenga un path.
 * @param selector selector que indica el tipo de instruccion que es
 * @param camino Path que guardaremos en la instruccion
 * @return Retorna la instruccion construida con el path dado.
 */
Instruccion instruccionUnPath(enum selectorInstruccion selector, char * camino) {
  // Inicializamos instruccion y la retornamos
  Instruccion instruccion =
    { .selector = selector
    , .argumentos.unPath.camino = camino
    }
  ;
  return instruccion;
}

/**
 * Se encarga de construir una instruccion que contenga dos paths.
 * @param selector selector que indica el tipo de instruccion que es
 * @param origen Path de origen que guardaremos en la instruccion
 * @param origenAbsoluto Path absoluto de origen que guardaremos en la instruccion
 * @param destino Path de destino que guardaremos en la instruccion
 * @return Retorna la instruccion construida con los paths dados.
 */
Instruccion instruccionDosPath(enum selectorInstruccion selector, char * origen, char * origenAbsoluto, char * destino) {
  // Inicializamos instruccion y la retornamos
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

/**
 * Se encarga de construir una instruccion que contenga un path y datos.
 * @param selector selector que indica el tipo de instruccion que es
 * @param camino Path que guardaremos en la instruccion
 * @param longitud longitud de los datos
 * @param texto contenido de los datos
 * @return Retorna la instruccion construida con el path y los datos dados.
 */
Instruccion instruccionDatosUnPath(enum selectorInstruccion selector, char * camino, ssize_t longitud, char * texto) {
  // Inicializamos instruccion y la retornamos
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

/**
 * Se encarga de construir una instruccion que contenga dos paths y datos.
 * @param selector selector que indica el tipo de instruccion que es
 * @param destino Path de destino que guardaremos en la instruccion
 * @param origen Path de origen que guardaremos en la instruccion
 * @param longitud longitud de lo datos
 * @param texto contenido de los datos
 * @return Retorna la instruccion construida con los paths y datos dados.
 */
Instruccion instruccionDatosDosPath(enum selectorInstruccion selector, char * destino, char * origen, ssize_t longitud, char * texto) {
  // Inicializamos instruccion y la retornamos
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

/**
 * Se encarga de construir una instruccion que contenga un error.
 * @param selector selector que indica el tipo de instruccion que es
 * @param texto texto que indica el error
 * @param codigo codigo de error
 * @return Retorna la instruccion construida con la informacion del error
 * dada
 */
Instruccion instruccionError(enum selectorInstruccion selector, char * texto, int codigo) {
  // Inicializamos instruccion y la retornamos
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

/**
 * Los constructores declarados anteriormente se utilizan ahora para construir
 * individualmente cada una de las instrucciones que se pueden manejar.
 */


/**
 * Constructor para la instruccion que imprime datos.
 * @param longitud longitud del texto a imprimir
 * @param texto contenido a imprimir
 * @return Retorna la instruccion construida con instruccionDatos.
 */
Instruccion c_imprimeReal       (int longitud, char * texto)                                    { return instruccionDatos       (SI_IMPRIME       , longitud     , texto            ); }
/**
 * Constructor para la instruccion que imprime datos y luego un prompt.
 * @param longitud longitud del texto a imprimir
 * @param texto contenido a imprimir
 * @return Retorna la instruccion construida con instruccionDatos.
 */
Instruccion c_imprimeRealyprompt(int longitud, char * texto)                                    { return instruccionDatos       (SI_IMPRIMEYPROMPT, longitud     , texto            ); }
/**
 * Constructor para la instruccion de muerte.
 * @return Retorna una instruccion creada con el constructor de instruccionSimple
 * y el selector correspondiente.
 */
Instruccion c_muere             (void)                                                          { return instruccionSimple      (SI_MUERE                                           ); }
/**
 * Constructor para la instruccion de terminacion.
 * @return Retorna una instruccion creada con el constructor de instruccionSimple
 * y el selector correspondiente.
 */
Instruccion c_termine           (void)                                                          { return instruccionSimple      (SI_TERMINE                                         ); }
/**
 * Constructor para la instruccion que imprime un prompt.
 * @return Retorna una instruccion creada con el constructor de instruccionSimple
 * y el selector correspondiente.
 */
Instruccion c_prompt            (void)                                                          { return instruccionSimple      (SI_PROMPT                                          ); }
/**
 * Constructor para la instruccion lsall.
 * @return Retorna una instruccion creada con el constructor de instruccionSimple
 * y su selector y camino correspondiente
 */
Instruccion c_lsall             (void)                                                          { return instruccionSimple      (SI_LSALL                                           ); }
/**
 * Constructor para la instruccion que imprime texto
 * @param texto texto que va a imprimir
 * @return Retorna una instruccion creada con el constructor de intruccionDatos
 * y su selector correspondiente.
 */
Instruccion c_imprime           (char * texto)                                                  { return instruccionDatos       (SI_IMPRIME       , strlen(texto), texto            ); }
/**
 * Constructor para la instruccion que imprime texto y luego un prommpt
 * @param texto texto que va a imprimir
 * @return Retorna una instruccion creada con el constructor de intruccionDatos
 * y su selector correspondiente.
 */
Instruccion c_imprimeyprompt    (char * texto)                                                  { return instruccionDatos       (SI_IMPRIMEYPROMPT, strlen(texto), texto            ); }
/**
 * Constructor para la instruccion ls.
 * @param camino camino donde se va a ejecutar la instruccion
 * @return Retorna una instruccion creada con el constructor de instruccionUnPath
 * y su selector y camino correspondiente
 */
Instruccion c_ls                (char * camino)                                                 { return instruccionUnPath      (SI_LS            , camino                          ); }
/**
 * Constructor para la instruccion mkdir.
 * @param camino camino donde se va a ejecutar la instruccion
 * @return Retorna una instruccion creada con el constructor de instruccionUnPath
 * y su selector y camino correspondiente
 */
Instruccion c_mkdir             (char * camino)                                                 { return instruccionUnPath      (SI_MKDIR         , camino                          ); }
/**
 * Constructor para la instruccion rm.
 * @param camino camino donde se va a ejecutar la instruccion
 * @return Retorna una instruccion creada con el constructor de instruccionUnPath
 * y su selector y camino correspondiente
 */
Instruccion c_rm                (char * camino)                                                 { return instruccionUnPath      (SI_RM            , camino                          ); }
/**
 * Constructor para la instruccion rmdir.
 * @param camino camino donde se va a ejecutar la instruccion
 * @return Retorna una instruccion creada con el constructor de instruccionUnPath
 * y su selector y camino correspondiente
 */
Instruccion c_rmdir             (char * camino)                                                 { return instruccionUnPath      (SI_RMDIR         , camino                          ); }
/**
 * Constructor para la instruccion cat.
 * @param camino camino donde se va a ejecutar la instruccion
 * @return Retorna una instruccion creada con el constructor de instruccionUnPath
 * y su selector y camino correspondiente
 */
Instruccion c_cat               (char * camino)                                                 { return instruccionUnPath      (SI_CAT           , camino                          ); }
/**
 * Constructor para la instruccion find.
 * @param origen Path de origen
 * @param origenAbsoluto Path absoluto de origen
 * @param destino Path de destino
 * @return Retorna una instruccion creada con el constructor de instruccionDosPath
 * y su selector y caminos correspondiente
 */
Instruccion c_find              (char * origen, char * origenAbsoluto, char * destino)          { return instruccionDosPath     (SI_FIND          , origen, origenAbsoluto, destino ); }
/**
 * Constructor para la instruccion cp.
 * @param origen Path de origen
 * @param origenAbsoluto Path absoluto de origen
 * @param destino Path de destino
 * @return Retorna una instruccion creada con el constructor de instruccionDosPath
 * y su selector y caminos correspondiente
 */
Instruccion c_cp                (char * origen, char * origenAbsoluto, char * destino)          { return instruccionDosPath     (SI_CP            , origen, origenAbsoluto, destino ); }
/**
 * Constructor para la instruccion mv.
 * @param origen Path de origen
 * @param origenAbsoluto Path absoluto de origen
 * @param destino Path de destino
 * @return Retorna una instruccion creada con el constructor de instruccionDosPath
 * y su selector y caminos correspondiente
 */
Instruccion c_mv                (char * origen, char * origenAbsoluto, char * destino)          { return instruccionDosPath     (SI_MV            , origen, origenAbsoluto, destino ); }
/**
 * Constructor para la instruccion write.
 * @param camino camino a donde vamos a escribir
 * @param longitud longitud de los datos a escribir
 * @param texto contenido de los datos a escribir
 * @return Retorna una instruccion creada con el constructor de instruccionDatosUnPath,
 * su selector, camino y contenido correspondiente
 */
Instruccion c_write             (char * camino, ssize_t longitud, char * texto)                 { return instruccionDatosUnPath (SI_WRITE         , camino, longitud, texto         ); }
/**
 * Constructor para la instruccion write y borro.
 * @param destino camino a donde vamos a escribir
 * @param origen camino del archivo que vamos a borrar cuando
 * se termine de escribir.
 * @param longitud longitud de los datos a escribir
 * @param texto contenido de los datos a escribir
 * @return Retorna una instruccion creada con el constructor de instruccionDatosDosPath,
 * su selector, caminos y contenido correspondiente
 */
Instruccion c_writeyborra       (char * destino, char * origen, ssize_t longitud, char * texto) { return instruccionDatosDosPath(SI_WRITEYBORRA   , destino, origen, longitud, texto); }
/**
 * Constructor para la instruccion error.
 * @param texto texto de error
 * @param codigo codigo de error
 * @return Retorna una instruccion creada con el constructor de instruccionError
 * su selector, texto y codigo correspondiente
 */
Instruccion c_error             (char * texto, int codigo)                                      { return instruccionError       (SI_ERROR         , texto, codigo                   ); }
/**
 * Constructor para la instruccion error y prompt.
 * @param texto texto de error
 * @param codigo codigo de error
 * @return Retorna una instruccion creada con el constructor de instruccionError
 * su selector, texto y codigo correspondiente
 */
Instruccion c_erroryprompt      (char * texto, int codigo)                                      { return instruccionError       (SI_ERRORYPROMPT  , texto, codigo                   ); }



/**
 * Se encarga de enviar una señal de muerte
 */
void muere() {
  orden(c_muere());
}


/**
 * Estructura que contiene la informacion que se
 * guarda en una libreta de direcciones.
 */
struct libreta {
  char * nombre;              /**< Nombre del actor                 */
  Direccion direccion;        /**< Direccion del actor              */
  struct libreta * siguiente; /**< Apuntador a la proxima direccion */
};

/** Libreta de direcciones.
 * Se crea una variable global que sea un apuntador a una libreta de
 * direcciones. Se guardan aca las direcciones de todos los actores que
 * se conozca. En este caso cada actor directorio tiene las direcciones
 * de su padre y de sus hijos (subdirectorios)
 */
struct libreta * libreta;

/**
 * Se encarga de insertar una nueva direccion a la libreta.
 * @param nombre nombre del actor correspondiente a la nueva direccion
 * @param direccion direccion a agregar
 * @return Retorna un apuntador a una libreta de direcciones
 */
struct libreta * insertarLibreta(char * nombre, Direccion direccion) {
  // Creamos un nuevo apuntador a libreta de direcciones y
  // reservamos espacio para una nueva direccion
  struct libreta * nueva = calloc(1, sizeof(struct libreta));
  // Asignamos los valores correspondientes y en el valor de siguiente
  // podemos la libreta ya existente, de modo que la nueva entrada quede
  // al principio de la libreta
  nueva->nombre = nombre;
  nueva->direccion = direccion;
  nueva->siguiente = libreta;
  // Asignamos el valor de la nueva libreta al apuntador a la libreta
  // actual y retornamos.
  libreta = nueva;
  return nueva;
}

/**
 * Se encarga de liberar el espacio ocupado por la libreta de direcciones
 */
void liberarLibreta(void) {
  // Si la libreta es vacia retornamos inmediatamente
  if (!libreta) return;
  // Si no esta vacia creamos un apuntador a libreta y guardamos
  // el apuntador a siguiente de la entrada actual
  struct libreta * siguiente = libreta->siguiente;
  // Liberamos los camposnombre y direccion para finalmente liberar
  // el apuntador global a libreta.
  free(libreta->nombre);
  liberaDireccion(libreta->direccion);
  free(libreta);
  // Le asignamos a libreta (global) el valor de siguiente (cola de la libreta
  // luego de eliminar la entrada actual) y llamamos recursivamente a la funcion.
  libreta = siguiente;
  liberarLibreta();
}

/**
 * Se encarga de liberar una celda de la libreta; es decir
 * elimina al actor de la libreta de direcciones, mata al
 * actor y libera el espacio de memoria que ocupaba su entrada en la libreta.
 * @param nombre nombre del actor a eliminar
 */
void eliminarCeldaLibreta(char const * nombre) {
  // Creamos un apuntador al nodo actua de la libreta
  struct libreta ** actual;
  // Recorremos la libreta de direcciones
  for (actual = &libreta; *actual; actual = &(*actual)->siguiente) {
    // Al conseguir la direccion en la librieta
    if (!strcmp((*actual)->nombre, nombre)) {
      // Se crea un reespaldo de la cola de la lista
      struct libreta * respaldo = (*actual)->siguiente;
      // Se libera el espacio ocupado por el nombre
      free((*actual)->nombre);
      // Se envia una instruccion al actor indicandole que muera
      enviarInstruccion((*actual)->direccion, c_muere());
      // Se libera la direccion de la libreta
      liberaDireccion((*actual)->direccion);
      // Liberamos el espacio ocupado por actual
      free(*actual);
      // Ahora ponemos en el lugar donde estaba actual al respaldo
      *actual = respaldo;
      return;
    };
  }
}

/**
 * Se encarga de buscar una direccion en la libreta
 * dado el nombre del actor.
 * @param nombre nombre del actor cuya direccion queremos
 * @return Retorna la direccion del actor cuyo nombre se
 * indica en los parametros, en caso de no conseguirla retorna
 * NULL
 */
Direccion buscarLibreta(char const * nombre) {
  // Creamos una variable apuntadora a una entrada de la libreta
  struct libreta * entLibreta;
  // Buscamos en todas las entradas de la libreta
  for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
    // Si se consigue un nombre en la libreta que concuerde con el
    // dado, se devuelve esa direccion
    if (!strcmp(entLibreta->nombre, nombre)) return entLibreta->direccion;
  }

  // Si se recorre la libreta y no se consigue nada se devuelve NULL
  return NULL;
}

/**
 * Se encarga de contar los elementos de la libreta.
 * @param libreta apuntador a la libreta cuyos elementos
 * queremos contar
 * @param acumulador numero de elementos que ya se han contado
 * de la libreta
 * @return Retorna la cantidad de elementos que tiene la libreta
 * apuntada en el parametro libreta.
 */
int contarLibreta(struct libreta * libreta, int acumulador) {
  // Si la libreta es vacia retornamos lo que llevabamos acumulado.
  if (!libreta) return acumulador;
  // Si la entrada no es la de . (directorio actual) o .. (directorio padre)
  // aumentamos en 1 el acumulador.
  if (strcmp(libreta->nombre, ".") && strcmp(libreta->nombre, "..")) ++acumulador;
  // Llamamos recursivamente a la libreta con el apuntador al siguiente elemento
  // de la libreta.
  return contarLibreta(libreta->siguiente, acumulador);
}

/**
 * Se encarga de calcular el numero de hijos que tiene un actor.
 * (numero de subdirectorios del directorio)
 * @return Retorna el numero de hijos que tiene el actor en su
 * libreta de memoria.
 */
int numeroHijos(void) {
  // Llamamos a la funcion contarLibreta con el apuntador a la libreta
  // global y el acumulador en cero y retornamos el valor resultante de
  // la llamada a esta funcion.
  return contarLibreta(libreta, 0);
}



/**
 * Se encarga de procesar el subdirectorio creado; es decir,
 * crear una nueva entrada en la libreta de direcciones y un
 * actor que esté asociado a este nuevo directorio.
 * @param dir direccion del nuevo directorio creado
 */
void procesarSubdirectorio(char const * dir);

/**
 * Se encarga de ejercer las funciones que debe cumplir
 * el comando mkdir.
 * @param instruccion instruccion que llega con los argumentos
 * necesarios para crear el directorio.
 */
void do_mkdir(Instruccion instruccion) {
  // En caso de que mkdir de error
  if (-1 == mkdir(instruccion.argumentos.unPath.camino, S_IRWXU | S_IRWXG | S_IRWXO)) {
    switch (errno) {
      // En caso de ser errores no recuperables se envia una orden
      // al front controller que indique que hubo un error con mkdir
      case EFAULT:
      case ENOMEM:
        orden(c_error("mkdir", errno));
        muere();
        return;

      // Si es un error recuperable se envia una orden al front controller
      // que indique que hubo un error y que imprima un prompt nuevo.
      default:
        orden(c_erroryprompt("mkdir", errno));
        return;
    }
  }
  // Una vez creado el nuevo directorio llamamos a procesar subdirectorio con
  // el camino dado en los argumentos.
  procesarSubdirectorio(instruccion.argumentos.unPath.camino);
  // Finalmente mandamos una orden al front controller para que imprima un nuevo prompt
  orden(c_prompt());
}

/**
 * Se encarga de ejercer las funciones que debe cumplir
 * el comando rm.
 * @param instruccion instruccion que llega con los argumentos necesarios
 * para eliminar algo.
 */
void do_rm(Instruccion instruccion) {
  // Utilizamos unlink para eliminar el archivo dado en los
  // argumentos de la instruccion
  if (-1 == unlink(instruccion.argumentos.unPath.camino)) {
    // En caso de que haya un error
    switch (errno) {
      // Si no es recuperable se manda una orden al front controller
      // que indique que hubo un error con unlink
      case EFAULT:
      case ENOMEM:
        orden(c_error("rm: unlink", errno));
        muere();
        return;

      // Si es recuperable se envia un mensaje al front controller
      // que indique que hubo un error y que imprima un nuevo prompt
      default:
        orden(c_erroryprompt("rm: unlink", errno));
        return;
    }
  }
  // Si se logro eliminar exitosamente enviamos una orden al front
  // controller para que imprima un nuevo prompt
  orden(c_prompt());
}

/**
 * Se encarga de eliminar un directorio (accion
 * de rmdir)
 * @param instruccion que llega con los argumentos
 * necesarios para realizar rmdir
 */
void do_rmdir(Instruccion instruccion) {
  // Eliminamos el directorio
  if (-1 == rmdir(instruccion.argumentos.unPath.camino)) {
    switch (errno) {
      // Si hubo un error no recuperable se envia una
      // orden al front controller para indicar que hubo
      // un error
      case EFAULT:
      case ENOMEM:
        orden(c_error("rmdir", errno));
        muere();
        return;

      // Si hubo un error recuperable se envia una orden
      // al front controller para indicar que hubo un error
      // y que imprima un nuevo prompt
      default:
        orden(c_erroryprompt("rmdir", errno));
        return;
    }
  }
  // Si eliminamos exitosamente el directorio
  // quitamos de la libreta de direcciones su direccion.
  eliminarCeldaLibreta(instruccion.argumentos.unPath.camino);
  // Enviamos una orden al front controller para que
  // imprima un prompt
  orden(c_prompt());
}

/**
 * Se encarga de poner un caracter nulo en la ultima ocurrencia
 * de un salto de linea en un string dado
 * @param s string dado
 * @return Retorna el string con un caracter nulo en el final.
 */
char * chomp(char * s) {
  // Buscamos el apuntador a la ultima ocurrencia de un
  // salto de linea y lo guardamos en pos
  char * pos = strrchr(s, '\n');
  // si se consiguio esa ocurrencia guardamos
  // en el contenido de esa posicion un caracter nulo
  if (pos) *pos = '\0';
  // Retornamos el string dado
  return s;
}

/**
 * Se encarga de ejercer las funciones que debe cumplir
 * el comando ls.
 * @param camino camino sobre el cual se ejercera ls
 */
void base_ls(char const * camino) {
  // Creamos una variable que almacene el resultado de hacer
  // stat del archivo.
  struct stat stats;
  if (-1 == stat(camino, &stats)) {
    switch (errno) {
      // Si falla y es un error no recuperable enviamos una orden
      // al front controller que indique que hubo un error
      case EFAULT:
      case ENOMEM:
        orden(c_error("ls: stat", errno));
        muere();
        return;

      // Si es recuperable se envia un mensaje al front controller
      // para indicar que hubo un error y que imprima un nuevo prompt
      default:
        orden(c_error("ls: stat", errno));
        return;
    }
  }

  char * user;
  // Buscamos el nombre del usuario dueño del archivo con getpwuid
  // que nos devuelve informacion del usuario en un struct.
  if (!getpwuid(stats.st_uid)) {
    if (-1 == asprintf(&user, "%s", getpwuid(stats.st_uid)->pw_name)) {
      // si hay un error copiando este nombre a un string
      // entonces enviamos una orden al front controller que indique
      // que hubo un error y que imprima un nuevo prompt
      orden(c_error("ls: getpwuid", errno));
      return;
    }
  } else {
    // Si no conseguimos la informacion del usuario tomamos como nombre
    // su userid
    if (-1 == asprintf(&user, "%d", stats.st_uid)) {
      // si hay un error copiando este nombre a un string
      // entonces enviamos una orden al front controller que indique
      // que hubo un error y que imprima un nuevo prompt
      orden(c_error("ls: asprintf", errno));
      return;
    }
  }

  char * group;
  // Buscamos el nombre del grupo dueño del archivo con getpwuid
  // que nos devuelve informacion del grupo en un struct.
  if (!getgrgid(stats.st_gid)) {
    if (0 > asprintf(&group, "%s", getgrgid(stats.st_gid)->gr_name)) {
      // si hay un error copiando este nombre a un string
      // entonces enviamos una orden al front controller que indique
      // que hubo un error y que imprima un nuevo prompt
      orden(c_error("ls: getgrid", errno));
      // Liberamos el espacio utilizado por user antes de retornar
      free(user);
      return;
    }
  } else {
    // Si no conseguimos la informacion del grupo tomamos como nombre
    // su groupid
    if (0 > asprintf(&group, "%d", stats.st_gid)) {
      // si hay un error copiando este nombre a un string
      // entonces enviamos una orden al front controller que indique
      // que hubo un error y que imprima un nuevo prompt
      orden(c_error("ls: asprintf", errno));
      // Liberamos el espacio utilizado por user antes de retornar
      free(user);
      return;
    }
  }

  // Construimos ahora la salida del comando (texto)
  char * texto;
  // Se crea un string de formato conteniendo los permisos,
  // el numero de hard links, nombre de usuario, nombre de grupo,
  // tamaño en bloques, ultima fecha de modificacion y finalmente
  // el nombre del archivo
  if (0 >
    asprintf
      ( &texto
      , "%c%c%c%c%c%c%c%c%c%c %d %s %s %d %s %s\n"
      , S_ISDIR(stats.st_mode)  ? 'd' : '-'   // Si es un directorio se pone d, sino se pone -
      , S_IRUSR & stats.st_mode ? 'r' : '-'   // permiso de lectura del usuario
      , S_IWUSR & stats.st_mode ? 'w' : '-'   // permiso de escritura del usuario
      , S_IXUSR & stats.st_mode ? 'x' : '-'   // permiso de ejecucion del usuario
      , S_IRGRP & stats.st_mode ? 'r' : '-'   // permiso de lectura del grupo
      , S_IWGRP & stats.st_mode ? 'w' : '-'   // permiso de escritura del grupo
      , S_IXGRP & stats.st_mode ? 'x' : '-'   // permiso de ejecucion del grupo
      , S_IROTH & stats.st_mode ? 'r' : '-'   // permiso de lectura de otros
      , S_IWOTH & stats.st_mode ? 'w' : '-'   // permiso de escritura de otros
      , S_IXOTH & stats.st_mode ? 'x' : '-'   // permiso de ejecucion de otros
      , (int)stats.st_nlink                   // numero de hard links
      , user                                  // nombre del usuario
      , group                                 // nombre del grupo
      , (int)stats.st_blocks                  // tamaño en bloques
      , chomp(ctime(&stats.st_mtime))         // ultima fecha de modificacion
      , camino                                // nombre del archivo
      )
  ) {
    // si hay un error haciendo asprintf se manda una orden al front controller que
    // indica que hubo un error e imprime un nuevo prompt
    orden(c_error("ls: asprintf", errno));
  } else {
    // Si no hay error escribiendo entonces se manda una orden al front controller
    // con texto para imprimir y que luego imprima un prompt en pantalla.
    orden(c_imprime(texto));
    // Libero el espacio ocupado por texto
    free(texto);
  }
  // Libero el espacio ocupado por user y group
  free(user);
  free(group);
}

/**
 * Se encarga de llamar a procesarSubdirectorio, esta
 * funcion se llama cuando se hace scandir.
 * @param dir contiene informacion de un directorio
 * @return retorna cero siempre
 */
/**
 * Se encarga de llamar a base_ls, esta funcion
 * se llama cuando se hace scandir.
 * @param dir contiene informacion de un directorio
 * @return Retorna cero siempre
 */
int filterLs(struct dirent const * dir) {
  // Se llama a base_ls con el nombre del
  // directorio que se le pasa
  base_ls(dir->d_name);
  return 0;
}

/**
 * Se encarga de realizar ls sobre los el archivo o
 * directorio indicado en instruccion
 * @param instruccion instruccion que contiene la informacion
 * necesaria para hacer el ls
 */
void do_ls(Instruccion instruccion) {
  // Se hace stat del archivo para ver informacion importante acerca
  // del mismo
  struct stat stats;
  if (-1 == stat(instruccion.argumentos.unPath.camino, &stats)) {
    switch (errno) {
      // En caso de que falle por algun error no recuperable se envia
      // una orden al front controller que indica que hubo un error
      case EFAULT:
      case ENOMEM:
        orden(c_error("ls: stat", errno));
        muere();
        return;

      // En caso de que falle por algun error recuperable se envia
      // una orden al front controller que indica que hubo un error
      // y ademas se manda a imprimir un nuevo prompt
      default:
        orden(c_erroryprompt("ls: stat", errno));
        return;
    }
  }

  // Si estamos tratando con un directorio se envia una instruccion
  // al directorio con el que estamos trabajando para que haga ls de
  // todos los archivos que el contiene.
  if (S_ISDIR(stats.st_mode)) {
    enviarInstruccion(buscarLibreta(instruccion.argumentos.unPath.camino), c_lsall());
  } else {
    // Sino se realiza el caso base de ls que es para un archivo solamente
    base_ls(instruccion.argumentos.unPath.camino);
    // Se envia una orden al front controller para que imprima un prompt
    orden(c_prompt());
  }
}

/**
 * Se encarga de manejar el trabajo con archivos.
 * Lee del archivo y pasa los datos contenidos en el a
 * una funcion dada que trabajara con estos datos.
 * @param camino camino del archivo que se va a leer
 * @param funcion funcion que va a trabajar con el
 * contenido del archivo luego de leido.
 * @param datos datos que se le pasan a la funcion
 * que trabaja con el contenido del archivo.
 */
void conContenido(char * camino, void (*funcion)(int longitud, char * buffer, void * datos), void * datos) {
  // Abrimos el archivo para lectura
  int fd = open(camino, O_RDONLY);
  if (-1 == fd) {
    switch (errno) {
      // Si hubo un error al hacer open
      // y es recuperable enviamos una orden al front controller
      // que indique que hubo un error e imprima un prompt
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

      // En caso de que no sea recuperable se envia solamente
      // la orden de error al front controller.
      default:
        orden(c_error("open", errno));
        muere();
        break;
    }
    return;
  }

  // Creamos variables para guardar la longitud y un buffer
  // de contenido leido.
  int longitud = 0;
  char * buffer = NULL;
  // Utilizamos un ciclo infinito (que no es un ciclo infinto
  // porque contiene una condicion de parada que asegura una
  // cantidad finita de iteraciones) para asegurarnos de leer
  // hasta el final del archivo
  while (1) {
    // Agregamos espacio al buffer (leeremos el archivo de
    // 1024 en 1024 bytes.
    char * nuevoBuffer = realloc(buffer, longitud + 1024);
    // Si hubo un error haciendo realloc se define errno apropiadamente
    // y se envia un mensaje al front controller que indique que hubo un error.
    if (!nuevoBuffer) {
      // Liberamos el espacio de memoria ocupado por buffer
      free(buffer);
      //realloc define errno si falla y entonces llamamos a muere
      orden(c_error("realloc", errno));
      // Cerramos el file descriptor
      close(fd);
      // Matamos al actor
      muere();
      return;
    }
    // En caso de que no haya errores ahora
    // buffer sera nuevoBuffer
    buffer = nuevoBuffer;

    int leido;

    // Utilizamos un ciclo infinito (que no es un ciclo infinto
    // porque contiene una condicion de parada que asegura una
    // cantidad finita de iteraciones) para poder recuperar errores
    // de lectura que no sean fatales.
    while (1) {
      // Leemos del archivo y guardamos en el buffer
      leido = read(fd, buffer + longitud, 1024);
      if (-1 == leido) {
        switch (errno) {
          // Cuando hay un error y es recuperable se envia una orden
          // al front controller para indicar que hay un error y se
          // imprime un nuevo prompt
          case EISDIR:
          case EINVAL:
          case EINTR:
            orden(c_erroryprompt("read", errno));
            break;

          // En caso de ser no recuperable se envia solamente la orden
          // indicando que hubo un error.
          default:
            orden(c_error("read", errno));
            // Se mata al actor
            muere();
            break;
        }
        // En cualquiera de los casos liberamos el espacio ocupado por
        // buffer y cerramos el file descriptor
        free(buffer);
        close(fd);
        return;
      }
      break;
    }
    // Si lo que leimos es igual a cero es porque
    // ya llegamos al final del archivo por lo que hacemos break y salimos
    // del ciclo de lectura
    if (0 == leido) break;
    // Sino agregamos la cantidad leida a la longitud
    longitud += leido;
  }

  // Cerramos el file descriptor
  close(fd);

  // Aca revisaremos que no se haya reservado memoria de mas, realloc se encarga
  // de dejar la memoria justa para contener al buffer en la longitud indicada.
  {
    char * nuevoBuffer = realloc(buffer, longitud);
    // Si ocurre un error haciendo realloc se asigna errno apropiadamente
    // y se envia una orden al front controller que indica que hubo un error
    if (!nuevoBuffer) {
      // liberamos el espacio ocupado por buffer
      free(buffer);
      //realloc define errno si falla y entonces llamamos a muere
      orden(c_error("realloc", errno));
      // Llamamos a muere y retornamos.
      muere();
      return;
    }
    // En caso de que sea exitoso ahora el apuntador a buffer será nuevo buffer
    buffer = nuevoBuffer;
  }

  // Cuando ya hemos leido el contenido del archivo llamamos a la funcion
  // que trabaja con el contenido.
  funcion(longitud, buffer, datos);

  // Liberamos el espacio ocupado por buffer
  free(buffer);
}

/**
 * Se encarga de enviar una orden al front controller
 * para que imprima los datos de un buffer.
 * @param longitud longitud de los datos
 * @param buffer buffer donde se encuentran los datos del archivo
 * @param datos datos que se le pasan a la funcion
 */
void hacerCat(int longitud, char * buffer, void * datos) {
  // Envia una orden al front controller con los datos que
  // va imprimir y luego imprime un nuevo prompt
  orden(c_imprimeRealyprompt(longitud, buffer));
}

/**
 * Se encarga de llamar a conContenido para que lea el
 * archivo indicado en los argumentos de la instruccion y luego
 * ese contenido se lo envie a hacerCat.
 * @param instruccion instruccion que contiene el path del archivo
 * al que se le hace cat
 */
void do_cat(Instruccion instruccion) {
  // Se llama a conContenido para que lea el archivo y luego su
  // contenido se lo pase a hacerCat para que trabaje con el
  // (lo mande a imprimir)
  conContenido(instruccion.argumentos.unPath.camino, hacerCat, NULL);
}

/**
 * Se encarga de tomar un contenido y copiarlo en un archivo
 * nuevo indicado por los datos que se le pasa en los
 * parametros a la funcion.
 * @param longitud longitud del contenido
 * @param buffer contenido
 * @param datos datos que se le pasan para que la funcion
 * trabaje con ellos
 */
void hacerCP(int longitud, char * buffer, void * datos) {
  // Los datos que se le pasan a esta funcion son una instruccion
  // que indicara el path de destino donde vamos a hacer cp
  Instruccion * instruccion = (Instruccion *)datos;
  // finalmente se le envia una orden al front controller
  // para que escriba en el archivo indicado por el path de
  // destino de la instruccion el contenido que se le pasa en los
  // argumentos.
  orden(c_write(instruccion->argumentos.dosPath.destino, longitud, buffer));
}

/**
 * Se encarga de tomar un contenido y copiarlo en un archivo
 * nuevo indicado por los datos que se le pasa en los
 * parametros a la funcion ademas de copiarlo elimina el archivo
 * inicial.
 * @param longitud longitud del contenido
 * @param buffer contenido
 * @param datos datos que se le pasan para que la funcion
 * trabaje con ellos
 */
void hacerMV(int longitud, char * buffer, void * datos) {
  // Los datos que se le pasan a esta funcion son una instruccion
  // que indicara el path de destino donde vamos a hacer mv y
  // ademas tiene el path de origen del archivo que debemos eliminar
  Instruccion * instruccion = (Instruccion *)datos;
  // finalmente se el envia una orden al front controller para que
  // escriba en el archivo indicado por el path de destino de la
  // instruccion el contenido que se le pasa en los argumentos y
  // ademas borre el archivo indicado por el path de origen de la
  // instruccion.
  orden(c_writeyborra(instruccion->argumentos.dosPath.destino, instruccion->argumentos.dosPath.origenAbsoluto, longitud, buffer));
}

/**
 * Se encarga de llamar a conContenido para que lea el
 * archivo indicado en los argumentos de la instruccion y luego
 * ese contenido se lo envie a hacerCP
 * @param instruccion instruccion que contiene el path del archivo
 * del que se hace cp y a donde se copia
 */
void do_cp(Instruccion instruccion) {
  // Se llama a conContenido para que lea el archivo y luego su
  // contenido se lo pase a hacerCP para que trabaje con el
  // (lo mande a imprimir en el nuevo archivo)
  conContenido(instruccion.argumentos.dosPath.origen, hacerCP, &instruccion);
}

/**
 * Se encarga de llamar a conContenido para que lea el
 * archivo indicado en los argumentos de la instruccion y luego
 * ese contenido se lo envie a hacerMV
 * @param instruccion instruccion que contiene el path del archivo
 * del que se hace mv y luego se borra y a donde se mueve
 */
void do_mv(Instruccion instruccion) {
  // Se llama a conContenido para que lea el archivo y luego su
  // contenido se lo pase a hacerMV para que trabaje con el
  // (lo mande a imprimir en el nuevo archivo y borre el original)
  conContenido(instruccion.argumentos.dosPath.origen, hacerMV, &instruccion);
}

/**
 * Se encarga de escribir en un archivo el contenido dado
 * dado el path de ese archivo.
 * @param camino Path del archivo donde vamos a escribir
 * @param longitud longitud del texto que vamos a escribir
 * @param texto contenido que vamos a escribir en el archivo
 */
void escribe(char * camino, int longitud, char * texto) {
  // Abrimos el archivo para lectura y escritura, en caso de que no este se crea
  int fd = open(camino, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
  if (-1 == fd) {
    // Si hubo un error al hacer open
    // y es recuperable enviamos una orden al front controller
    // que indique que hubo un error e imprima un prompt
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

      // En caso de que no sea recuperable se envia solamente
      // la orden de error al front controller.
      default:
        orden(c_error("escribe: open", errno));
        muere();
        break;
    }
    return;
  }

  // Utilizamos un ciclo infinito (que no es un ciclo infinto
/**
 * Se encarga de manejar el trabajo con archivos.
 * Lee del archivo y pasa los datos contenidos en el a
 * una funcion dada que trabajara con estos datos.
 * @param camino camino del archivo que se va a leer
 * @param funcion funcion que va a trabajar con el
 * contenido del archivo luego de leido.
 * @param datos datos que se le pasan a la funcion
 * que trabaja con el contenido del archivo.
 */
  // porque contiene una condicion de parada que asegura una
  // cantidad finita de iteraciones) para asegurarnos de escribir
  // hasta escribir todo lo que se tenga que escribir. Ademas de que
  // facilita la recuperacion de errores de escritura que no sean fatales.
  while (1) {
    // Mientras aun quede por escribir, se escribe.
    while (longitud > 0) {
      // Escribimos en el archivo
      int escrito = write(fd, texto, longitud);
      if (-1 == escrito) {
        // Si hay un error de escritura porque fue interrumpido
        // (error recuperable) se hace continue para intentar de nuevo
        if (EINTR == errno) continue;
        // Si falla por alguna otra razon recuperable se envia una orden al
        // front controller que indica que hubo un error y que imprima un
        // nuevo prompt
        switch (errno) {
          case EFBIG:
          case EIO:
          case ENOSPC:
          case EPIPE:
            orden(c_erroryprompt("write", errno));
            break;

          // En caso de que sea un error no recuperable se envia una orden
          // que solo indique que hubo un error al front controller
          default:
            orden(c_error("write", errno));
            muere();
            break;
        }
        return;
      }
      // Disminuimos la longitud en la cantidad escrita
      longitud -= escrito;
      // nos movemos en el contador la cantidad ya escrita
      texto += escrito;
    }
    // Cuando se ha escrito todo salimos del ciclo infinto
    break;
  }

  // Cerramos el file descriptor del archivo
  close(fd);
}

/**
 * Se encarga de llamar a escribe para que escriba en el
 * archivo indicado el contenido indicado en la instruccion dada.
 * El path del archivo a donde se va a escribir se encuentra en la
 * instruccion que se recibe como parametro.
 * @param instruccion instruccion que contiene el path del archivo
 * al que se va a escribir y el contenido que se va a escribir.
 */
void do_write(Instruccion instruccion) {
  // Se llama a escribe para que escriba en el archivo
  // el contenido necesario
  escribe(instruccion.argumentos.datosUnPath.camino, instruccion.argumentos.datosUnPath.longitud, instruccion.argumentos.datosUnPath.texto);
  // Se le envia una orden al front controller para que
  // imprima un nuevo prompt
  orden(c_prompt());
}

/**
 * Se encarga de llamar a escribe para que escriba en el
 * archivo indicado el contenido indicado en la instruccion dada.
 * El path del archivo a donde se va a escribir se encuentra en la
 * instruccion que se recibe como parametro. Adicionalmente se borra
 * el archivo de origen indicado tambien en la instruccion.
 * @param instruccion instruccion que contiene el path del archivo
 * al que se va a escribir y el contenido que se va a escribir.
 */
void do_writeyborra(Instruccion instruccion) {
  // Se llama a escribe para que escriba en el archivo
  // el contenido necesario
  escribe(instruccion.argumentos.datosDosPath.destino, instruccion.argumentos.datosDosPath.longitud, instruccion.argumentos.datosDosPath.texto);
  // Se le envia una orden al front controller para que
  // elimine el archivo contenido en los argumentos de la instruccion
  // (archivo de origen)
  orden(c_rm(instruccion.argumentos.datosDosPath.origen));
}



/**
 * Se encarga de descender en el path de una instruccion
 * hasta llegar al subdirectorio que pueda ejecutar la accion
 * pedida.
 * @param accion funcion que determina la accion que se realizara
 * al llegar al subdirectorio correcto
 * @param instruccion instruccion que contiene el path por el
 * cual se debe descender
 */
void descender(void (*accion)(Instruccion), Instruccion instruccion) {
  // Creamos una nueva variable que sea la cola del camino,
  // colaDeCamino apuntara a la ultima ocurrencia
  // de "/" en el path
  char * colaDeCamino = strchr(instruccion.argumentos.unPath.camino, '/');
  if (!colaDeCamino) {
    // Si no hay cola de camino es porque estamos en el subdirectorio
    // indicado asi que ejecutamos la accion con la instruccion dada
    accion(instruccion);
  } else {
    // Sino, guardamos en los datos en la posicion de colaDeCamino
    // un caracter nulo.
    *colaDeCamino = '\0';
    // Tendremos una nueva variable para la cabeza del camino que
    // será la direccion de inicio del path de los argumentos de
    // la instruccion
    char * cabezaDeCamino = instruccion.argumentos.unPath.camino;
    instruccion.argumentos.unPath.camino = 1 + colaDeCamino;
    // OJO: esto funciona por el union, pero es medio arriesgado;
    // la idea es que el path que se consume con descender es el
    // que esté al principio de los argumentos, y aunque acá se
    // modifica en el formato unPath, también se aplica al formato
    // dosPath, datosUnPath y datosDosPath.

    // Buscamos en la libreta la direccion del subdirectorio
    Direccion subdirectorio;
    if ((subdirectorio = buscarLibreta(cabezaDeCamino))) {
      // Si se consigue se el envia la instruccion
      enviarInstruccion(subdirectorio, instruccion);
    } else {
      // si no se consigue se envia una orden al front controller
      // para indicar que hubo un error y que imprima un prompt
      orden(c_erroryprompt("descender", ENOENT));
    }
  }
}

/**
 * Se encarga de llamar a procesarSubdirectorio, esta
 * funcion se llama cuando se hace scandir.
 * @param dir contiene informacion de un directorio
 * @return retorna cero siempre
 */
int filter(struct dirent const * dir);

/**
 * Se crea una instruccion global que contenga la
 * instruccion de find actual, esto nos permite saber en
 * que directorio estamos al momento de hacer find
 */
Instruccion * findActual;

/**
 * Se utiliza para buscar en el directorio
 * cuyos datos estan en el dirent dado si
 * se encuentra el patron deseado en el find
 * que se realiza actualmente. Esta funcion se utiliza
 * al llamar a scandir cuando hacemos find.
 * @param dirent contiene la informacion del
 * directorio actual
 * @return Retorna cero siempre.
 */
int buscar(struct dirent const * dirent) {
  struct stat infoArchivo;
  // Guardamos en dir el nombre del directorio
  char const * dir = dirent->d_name;
  // Si el archivo es el mismo o su padre entonces
  // retornamos cero
  if (!strcmp(dir, ".") || !strcmp(dir, "..")) return 0;
  // Buscamos la informacion del archivo
  if (-1 == stat(dir, &infoArchivo)) {
    switch (errno) {
      // Si stat da errores se le envia una orden al front controller que
      // indique que hubo un error con stat y se mata al actor.
      case EFAULT:
      case ENOMEM:
      case EBADF:
        orden(c_error("stat", errno));
        muere();
        break;

      default: break;
    }
    // Finalmente se retorna cero cuando da error
    return 0;
  }

  // creamos una nueva variable donde guardamos el camino en ella
  // guardamos el directorio origen del find actual mas el directorio en el
  // que estamos actualmente separados por un "/"
  char * camino;
  if (-1 == asprintf(&camino, "%s/%s", findActual->argumentos.dosPath.origen, dirent->d_name)) {
    // Si hay un error haciendo asprintf se retorna cero
    return 0;
  }

  // Se compara el camino con el patron buscado en el find
  if (strstr(camino, findActual->argumentos.dosPath.destino)) {
    char * caminoNL;
    // Si son iguales se crea un nuevo camino que se encuentre
    // terminado con un new line
    if (-1 == asprintf(&caminoNL, "%s\n", camino)) {
      // Si asprintf da error se retorna cero
      return 0;
    }
    // Se envia una orden al front controller para que imprima
    // el camino que hace match con el patron buscado.
    orden(c_imprime(caminoNL));
    // Liberamos el espacio ocupado por caminoNL
    free(caminoNL);
  }

  if (S_ISDIR(infoArchivo.st_mode)) {
    // Si el archivo es un directorio entonces se crea una nueva instruccion
    // para sustituir al find actual y se le cambia el path de origen que
    // ahora sera el camino que se contstruyo
    Instruccion instruccionSubdirectorio = *findActual;
    instruccionSubdirectorio.argumentos.dosPath.origen = camino;
    // Finalmente se envia una direccion al directorio actual
    // que mande a buscar el patron en sus subdirectorios
    enviarInstruccion(buscarLibreta(dirent->d_name), instruccionSubdirectorio);
  }

  // Se libera el espacio ocupado por camino y se retorna 0
  free(camino);
  return 0;
}



/**
 * Se encarga de determinar que va a hacer el actor segun
 * el mensaje que se le envie.
 * @param mensaje mensaje que contiene la instruccion a realizar
 * @param datos datos que se le envian a la funcion
 * @return Retorna un actor que es la manera en que se va a comportar
 * este actor ante la llegada del siguiente mensaje
 */
Actor despachar(Mensaje mensaje, void * datos);

/**
 * Cuando se hace find los directorios deben esperar a que sus subdirectorios
 * terminen de ejecutar el find antes de retornar ellos. Esta funcion se encarga
 * de esperar a todos los hijos que se encuentren haciendo find antes de retornar.
 * @param mensaje mensaje que contiene una instruccion
 * @param datos datos que se le envian a la funcion
 * @return Retorna un actor, esta es la manera en como el actor va a responder
 * ante el siguiente mensaje que le llegue
 */
Actor esperarFind(Mensaje mensaje, void * datos) {
  // Se deserializa la instruccion contenida en el mensaje
  Instruccion instruccion = deserializar(mensaje);
  // En los datos que se le pasan a la funcion se encuentra el numero
  // de hijos de este actor
  int * numHijos = (int *)datos;

  // Segun el selector de la instruccion se decide que hacer
  switch (instruccion.selector) {
    // Si es una instruccion de muere
    case SI_MUERE: {
      // se manda a morir a cada actor contenido en la libreta de direcciones
      struct libreta * entLibreta;
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        enviar(entLibreta->direccion, mensaje);
      }
      // Luego se espera por la muerte de cada uno de esos actores
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        esperar(entLibreta->direccion);
      }
      // Se libera la libreta
      liberarLibreta();
      // Se libera el espacio ocupado por mensaje.contenido
      free(mensaje.contenido);
      // Se libera el espacio ocupado por numHijos
      free(numHijos);
      // Retornamos finActor (actor nulo)
      return finActor();
    }

    // Si es una señal de termine, significa que uno de sus hijos termino
    case SI_INVALIDO:
      //reduzco en 1 el numero de hijos
      --*numHijos;
      // Libero el espacio ocupado por mensaje.contenido
      free(mensaje.contenido);

      // Si el numero de hijos actual no ha llegado a cero retorno un
      // actor que espere por sus demas hijos.
      if (0 != *numHijos) {
        return mkActor(esperarFind, numHijos);
      }

      // Si el numero de hijos ya llego a cero, existen dos posibilidades:
      // Si soy la raiz envio al front controller una orden para que imprima un prompt
      if (soyRaiz) orden(c_prompt());
      // Si no soy la raiz envio una instruccion al padre diciendo que termine
      else enviar(buscarLibreta(".."), mkMensaje(0, NULL));

      // Libero el espacio utilizado por numHijos
      // y retorno un actor cuyo comportamiento sea despachar
      free(numHijos);
      return mkActor(despachar, NULL);

    default:
      // Si llega a este codigo es porque hubo un error interno de modo
      // que se envia una orden al front controller para que imprima
      // un mensaje de error y luego ejecuto muere
      orden(c_imprime("esperarFind: mensaje inesperado\n"));
      muere();
      // Finalmente retorno un actor cuyo comportamiento se defina por esperarFind
      return mkActor(esperarFind, numHijos);
  }
}

/**
 * Se encarga de determinar que va a hacer el actor segun
 * el mensaje que se le envie.
 * @param mensaje mensaje que contiene la instruccion a realizar
 * @param datos datos que se le envian a la funcion
 * @return Retorna un actor que es la manera en que se va a comportar
 * este actor ante la llegada del siguiente mensaje
 */
Actor despachar(Mensaje mensaje, void * datos) {
  // En el mensaje se encuentra una instruccion, la deserializamos
  Instruccion instruccion = deserializar(mensaje);

  // Segun su selector se decide que hacer
  switch (instruccion.selector) {
    // Si es una instruccion muere se matan a todos los procesos en la libreta
    case SI_MUERE: {
      struct libreta * entLibreta;
      // Se les envia un mensaje con una instruccion de muerte a cada uno
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        enviar(entLibreta->direccion, mensaje);
      }
      // El actor actual espera por cada uno de ellos
      for (entLibreta = libreta; entLibreta; entLibreta = entLibreta->siguiente) {
        esperar(entLibreta->direccion);
      }
      // Se libera el espacio ocupado por la libreta y mensaje.contenido
      liberarLibreta();
      free(mensaje.contenido);
      return finActor();
    }

    // En caso de ser alguna de las instrucciones siguientes se llama a
    // descender con su funcion correspondiente.
    case SI_MKDIR      : descender(do_mkdir      , instruccion); break;
    case SI_RM         : descender(do_rm         , instruccion); break;
    case SI_RMDIR      : descender(do_rmdir      , instruccion); break;
    case SI_LS         : descender(do_ls         , instruccion); break;
    case SI_CAT        : descender(do_cat        , instruccion); break;
    case SI_CP         : descender(do_cp         , instruccion); break;
    case SI_MV         : descender(do_mv         , instruccion); break;
    case SI_WRITE      : descender(do_write      , instruccion); break;
    case SI_WRITEYBORRA: descender(do_writeyborra, instruccion); break;

    // Si se trata de un find la estructura es un poco diferente.
    case SI_FIND: {
      struct dirent ** listaVacia;
      // Se actualiza el find actual con la instruccion actual
      findActual = &instruccion;
      // Se hace scandir con la funcion busca definida anteriormente
      if (-1 == scandir(".", &listaVacia, buscar, NULL)) {
        // Si da error se envia una instruccion al front controller
        // que indique que hubo un error
        orden(c_error("scandir", errno));
        // Se llama a la funcion muere
        muere();
      }
      // Liberamos el contenido del mensaje
      free(mensaje.contenido);
      // Reservo espacio para el apuntador al numero de hijos
      int * hijosEsperar = malloc(sizeof(int));
      if (!hijosEsperar) {
        // Si da error se envia una instruccion al front controller
        // que indique que hubo un error
        orden(c_error("malloc", errno));
        // Se llama a la funcion muere
        muere();
      }
      // Se calcula el numero de hijos por el buq tiene que esperar
      *hijosEsperar = numeroHijos();
      if (0 == *hijosEsperar) {
        // Si no tengo que esperar a ningun hijo libero
        // el espacio ocupado por hijosEsperar
        // Si soy la raiz mando al front controller a imprimir un prompt
        if (soyRaiz) orden(c_prompt());
        // En caso contrario se busca al padre en la liberta y
        // se le envia un mensaje de terminacinon
        else enviar(buscarLibreta(".."), mkMensaje(0, NULL));
        free(hijosEsperar);
        // retornamos un actor cuyo comportamiento ante el siguiente mensa.
        return mkActor(despachar, datos);
      }
      // En caso contrario retornamos un actor que tenga su encontrar Fijo
      // y los demas son sus compañeros de cuarto
      return mkActor(esperarFind, hijosEsperar);
    }

    // En caso de ser un ls para todos los archivos dentro de un directorio
    case SI_LSALL: {
      struct dirent ** listaVacia;
      // Para cada uno de los directorio (se recorre con
      // scandir) se hace filterLs
      if (-1 == scandir(".", &listaVacia, filterLs, NULL)) {
        // Si hay un error no recuperable se envia una instruccion al
        // front controller indicando que hubo un error
        if (ENOMEM == errno) {
          orden(c_error("scandir", errno));
          muere();
        } else {
          // En caso de que haya un error recuperable se envia una instruccion
          // al front controller indicando que hubo un error y se le indica
          // que imprima un nuevo prompt
          orden(c_erroryprompt("scandir", errno));
        }
      }
      // Si se termina adecuadamente se envia una instruccion al front controller
      // para que imprima un nuevo prompt
      orden(c_prompt());
    } break;

    default: break;
  }
  // Liberamos el espacio de memoria compartiva. Y se retorna
  // el mensaje es invalido
  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

/**
 * Se encarga de crear a un nuevo actor y agregarlo
 * a la libreta de direcciones.
 * @param mensaje mensaje que se le envia al actor
 * @param datos datos que se le pasan para que trabaje
 * con ellos
 * @return Retorna el actor con el comportamiento con el
 * cual manejara el proximo mensaje
 */
Actor contratar(Mensaje mensaje, void * datos) {
  // Si la libreta no existe es porque soy la raiz
  if (!libreta) {
    // Indico en la variable global que soy la raiz
    soyRaiz = 1;
    // Agrego a la direccion de la libreta mi direccion
    // bajo el nombre de ..
    char * puntopunto;
    asprintf(&puntopunto, "..");
    insertarLibreta(puntopunto, miDireccion());
  }

  // Si soy un subdirectorio de la raiz agrego
  // mi direccion a la libreta de direcciones bajo
  // el nombre de .
  char * punto;
  asprintf(&punto, ".");
  insertarLibreta(punto, miDireccion());

  //Si hay un error al momento de intentar descender por
  //ese directorio se envia una orden al front controller
  //que indique que hubo un error
  if (-1 == chdir(mensaje.contenido)) {
    orden(c_error("chdir", errno));
    muere();
  } else {
    // En caso de que podamos descender por el directorio
    // hacemos scandir para crear los hijos actores correspondientes
    // a sus subdirectorios
    struct dirent ** listaVacia;
    if (-1 == scandir(".", &listaVacia, filter, NULL)) {
      // Si hay un error en el scandir se envia una orden al
      // front controller que indique que hubo un error y matamos
      // al actor.
      orden(c_error("scandir", errno));
      muere();
    }
  }

  // finalmente se libera el espacio ocupado por el
  // contenido del mensaje y se retorna un actor cuyo
  // comportamiento sea despachar
  free(mensaje.contenido);
  return mkActor(despachar, datos);
}

/**
 * Se encarga de manejar al padre del directorio que
 * se procesa actualmente, agrega su direccion a la libreta
 * de direcciones global.
 * @param mensaje que contiene la direccion del padre
 * @param datos datos con los que puede trabajar la funcion
 * @return Retorna un actor con el comportamiento de
 * contratar.
 */
Actor manejarPapa(Mensaje mensaje, void * datos);

/**
 * Se encarga de procesar el subdirectorio creado; es decir,
 * crear una nueva entrada en la libreta de direcciones y un
 * actor que esté asociado a este nuevo directorio.
 * @param dir direccion del nuevo directorio creado
 */
void procesarSubdirectorio(char const * dir) {
  // Creamos una variable donde guardar la informacion
  // del archivo que obtengamos con stat
  struct stat infoArchivo;
  // Si la direccion del directorio es . o .. retornamos
  if (!strcmp(dir, ".") || !strcmp(dir, "..")) return;
  // Si stat da errores se le envia una orden al front controller que
  // indique que hubo un error con stat y se mata al actor.
  if (-1 == stat(dir, &infoArchivo)) {
    orden(c_error("stat", errno));
    muere();
  }

  // Si es un directorio
  if (S_ISDIR(infoArchivo.st_mode)) {
    char * nombre;
    // Creamos una variable para guardar el string del nombre
    asprintf(&nombre, "%s", dir);

    // Creamos un actor para este nuevo subdirectorio
    Direccion subdirectorio = crear(mkActor(manejarPapa, NULL));
    // Si hubo un error creando se envia una orden al front controller
    // que indique que hubo un error y se mata al actor.
    if (!subdirectorio) {
      orden(c_error("crear", errno));
      muere();
      return;
    }

    // En caso de que se cree con exito se agrega
    // el nuevo actor a la libreta de direcciones
    insertarLibreta(nombre, subdirectorio);

    // Enviamos un mensaje al nuevo actor con su nombre
    int retorno = enviar
      ( subdirectorio
      , mkMensaje(strlen(dir) + 1, (char *)dir)
      )
    ;
    // Si da un error al enviar el mensaje se envia una
    // orden al front controller indicando que hubo un error
    // y se mata al actor
    if (-1 == retorno) {
      orden(c_error("enviar", errno));
      muere();
      return;
    }
  }
}

/**
 * Se encarga de llamar a procesarSubdirectorio, esta
 * funcion se llama cuando se hace scandir.
 * @param dir contiene informacion de un directorio
 * @return retorna cero siempre
 */
int filter(struct dirent const * dir) {
  procesarSubdirectorio(dir->d_name);
  return 0;
}



/**
 * Se encarga de manejar al padre del directorio que
 * se procesa actualmente, agrega su direccion a la libreta
 * de direcciones global.
 * @param mensaje que contiene la direccion del padre
 * @param datos datos con los que puede trabajar la funcion
 * @return Retorna un actor con el comportamiento de
 * contratar.
 */
Actor manejarPapa(Mensaje mensaje, void * datos) {
  // Se libera la libreta acumulada anteriormente
  // ya que queremos que este actor solo tenga acceso a las
  // direcciones de su padre y de sus hijos.
  liberarLibreta();

  // Si tenemos que manejar al padre es porque no somos la raiz
  soyRaiz = 0;
  // Agregamos a la libreta de direcciones la direccion del padre
  // bajo el nombre de ..
  char * puntopunto;
  asprintf(&puntopunto, "..");
  insertarLibreta(puntopunto, deserializarDireccion(mensaje));

  // Liberamos el espacio que ocupa el contenido del mensaje.
  free(mensaje.contenido);
  // Retornamos un nuevo actor cuyo comportamiento sea contratar
  return mkActor(contratar, datos);
}



/**
 * Se encarga de imprimir por pantalla, es el
 * comportamiento del actor impresora.
 * @param mensaje mensaje que contiene la instruccion
 * con los datos que se van a imprimir
 * @param datos con los que trabaja el actor
 * @return Retorna un nuevo actor cuyo comportamiento
 * sea imprimir
 */
Actor imprimir(Mensaje mensaje, void * datos) {
  // Deserializamos el la instruccion contenida en el mensaje
  Instruccion instruccion = deserializar(mensaje);

  // Segun el selector sabemos que tipo de informacion vamos
  // a imprimir
  switch (instruccion.selector) {
    // Si es un muere matamos al actor.
    case SI_MUERE:
      return finActor();

    // Si es un imprime imprimimos el texto en los argumentos
    // de la instruccion.
    case SI_IMPRIME:
      fwrite(instruccion.argumentos.datos.texto, sizeof(char), instruccion.argumentos.datos.longitud, stdout);
      fflush(stdout);
      break;

    // Si es un imprime imprimimos el texto en los argumentos
    // de la instruccion y ademas enviamos una orden al front controller
    // para que imprima un nuevo prompt
    case SI_IMPRIMEYPROMPT:
      fwrite(instruccion.argumentos.datos.texto, sizeof(char), instruccion.argumentos.datos.longitud, stdout);
      fflush(stdout);
      orden(c_prompt());
      break;

    // En caso de ser un error se maneja el error con
    // perror y se le asigna a errno el codigo del error.
    case SI_ERROR:
      errno = instruccion.argumentos.error.codigo;
      perror(instruccion.argumentos.error.texto);
      break;

    // Si es un error y prompt se maneja de la misma manera
    // que antes solo qeu ahora se le envia una orden al
    // front controller para que imprima un nuevo prompt
    case SI_ERRORYPROMPT:
      errno = instruccion.argumentos.error.codigo;
      perror(instruccion.argumentos.error.texto);
      orden(c_prompt());
      break;

    default:
      break;
  }

  // Liberamos el espacio ocupado por el contenido del mensaje
  free(mensaje.contenido);
  // Retornamos un actor cuyo comportamiento sea imprimir (es decir,
  // como va a manejar este actor el proximo mensaje que le llegue)
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



/**
 * Tipo que contiene una estructura que sirve para
 * parsear un comando introducido en el shell.
 */
typedef
  /**
   * Estructura que contiene un selector para indicar
   * que tipo de comando es, primer y segundo argumento.
   */
  struct comando {
    /**
     * Tipo enumerado para los distintos tipos de comandos
     * que se pueden tener en el shell
     */
    enum selectorComando
      { SC_INVALIDO = 0 /**< Comando Invalido                                       */
      , SC_NADA         /**< Comando Nada (no se introduce comando alguno al prompt */
      , SC_LS           /**< Comando ls                                             */
      , SC_CAT          /**< Comando cat                                            */
      , SC_CP           /**< Comando cp                                             */
      , SC_MV           /**< Comando mv                                             */
      , SC_FIND         /**< Comando find                                           */
      , SC_RM           /**< Comando rm                                             */
      , SC_MKDIR        /**< Comando mkdir                                          */
      , SC_RMDIR        /**< Comando rmdir                                          */
      , SC_QUIT         /**< Comando quit                                           */
      }
      selector /**< Selector para el tipo de comando */
    ;

    char * argumento1; /**< Primer argumento */
    char * argumento2; /**< Segundo argumento */
  }
  Comando
;

/**
 * Se encarga de decodificar un comando leido
 * del prompt y retonar su selector correspondiente
 * dado el texto que se lee.
 * @param texto texto que vamos a verificar que
 * comando es
 * @return Retorna el valor adecuado para el selector
 * del comando.
 */
enum selectorComando decodificar(char * texto) {
  // Si el texto es igual a ls se retorna el selector de ls
  if (!strcmp(texto, "ls"   )) return SC_LS   ;
  // Si el texto es igual a cat se retorna el selector de cat
  if (!strcmp(texto, "cat"  )) return SC_CAT  ;
  // Si el texto es igual a cp se retorna el selector de cp
  if (!strcmp(texto, "cp"   )) return SC_CP   ;
  // Si el texto es igual a mv se retorna el selector de mv
  if (!strcmp(texto, "mv"   )) return SC_MV   ;
  // Si el texto es igual a find se retorna el selector de find
  if (!strcmp(texto, "find" )) return SC_FIND ;
  // Si el texto es igual a rm se retorna el selector de rm
  if (!strcmp(texto, "rm"   )) return SC_RM   ;
  // Si el texto es igual a mkdir se retorna el selector de mkdir
  if (!strcmp(texto, "mkdir")) return SC_MKDIR;
  // Si el texto es igual a rmdir se retorna el selector de rmdir
  if (!strcmp(texto, "rmdir")) return SC_RMDIR;
  // Si el texto es igual a quit se retorna el selector de quit
  if (!strcmp(texto, "quit" )) return SC_QUIT ;
  // Si no es ninguno de ellos se retorna el selector invalido.
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
}

/**
 * Se encarga de enviar un mensaje al front controller
 * para que imprima un mensaje que indique al usuario
 * que el comando insertado es invalido
 * @param comando comando insertado por el usuario
 */
void comandoInvalido(Comando * comando) {
  // Se le envia una orden al front controller de que
  // imprima que el comando es invalido y ademas imprima
  // un nuevo prompt
  orden(c_imprimeyprompt("Comando inválido\n"));
  // Asigna el selector del comando insertado como invalido
  // ya que puede ser un comando invalido por no existir
  // o porque los argumentos del comando esten mal especificados
  comando->selector = SC_INVALIDO;
}

/**
 * Se encarga de ejercer las funciones del prompt
 */
void prompt() {
  // Hacemos fetch del comando
  Comando comando = fetch();

  // Segun el selector del comando se verifica que no
  // sea un comando invalido.
  switch (comando.selector) {
    // En caso de ser un comando invalido se llama
    // a la funcion comandoInvalido
    case SC_INVALIDO:
      comandoInvalido(&comando);
      break;

    // En caso de ser nada hace break
    case SC_NADA:
      break;

    // En caso de que sea quit, si hay algun argumento ademas
    // de la palabra quit ese comando se torna invalido.
    case SC_QUIT:
      if (comando.argumento1 || comando.argumento2) comandoInvalido(&comando);
      break;

    // Para el caso de ls, cat, find, rm, mkdir y rmdir se trabaja
    // con un solo argumento asi que este comando se torna invalido
    // cuando hay un segundo argumento o cuando el primero esta ausente.
    case SC_LS:
    case SC_CAT:
    case SC_FIND:
    case SC_RM:
    case SC_MKDIR:
    case SC_RMDIR:
      if (!comando.argumento1 || comando.argumento2) comandoInvalido(&comando);
      break;

    // Para el caso de cp y mv necesitamos dos argumentos, asi que
    // la falta de alguno de ellos invalida el comando
    case SC_CP:
    case SC_MV:
      if (!comando.argumento1 || !comando.argumento2) comandoInvalido(&comando);
      break;
  }

  // Una vez que se verifica que el comando tiene el numero de argumentos
  // adecuados se procede a efectuar la accion segun el tipo de comando que sea
  switch (comando.selector) {
    // En caso de ser invalido no se hace nada
    case SC_INVALIDO: break;

    // En caso de ser vacio se imprime un nuevo prompt (a traves de una orden
    // al front controller)
    case SC_NADA:
      orden(c_imprimeyprompt(""));
      break;

    // En caso de ser quit mata a los actores.
    case SC_QUIT: muere(); break;

    // En caso de ser mkdir, llama a orden con el constructor para la instruccion mkdir
    case SC_MKDIR: orden(c_mkdir(comando.argumento1)); break;
    // En caso de ser rm, llama a orden con el constructor para la instruccion rm
    case SC_RM   : orden(c_rm   (comando.argumento1)); break;
    // En caso de ser rmdir, llama a orden con el constructor para la instruccion rmdir
    case SC_RMDIR: orden(c_rmdir(comando.argumento1)); break;
    // En caso de ser ls, llama a orden con el constructor para la instruccion ls
    case SC_LS   : orden(c_ls   (comando.argumento1)); break;
    // En caso de ser cat, llama a orden con el constructor para la instruccion cat
    case SC_CAT  : orden(c_cat  (comando.argumento1)); break;

    // En caso de ser cp, llama a orden con el constructor para la instruccion cp
    case SC_CP: orden(c_cp(comando.argumento1, comando.argumento1, comando.argumento2)); break;
    // En caso de ser mv llama a orden con el constructor para la instruccion mv
    case SC_MV: orden(c_mv(comando.argumento1, comando.argumento1, comando.argumento2)); break;

    // En caso de ser find, llama a orden con el constructor para la instruccion find
    case SC_FIND: orden(c_find(".", ".", comando.argumento1)); break;

    default:
      // Si no es alguna de las anteriores se envia una orden al front
      // controller para que imprima que la instruccion no fue encontrada
      // e imprima un nuevo prompt
      orden(c_imprimeyprompt("Instruccion no encontrada\n"));
      break;
  }
}


/**
 * Se encarga de realizar las acciones del
 * front controller.
 * @param mensaje mensaje que le llega al front
 * controller con la instruccion
 * @datos datos con los que trabajara el actor que
 * retorne
 * @return Retorna un nuevo actor que indica como se comportara
 * el front controller cuando le llegue el proximo mensaje.
 */
Actor accionFrontController(Mensaje mensaje, void * datos) {
  // Primero debemos deserializar la instruccion contenida
  // en el mensaje
  Instruccion instruccion = deserializar(mensaje);

  // Segun el selector de la instruccion se decide que hacer
  switch (instruccion.selector) {
    // Si la instruccion es un muere
    // el front controller mata a la raiz
    // mata a la impresora y espera a que ellos mueran
    case SI_MUERE:
      enviar(raiz, mensaje);
      enviar(impresora, mensaje);
      esperar(raiz);
      esperar(impresora);
      // finalmente libera el contenido del mensaje y
      // muere
      free(mensaje.contenido);
      return finActor();

    // En caso de ser un imprime, imprime y prompt,
    // error o error y prompt se reenvia el mensaje
    // a la impresora
    case SI_IMPRIME:
    case SI_IMPRIMEYPROMPT:
    case SI_ERROR:
    case SI_ERRORYPROMPT:
      enviar(impresora, mensaje);
      break;

    // En caso de ser un prompt el front controller
    // llama a la funcion prompt
    case SI_PROMPT:
      prompt();
      break;

    // Por defecto si es cualquier otro mensaje
    // se lo reenvia a la raiz.
    default:
      enviar(raiz, mensaje);
      break;
  }

  // Se libera el espacio ocupado por el contenido del mensaje
  free(mensaje.contenido);
  // Retorna un actor cuya accion sea accionFrontController
  // (esta es la manera en que el front controller se comportara
  // cuando le llegue una nueva instruccion)
  return mkActor(accionFrontController, datos);
}

/**
 * Se encarga de las funciones que cumple el front controller
 * la primera vez que es llamado al inicio de la ejecucion
 * @param mensaje mensaje que le llega al front controller
 * @param datos con los que trabaja el actor que retorna
 * @return Retorna un actor que representa como se comportara
 * el front controller ante un nuevo mensaje
 */
Actor inicioFrontController(Mensaje mensaje, void * datos) {
  // Asignamos a la direccion global del front controller
  // su direccion para que todos los actores le puedan escribir
  frontController = miDireccion();

  // Crea al actor encargado de la impresora (sin enlazar)
  impresora = crearSinEnlazar(mkActor(imprimir, NULL));
  // Si no se puede crear se envia la instruccion de muere
  if (!impresora) {
    muere();
  }

  // Crea al actor encargado de la raiz (sin enlazar)
  raiz = crearSinEnlazar(mkActor(contratar, NULL));
  // Si no se puede crear se envia una orden al front controller
  // que indique que hubo un error y se ejecuta la instruccion de muere
  if (!raiz) {
    orden(c_error("crearSinEnlazar", errno));
    muere();
  }

  // Enviamos el primer mensaje a la raiz
  if (-1 == enviar(raiz, mensaje)) {
    // Si hay un error enviando se envia una orden al front
    // controller para indicar que hubo un error y se
    // ejecuta la instruccion de muere
    orden(c_error("enviar", errno));
    muere();
  }

  // Finalmente se envia una orden al front controller
  // para que imprima el primer prompt
  orden(c_prompt());

  // Liberamos el espacio ocupado por el contenido del mensaje
  free(mensaje.contenido);
  // Se retorna el actor con el comportamiento del front controller
  // ante un nuevo mensaje
  return mkActor(accionFrontController, datos);
}



/**
 * Como queremos que el shell no pueda ser matado con ctrl+c
 * y afines creamos un nuevo manejador de señales para estas
 * @param s señal que va a manejar
 */
void handler(int s) {
  // Se imprime un mensaje al usuario indicando que no se puede
  // matar al shell con una señal.
  printf("\nNo es posible matarmme con una señal jijiji >:3 tienes que usar quit\nchelito: ");
  fflush(stdout);
}

/**
 * Main del programa, se encarga de crear el front controller
 * y enviarle el primer mensaje para comenzar la ejecucion del
 * shell
 */
int main(int argc, char * argv[]) {
  // Si el numero de argumentos con que se invoca al
  // shell es erroneo se imprime un mensaje al usuario
  // indicando su uso.
  if (2 != argc) {
    fprintf(stderr, "Uso: %s <directorio>\n", argv[0]);
    exit(EX_USAGE);
  }

  // Se indica que se ignoran la señal SIGQUIT
  // en caso de que signal de error, se maneja
  // con perror
  if (SIG_ERR == signal(SIGQUIT, SIG_IGN)) {
    perror("signal");
    exit(EX_OSERR);
  }

  // Se indica que se ignora la señal SIGINT
  // en caso de que signal de error, se maneja
  // con perror
  if (SIG_ERR == signal(SIGINT, SIG_IGN)) {
    perror("signal");
    exit(EX_OSERR);
  }

  // Creamos el actor para el front controller (sin enlazar)
  frontController = crearSinEnlazar(mkActor(inicioFrontController, NULL));
  if (!frontController) {
    // Si hay un error se maneja con perror
    perror("crear");
    exit(EX_IOERR);
  }

  // Se le envia el primer mensaje al front controller indicandole
  // el directorio raiz con el que va a trabajar el shell
  if (-1 == enviar(frontController, mkMensaje(strlen(argv[1]) + 1, argv[1]))) {
    // Si hay un error al enviar se maneja con perror
    perror("enviar");
    exit(EX_IOERR);
  }

  // Se indica que la señal SIGQUIT se maneja con
  // el nuevo handler,
  // en caso de que signal de error, se maneja
  // con perror
  if (SIG_ERR == signal(SIGQUIT, handler)) {
    perror("signal");
    exit(EX_OSERR);
  }

  // Se indica que la señal SIGINT se maneja con
  // el nuevo handler,
  // en caso de que signal de error, se maneja
  // con perror
  if (SIG_ERR == signal(SIGINT, handler)) {
    perror("signal");
    exit(EX_OSERR);
  }

  // Se espera a que el actor front controller
  // termine su ejecucion
  esperar(frontController);
  // Finalmente salimos del programa con EX_OK
  exit(EX_OK);
}
