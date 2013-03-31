/**
 * @file actores.c
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Contiene la implementacion de la libreria con
 * las funciones necesarias para el trabajo con actores.
 *
 */
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <sys/file.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "actores.h"

/**
 * Estructura que contiene la direccion de
 * un actor. La comunicacion entre actores se realiza
 * mediante pipes, entonces esta estructura tiene tres campos;
 * el file descriptor de la punta de lectura (lugar por donde
 * el actor lee los mensajes que le llegan), el file descriptor
 * de la punta de escritura (lugar por donde pueden escribirle
 * mensajes otros actores) y el pid del proceso asociado al actor.
 * En caso de que no se tenga permiso de lectura o de escritura el
 * valor de estos campos será -1
 */
struct direccion {
  int fdLec; /**< file descriptor de la punta de lectura del pipe   */
  int fdEsc; /**< file descriptor de la punta de escritura del pipe */
  pid_t pid; /**< pid del proceso asociado al actor                 */
};

/**
 * Variable global que contiene la direccion del actor
 * actual, es de uso interno en este codigo; es decir,
 * no se exporta para el uso público.
 */
Direccion yo;

/**
 * Es el constructor de las direcciones. Dados los file
 * descriptors de las puntas de los pipes y el pid del
 * proceso asociado al actor se crea una direccion para el mismo.
 * @param fdLec file descriptor de la punta de lectura, por ahi llegan
 * los mensajes al actor, puede ser -1 si no queremos que haya permiso
 * de lectura.
 * @param fdEsc file descriptor de la punta de escritura, por ahi se le
 * escriben mensajes al actor, puede ser -1 si no queremos que haya permiso
 * de escritura.
 * @param pid pid del proceso asociado al actor.
 * @return Retorna la direccion ya construida en un elemento del tipo Direccion,
 * en caso de error retorna NULL.
 */
Direccion crearDireccion(int fdLec, int fdEsc, pid_t pid) {
  // Reservamos espacio para la nueva direccion.
  Direccion direccion = malloc(sizeof(struct direccion));
  // Si hay un error al reservar la memoria se  devuelve NULL y
  // se asigna errno con ENOMEM
  if (NULL == direccion) {
    return NULL;
  }

  // Si se puede reservar la memoria adecuadamente, se asignan
  // los valores dados en los parametros a los campos de la
  // variable nueva.
  direccion->fdLec = fdLec;
  direccion->fdEsc = fdEsc;
  direccion->pid   = pid  ;

  // Retornamos la direccion creada.
  return direccion;
}

/**
 * Se encarga de verificar si una direccion es válida para
 * escritura, es decir, si podemos enviar mensajes a esta
 * direccion.
 * @param direccion direccion cuyo permiso queremos revisar
 * @return retorna 0 si no hay permiso para escritura (false) y
 * cualquier otro numero diferente de cero si hay permiso para
 * escritura (true)
 */
int validoEsc(Direccion direccion) {
  // Retorna cero cuando no se cumple que haya una direccion
  // y que su campo del file descriptor de escritura no sea
  // invalido.
  return direccion && -1 != direccion->fdEsc;
}

/**
 * Se encarga de verificar si una direccion es válida para
 * lectura, es decir, si podemos leer mensajes de esta
 * direccion.
 * @param direccion direccion cuyo permiso queremos revisar
 * @return retorna 0 si no hay permiso para lectura (false) y
 * cualquier otro numero diferente de cero si hay permiso para
 * lectura (true)
 */
int validoLec(Direccion direccion) {
  // Retorna cero cuando no se cumple que haya una direccion
  // y que su campo del file descriptor de lectura no sea
  // invalido.
  return direccion && -1 != direccion->fdLec;
}

/**
 * Se encarga de verificar si una direccion es válida para
 * espera, es decir, si podemos hacer wait del proceso asociado
 * a este actor.
 * @param direccion direccion cuya validez queremos revisar
 * @return retorna 0 si no es una direccion valida para esperar (false) y
 * cualquier otro numero diferente de cero si es una direccion
 * que podemos esperar (true)
 */
int validoEsp(Direccion direccion) {
  // Retorna cero cuando no se cumple que haya una direccion
  // y que su campo de píd no sea invalido.
  return direccion && -1 != direccion->pid;
}

