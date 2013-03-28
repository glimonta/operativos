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

// DEBUG: descomentar esto
//struct direccion {
//  int fdLec;
//  int fdEsc;
//  pid_t pid;
//};

Direccion yo;

Direccion crearDireccion(int fdLec, int fdEsc, pid_t pid) {
  Direccion direccion = malloc(sizeof(struct direccion));
  if (NULL == direccion) {
    // en caso de que falle se devuelve NULL y se asigna errno con ENOMEM
    return NULL;
  }
  direccion->fdLec = fdLec;
  direccion->fdEsc = fdEsc;
  direccion->pid   = pid  ;
  return direccion;
}

int validoEsc(Direccion direccion) {
  return direccion && -1 != direccion->fdEsc;
}

int validoLec(Direccion direccion) {
  return direccion && -1 != direccion->fdLec;
}

int validoEsp(Direccion direccion) {
  return direccion && -1 != direccion->pid;
}

void liberaDireccion(Direccion direccion) {
  //if (validoEsc(direccion)) close(direccion->fdEsc);
  //if (validoLec(direccion)) close(direccion->fdLec);
  free(direccion);
}

int agregaMensaje(Mensaje * mensaje, void * fuente, int cantidad) {
  if (NULL == (mensaje->contenido = realloc(mensaje->contenido, mensaje->longitud + cantidad))) {
    //esta funcion puede fallar con -1 en ese caso define errno con un valor apropiado. (ENOMEM)
    return -1;
  }
  memcpy(mensaje->contenido + mensaje->longitud, fuente, cantidad);
  mensaje->longitud += cantidad;
  return 0;
}

Mensaje serializarDireccion(Direccion direccion) {
  Mensaje mensaje = {};

  agregaMensaje(&mensaje, &direccion->fdLec, sizeof(direccion->fdLec));
  agregaMensaje(&mensaje, &direccion->fdEsc, sizeof(direccion->fdEsc));
  agregaMensaje(&mensaje, &direccion->pid  , sizeof(direccion->pid));

  return mensaje;
}

Direccion deserializarDireccion(Mensaje mensaje) {
  int fdLec = *((int *)mensaje.contenido);
  int fdEsc = *((int *)(mensaje.contenido + sizeof(int)));
  pid_t pid = *((pid_t *)(mensaje.contenido + sizeof(int) * 2));

  return crearDireccion(fdLec, fdEsc, pid);
}

Direccion miDireccion() {
  if (!yo) return NULL;
  return crearDireccion(-1, yo->fdEsc, -1);
}



Mensaje mkMensaje
  ( ssize_t longitud
  , void * contenido
  )
{
  Mensaje mensaje = { longitud, contenido };
  return mensaje;
}



Actor mkActor
  ( Actor (*actor)(Mensaje mensajito, void * datos)
  , void * datos
  )
{
  Actor actorSaliente = { actor, datos };
  return actorSaliente;
}

Actor finActor() {
  return mkActor(NULL, NULL);
}

void correActor(Actor actor) {
  if (!actor.actor) return;
  Mensaje mensajito;

  ssize_t contador = sizeof(mensajito.longitud);
  int acumulado = 0;
  while (contador > 0) {
    int leido = read(yo->fdLec, &mensajito.longitud + acumulado, contador);
    if (-1 == leido) {
      perror("read");
      exit(EX_IOERR);
    }
    contador -= leido;
    acumulado += leido;
  }

  if (0 >= mensajito.longitud) {
    mensajito.contenido = NULL;
  } else {
    if (!(mensajito.contenido = malloc(mensajito.longitud))){
      perror("malloc");
      exit(EX_OSERR);
    }

    ssize_t longitud = mensajito.longitud;
    void * contenido = mensajito.contenido;

    while (longitud > 0) {
      int leido = read(yo->fdLec, contenido, longitud);
      if (-1 == leido) {
        perror("read");
        exit(EX_IOERR);
      }
      longitud -= leido;
      contenido += leido;
    }
  }

  correActor(actor.actor(mensajito, actor.datos));
}