/**
 * Se encarga de liberar el espacio de memoria ocupado por
 * la direccion dada.
 * @param direccion direccion que queremos liberar.
 */
void liberaDireccion(Direccion direccion) {
  //TODO
  //if (validoEsc(direccion)) close(direccion->fdEsc);
  //if (validoLec(direccion)) close(direccion->fdLec);
  free(direccion); // Liberamos el espacio ocupado por direccion.
}

/**
 * Se encarga de agregar a un mensaje mas contenido. Es una
 * funcion de conveniencia que permite que un actor cree
 * mensajes sin tener que preocuparse por el manejo de memoria.
 * @param mensaje mensaje al cual le vamos a agregar contenido.
 * @param fuente lugar de origen del nuevo contenido que vamos a
 * agregar al final.
 * @param cantidad cantidad de contenido nuevo que vamos a
 * agregar al mensaje.
 * @return Retorna un enterio si logra agregar exitosamente
 * y -1 si hay un error.
 */
int agregaMensaje(Mensaje * mensaje, void * fuente, int cantidad) {
  // Si hacer realloc da error entonces el define errno con un valor
  // apropiado (ENOMEM) y retorna -1
  if (NULL == (mensaje->contenido = realloc(mensaje->contenido, mensaje->longitud + cantidad))) {
    return -1;
  }
  // Cuando tenemos nuevo espacio de memoria copiamos en ese espacio nuevo
  // el contenido a agregar al mensaje.
  memcpy(mensaje->contenido + mensaje->longitud, fuente, cantidad);
  // Aumentamos la longitud del mensaje.
  mensaje->longitud += cantidad;
  // Retornamos exitosamente
  return 0;
}

/**
 * Se encarga de serializar una direccion para enviarla
 * a traves del contenido de un mensaje.
 * @param direccion direccion que vamos a serializar
 * @return Retorna el mensaje construido resultante de
 * serializar la direccion dada.
 */
Mensaje serializarDireccion(Direccion direccion) {
  // Creamos un nuevo mensaje
  Mensaje mensaje = {};

  // Utilizamos la funcion de agregar mensaje para ir agregando
  // cada parte del mensaje (el file descriptor de lectura,
  // el de escritura y el pid)
  agregaMensaje(&mensaje, &direccion->fdLec, sizeof(direccion->fdLec));
  agregaMensaje(&mensaje, &direccion->fdEsc, sizeof(direccion->fdEsc));
  agregaMensaje(&mensaje, &direccion->pid  , sizeof(direccion->pid));

  // Retornamos el mensaje construido
  return mensaje;
}

/**
 * Se encarga de deserializar una direccion contenida en
 * un mensaje. Cuando se envia una direccion a traves de
 * los datos de un mensaje esta funcion se encarga de
 * deserializarla y devolverla en una forma del tipo Direccion.
 * @param mensaje mensaje que contiene la direccion a deserializar.
 * @return Retorna la direccion contenida en el mensaje en algo
 * de tipo Direccion.
 */
Direccion deserializarDireccion(Mensaje mensaje) {
  // El primer contenido del mensaje sera el file descriptor de lectura
  // asi que casteamos a un apuntador a entero y despues desreferenciamos.
  int fdLec = *((int *)mensaje.contenido);
  // Luego tenemos el file descriptor de escritura, sumamos a la direccion
  // inicial el tamaño de un entero (primer file descriptor) y casteamos a
  // un apuntadora a entero y desreferenciamos para obtener el file
  // descriptor de escritura.
  int fdEsc = *((int *)(mensaje.contenido + sizeof(int)));
  // Finalmente para obtener el pid le sumamos a la direccion inicial
  // el tamaño de dos enteros (los primeros file descriptors) y
  // casteamos a apuntadora a pid_t y desreferenciamos para
  // obtener el pid
  pid_t pid = *((pid_t *)(mensaje.contenido + sizeof(int) * 2));

  // Retornamos la direccion que se crea con la informacion que sacamos
  // del mensaje.
  return crearDireccion(fdLec, fdEsc, pid);
}

/**
 * Se encarga de retornar la direccion del actor que llama a la funcion.
 * @return direccion del actor que llama a la funcion en caso de
 * que no tenga direccion retorna NULL.
 */
Direccion miDireccion() {
  // Si yo es una direccion vacia retorna NULL
  if (!yo) return NULL;
  // Sino devolvemos la direccion del actor actual para
  // escribirle mensajes
  return crearDireccion(-1, yo->fdEsc, -1);
}



/**
 * Es el constructor de un mensaje, se encarga de construir
 * algo del tipo Mensaje.
 * @param longitud indica la longitud que tendra el mensaje.
 * @param contenido indica el contenido del mensaje a ser enviado.
 * @return retorna el mensaje construido del tipo Mensaje.
 */
Mensaje mkMensaje
  ( ssize_t longitud
  , void * contenido
  )
{
  // Se crea un mensaje con la longitud y el contenido dado
  // y se retorna.
  Mensaje mensaje = { longitud, contenido };
  return mensaje;
}



/**
 * Es el constructor de actor, se encarga de construir un actor
 * del tipo actor dada una funcion que determine su comportamiento
 * y los datos que necesite el mismo.
 * @param actor es un apuntador a una funcion que toma un mensaje
 * y unos datos y retorna un actor, esta determina el comportamiento
 * del actor.
 * @param datos datos que se le pasan a la funcion que ejecuta el
 * actor.
 * @return retorna el actor creado.
 */
Actor mkActor
  ( Actor (*actor)(Mensaje mensajito, void * datos)
  , void * datos
  )
{
  // Se crea un actor con la funcion y los datos dados
  // y se retorna.
  Actor actorSaliente = { actor, datos };
  return actorSaliente;
}

/**
 * Se encarga de terminar a un actor, esto lo hace retornando
 * un actor cuya funcion de comportamiento sea nula.
 * @return retorna un actor nulo.
 */
Actor finActor() {
  return mkActor(NULL, NULL);
}

/**
 * Se encarga de correr la funcion del actor que determina su
 * comportamiento.
 * @param actor actor que vamos a ejecutar
 */
void correActor(Actor actor) {
  // Si no hay actor; es decir, recibimos un actor cuya funcion
  // sea NULL retornamos automaticamente.
  if (!actor.actor) return;

  // Creamos una variable del tipo mensaje.
  Mensaje mensajito;

  // Antes de ejecutar la funcion del actor se debe leer el mensaje
  // que se la haya enviado al actor.

  // Inicializamos un contador para guardar la longitud de los datos
  // que vienen en el mensaje
  ssize_t contador = sizeof(mensajito.longitud);
  // Tambien inicializamos en cero una variable que guarde el acumulado de lo leido.
  int acumulado = 0;
  // Aca se lee del pipe la longitud del mensaje.
  // Mientras no se haya terminado de leer del pipe se continua leyendo hasta
  // leer el numero.
  while (contador > 0) {
    // creamos una variable leido que cuente cuanto se leyo en la iteracion
    // leemos de la punta de lectura del pipe y guardamos en el campo "longitud"
    // del mensajito
    int leido = read(yo->fdLec, &mensajito.longitud + acumulado, contador);
    // Si hay un error al leer se hace exit y se maneja el error con perror
    if (-1 == leido) {
      perror("read");
      exit(EX_IOERR);
    }
    // Al contador le descontamos lo que ya leimos
    contador -= leido;
    // y al acumulado le sumamos lo que leimos
    acumulado += leido;
  }

  // En este punto ya tenemos la longitud de los datos que debemos leer a continuacion.
  // Si la longitud del mensaje es menor o igual a cero es porque su contenido es null
  // sino, procedemos a leer el contenido.
  if (0 >= mensajito.longitud) {
    mensajito.contenido = NULL;
  } else {
    // Reservamos espacio suficiente para guardar todo el contenido
    // del mensaje con malloc, si hay un error haciendo esto se maneja con perror
    // y hace exit.
    if (!(mensajito.contenido = malloc(mensajito.longitud))){
      perror("malloc");
      exit(EX_OSERR);
    }

    // Se crean variables auxiliares para la longitud y
    // un apuntador al contenido del mensaje.
    ssize_t longitud = mensajito.longitud;
    void * contenido = mensajito.contenido;

    // Se lee mientras aun queden cosas del contenido por leer.
    while (longitud > 0) {
      // Se lee del pipe la cantidad indicada en longitud y se guarda en
      // el espacio de memoria apuntado por contenido.
      int leido = read(yo->fdLec, contenido, longitud);
      // Si hay un error leyendo se maneja con perror y se hace exit.
      if (-1 == leido) {
        perror("read");
        exit(EX_IOERR);
      }
      // Le quitamos a la longitud la cantidad leida
      longitud -= leido;
      // y movemos el apuntador a contenido en la cantidad leida.
      contenido += leido;
    }
  }

  // Finalmente se ejecuta la funcion que define el comportamiento del actor.
  // La firma de esta funcion indica que esta retorna un actor, este actor es
  // el actor resultante luego de ejecutar la accion de esa funcion. correActor
  // se llama recursivamente con el actor que devuelve la funcion del actor actual.
  correActor(actor.actor(mensajito, actor.datos));
}