Direccion crearActor(Actor actor, int enlazar) {
  if (enlazar && !yo) {
    errno = ENXIO;
    return NULL;
  }

  enum indicePipes { P_ESCRITURA = 1, P_LECTURA = 0 };
  int pipeAbajo[2];
  pipe(pipeAbajo);

  int shmid = shmget(IPC_PRIVATE, sizeof(sem_t), S_IRUSR | S_IWUSR);
  if (-1 == shmid) {
    perror("shmget");
    exit(EX_OSERR);
  }
  sem_t * sem = shmat(shmid, NULL, 0);
  if (!sem) {
    perror("shmat");
    exit(EX_OSERR);
  }

  sem_init(sem, 1, 0);

  pid_t pid;
  switch (pid = fork()) {
    case -1: {
      // puede fallar por cualquiera de las razones por las que fork pueda fallar
      return NULL;
    }

    case 0: {
      // Esto es feo porque estoy revalidando el invariante
      // de que yo soy yo.
      Direccion papa = NULL;
      if (yo) {
        close(yo->fdLec);
        papa = crearDireccion(-1, yo->fdEsc, -1);
        free(yo);
      }
      yo = crearDireccion(pipeAbajo[P_LECTURA], pipeAbajo[P_ESCRITURA], -1);

      if (enlazar && papa) {
        Mensaje mensaje = serializarDireccion(papa);
        enviar(yo, mensaje);
        free(mensaje.contenido);
      }

      sem_post(sem);

      liberaDireccion(papa);

      correActor(actor);
      exit(EX_OK);
    }

    default: {
      close(pipeAbajo[P_LECTURA]);
      Direccion nuevoActor = crearDireccion(-1, pipeAbajo[P_ESCRITURA], pid);
      sem_wait(sem);
      return nuevoActor;
    }
  }
}

Direccion crear(Actor actor) {
  return crearActor(actor, 1);
}

Direccion crearSinEnlazar(Actor actor) {
  return crearActor(actor, 0);
}



int enviar(Direccion direccion, Mensaje mensaje) {
  int retorno = 0;
  if (!validoEsc(direccion)) {
    // falla cuando no tengo permiso para escribir, entonces se asigna errno con EPERM y retorna -1
    errno = EPERM;
    return -1;
  }

  //TODO check errors :(
  flock(direccion->fdEsc, LOCK_EX);
  //Seccion critica :)
  while (1) {
    if (-1 == write(direccion->fdEsc, &mensaje.longitud, sizeof(mensaje.longitud))) {
      printf("error de write; fd = %d\n", direccion->fdEsc); //DEBUG
      if (EINTR == errno) continue;
      //puede fallar por cualquiera de las razones que falla write excepto EINTR, en ese caso se repite el ciclo hasta que haga algo
      retorno = -1;
      break;
    }

    while (mensaje.longitud > 0) {
      int escrito = write(direccion->fdEsc, mensaje.contenido, mensaje.longitud);
      if (-1 == escrito) {
        if (EINTR == errno) continue;
        //puede fallar por cualquiera de las razones que falla write excepto EINTR, en ese caso se repite el ciclo hasta que haga algo
        retorno = -1;
        break;
      }
      mensaje.longitud -= escrito;
      mensaje.contenido += escrito;
    }
    break;
  }
  //TODO check errors :(
  flock(direccion->fdEsc, LOCK_UN);
  return retorno;
}

int enviarme(Mensaje mensaje) {
  return enviar(yo, mensaje);
}

int esperar(Direccion direccion) {
  if (!validoEsp(direccion)) {
    errno = EPERM;
    // falla cuando no hay permiso, se retorna -1 y se asigna EPERM a errno.
    return -1;
  }

  int foo;
  while (1) {
    if (-1 == (foo = waitpid(direccion->pid, NULL, 0))) {
      if (EINTR == errno) continue;
      // falla cuando falla waitpid y no es EINTR entonces se retorna -1 y se le pone el valor adecuado a errno
      return -1;
    }
    break;
  }

  return 0;
}