/**
 * Se encarga de crear un nuevo actor en un nuevo proceso.
 * El nuevo proceso es hijo del proceso que llama a crear.
 * Si el proceso que llama a la funcion crear es un actor,
 * se pueden enlazan ambos actores; es decir, el primer
 * mensaje que le llega al nuevo actor es la direccion de
 * su padre.
 * @param actor contiene el actor que se va a crear.
 * @param enlazar entero que indica si se va a enlazar o no.
 * Si se va a enlazar es 1, sino es 0.
 * @return Retorna la direccion del nuevo actor creado
 * mediante la cual se le pueden enviar mensajes, en caso
 * de no crearlo o error retorna NULL.
 */
Direccion crearActor(Actor actor, int enlazar) {
  // Si se pide enlazar y no conozco la direccion
  // yo, hay un error, se asigna errno y se retorna NULL.
  if (enlazar && !yo) {
    errno = ENXIO;
    return NULL;
  }

  /**
   * Tipo enumerado que indica los indices de los pipes
   */
  enum indicePipes { P_ESCRITURA = 1, P_LECTURA = 0 };
  int pipeAbajo[2];
  // Creamos un pipe de comunicación entre el padre y el hijo.
  pipe(pipeAbajo);

  /**
   * Si nos piden enlazar al actor con su padre, se le debe enviar
   * al mismo un mensaje con la direccion de su padre, tenemos que
   * asegurarnos de que este mensaje sea el primero que recibe este
   * actor, por lo tanto utilizamos un semaforo que permita que el
   * padre no haga cosas hasta que se le haya enviado dicho mensaje
   * al hijo.
   */

  // Necesitamos que sea un semaforo en un espacio de memoria compartida
  // para que tanto el padre como el hijo pueda acceder al mismo.
  // Para hacer esto utilizamos shmget para obtener un espacio de memoria
  // compartida donde quepa un semaforo.
  int shmid = shmget(IPC_PRIVATE, sizeof(sem_t), S_IRUSR | S_IWUSR);
  // Si da error, salimos el programa y se imprime en pantalla el
  // error correspondiente
  if (-1 == shmid) {
    perror("shmget");
    exit(EX_OSERR);
  }

  // Se llama a shmat para agregar el espacio de memoria compartida,
  // identificado por shmid, al espacio de direcciones del proceso y
  // se crea el semaforo.
  sem_t * sem = shmat(shmid, NULL, 0);
  // Si shmat da error, salimos del programa y se imprime en pantalla
  // el error correspondiente.
  if (!sem) {
    perror("shmat");
    exit(EX_OSERR);
  }

  // Inicializamos el semaforo.
  sem_init(sem, 1, 0);

  pid_t pid;
  //Hacemos fork
  switch (pid = fork()) {
    case -1: {
      // En caso de que fork falle, retorna NULL.
      return NULL;
    }

    case 0: {
      // Creamos una nueva variable para la direccion del papa
      Direccion papa = NULL;
      // En yo, se encuentra la direccion del padre de este actor.
      // Acá se está revalidando el invariante de que yo soy yo.

      // Si la direccion del padre no es null cerramos la punta de lectura
      // del pipe ya que el hijo no debe ser capaz de leer los mensajes
      // que se le mandan al padre. Luego construimos la direccion del padre
      // y liberamos el espacio de memoria ocupado por yo.
      if (yo) {
        close(yo->fdLec);
        papa = crearDireccion(-1, yo->fdEsc, -1);
        free(yo);
      }
      // Luego creamos la direccion nueva para yo con las puntas de lectura
      // y escritura del pipe y asignamos -1 al campo del pid ya que un proceso
      // no debe esperar por si mismo.
      yo = crearDireccion(pipeAbajo[P_LECTURA], pipeAbajo[P_ESCRITURA], -1);

      // Si la direccion del padre existe y piden que se cree al actor enlazado
      // se construye un mensaje que es el primero que recibe el actor, este
      // contiene la direccion de su padre.
      if (enlazar && papa) {
        // Creamos el mensaje serializando la direccion
        Mensaje mensaje = serializarDireccion(papa);
        // TODO enviar deberia ser cambiado por enviarme
        // El actor se envia a si mismo un mensaje con la direccion de su padre.
        enviar(yo, mensaje);
        // Liberamos el espacio de memoria ocupado por el contenido del mensaje.
        free(mensaje.contenido);
      }

      // hacemos signal porque ya enviamos el mensaje que necesitabamos
      // que llegara de primero.
      sem_post(sem);

      // Una vez utilizado liberamos la dirección del padre.
      liberaDireccion(papa);

      // Llamamos a la funcion que se encarga de correr el actor; es decir,
      // ejecutar la funcion que determina el comportamiento del mismo.
      correActor(actor);

      // Finalmente salimos exitosamente.
      exit(EX_OK);
    }

    default: {
      // Si es el padre, cerramos la punta de lectura del pipe.
      close(pipeAbajo[P_LECTURA]);
      // Creamos la direccion del nuevo actor hijo que creamos. En
      // ella solo incluimos el file descriptor de escritura para que
      // se le pueda escribir al nuevo actor y se incluye su pid.
      Direccion nuevoActor = crearDireccion(-1, pipeAbajo[P_ESCRITURA], pid);
      // Esperamos a que se termine de trabajar en el hijo (en caso
      // de haber enlazado estamos esperando a que se envie el mensaje
      // con la direccion del padre)
      sem_wait(sem);
      //Retornamos la direccion del nuevo actor.
      return nuevoActor;
    }
  }
}

/**
 * Se encarga de crear un nuevo actor en un nuevo proceso.
 * El nuevo proceso es hijo del proceso que llama a crear.
 * Si el proceso que llama a la funcion crear es un actor,
 * se enlazan ambos actores; es decir, el primer mensaje que
 * le llega al nuevo actor es la direccion de su padre.
 * @param actor contiene el actor que se va a crear.
 * @return Retorna la direccion del nuevo actor creado
 * mediante la cual se le pueden enviar mensajes. En
 * caso de no crearlo o error retorna NULL.
 */
Direccion crear(Actor actor) {
  return crearActor(actor, 1);
}

/**
 * Se encarga de crear un nuevo actor en un nuevo proceso.
 * El nuevo proceso es hijo del proceso que llama a crear.
 * Si el proceso que llama a la funcion crear es un actor,
 * no se enlazan ambos actores; es decir, el actor hijo no
 * tendra disponible la direccion de su padre
 * @param actor contiene el actor que se va a crear.
 * @return Retorna la direccion del nuevo actor creado
 * mediante la cual se le pueden enviar mensajes. En
 * caso de no crearlo o error retorna NULL.
 */
Direccion crearSinEnlazar(Actor actor) {
  return crearActor(actor, 0);
}



/**
 * Dada la direccion de un actor, si el actor actual
 * tiene permiso de esperarlo; es decir, es su padre,
 * entonces espera a que el proceso hijo asociado al
 * actor de la direccion dada muera.
 * @param mensaje mensaje que se le enviara al actor
 * @param direccion direccion del actor a quien se va
 * a esperar.
 * @return retorna 0 si lo hace exitosamente y -1 si
 * hay algun error
 */
int enviar(Direccion direccion, Mensaje mensaje) {
  // Inicializamos el valor de retorno
  int retorno = 0;
  // Si no tengo permiso para escribirle a ese actor se asigna
  // errno con EPERM y se retorna -1
  if (!validoEsc(direccion)) {
    errno = EPERM;
    return -1;
  }

  //TODO check errors :(
  // Queremos asegurarnos que al momento de escribir en los pipes
  // mas nadie pueda escribir en ellos hasta que se haya escrito el
  // mensaje completo, por ello utilizamos un lock en el file descriptor
  // de escritura de la direccion.
  flock(direccion->fdEsc, LOCK_EX);
  // Entramos en la seccion critica

  // Se simula una especie de ciclo infinito con una condicion de parada
  // que asegura que se quedara iterando una cantidad finita de veces.
  // Esto es para asegurarnos de que se logre escribir todo en el pipe
  // y que el programa no falle completamente por errores que pueden ser
  // recuperables.
  while (1) {
    // Primero escribimos la longitud del mensaje para que el receptor sepa
    // cuanto debe leer (de que tamaño es el mensaje)
    if (-1 == write(direccion->fdEsc, &mensaje.longitud, sizeof(mensaje.longitud))) {
      // Si hay un error de write y es EINTR (fue interrumpido) es posible recuperarlo
      // de modo que hacemos continue para entrar en la siguiente iteracion e intentar
      // hacer write de nuevo.
      if (EINTR == errno) continue;
      // Si falla por alguna de las otras razones no recuperables
      // se asigna para retornar -1 y se sale del ciclo.
      retorno = -1;
      break;
    }

    // Mientras no se haya escrito todo el mensaje se intenta escribir.
    while (mensaje.longitud > 0) {
      // Se lleva con un contador la cantidad que se ha escrito en el pipe.
      int escrito = write(direccion->fdEsc, mensaje.contenido, mensaje.longitud);
      // En caso de que write de un error (retorna -1)
      if (-1 == escrito) {
        // Si es EINTR es recuperable entonces se continua a la proxima
        // iteracion para que intente escribir hasta que lo logre.
        if (EINTR == errno) continue;
        // Si falla por alguna de las otras razones no recuperables
        // se asigna -1 para retornar y se sale del ciclo.
        retorno = -1;
        break;
      }

      // Se le resta a la longitud del mensaje lo que ya se ha escrito.
      mensaje.longitud -= escrito;
      // Se mueve el apuntador al contenido la cantidad que ya se ha escrito.
      mensaje.contenido += escrito;
    }
    // Al terminar de escribir todo sale del ciclo infinito con un break
    break;
  }
  //TODO check errors :(
  // Quitamos el lock del file descriptor ya que terminamos de trabajar
  // en la seccion crítica.
  flock(direccion->fdEsc, LOCK_UN);
  // Retornamos 0 en caso de exito y -1 en caso de error.
  return retorno;
}

/**
 * Permite que un actor pueda enviarse un mensaje a si mismo.
 * @param mensaje mensaje que se desea enviar al mismo actor
 * @return retorna 0 si fue exitoso y -1 si falla
 */
int enviarme(Mensaje mensaje) {
  // Retorna el resultado de enviar el mensaje a yo.
  return enviar(yo, mensaje);
}

/**
 * Dada la direccion de un actor, si el actor actual
 * tiene permiso de esperarlo; es decir, es su padre,
 * entonces espera a que el proceso hijo asociado al
 * actor de la direccion dada muera.
 * @param direccion direccion del actor a quien se va
 * a esperar.
 * @return retorna 0 si lo hace exitosamente y -1 si
 * hay algun error
 */
int esperar(Direccion direccion) {
  // Revisamos que sea una direccion valida para esperar
  if (!validoEsp(direccion)) {
    // Si no lo es, se asigna errno con EPERM y se retorna -1
    errno = EPERM;
    return -1;
  }

  // Si tenemos permiso para esperar al actor de la direccion
  int foo;
  // De nuevo utilizamos una especie de ciclo infinito, que
  // tiene garantizada una cantidad finita de iteraciones.
  // Esto con el fin de que si waitpid nos da un error recuperable
  // toda la ejecucion del programa no se vea afectada por esto y
  // se pueda hacer wait hasta que en efecto espere al proceso o
  // de un error no recuperable.
  while (1) {
    // Esperamos por el proceso asociado a este actor, en caso
    // de que waitpid de un error
    if (-1 == (foo = waitpid(direccion->pid, NULL, 0))) {
      // Si es un error que se puede recuperar como EINTR se continua en la
      // iteracion siguiente hasta poder esperar por el proceso.
      if (EINTR == errno) continue;
      // Cuando falla por alguna razon que no es recuperable, se retorna
      // -1 y se le pone el valor adecuado a errno.
      return -1;
    }
    // Si logro esperar por el proceso salimos del ciclo infinito.
    break;
  }

  // Retornamos cero porque fue exitoso.
  return 0;
}
