#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "actores.h"

struct direccion {
  int fdLec;
  int fdEsc;
  pid_t pid;
};

Direccion yo;

Direccion crearDireccion(int fdLec, int fdEsc, pid_t pid) {
  Direccion direccion = malloc(sizeof(struct direccion));
  if (NULL == direccion) {
    perror("malloc");
    exit(EX_OSERR);
  }
  direccion->fdLec = fdLec;
  direccion->fdEsc = fdEsc;
  direccion->pid   = pid  ;
  return direccion;
}

int validoEsc(Direccion direccion) {
  return -1 != direccion->fdEsc;
}

int validoLec(Direccion direccion) {
  return -1 != direccion->fdLec;
}

int validoEsp(Direccion direccion) {
  return -1 != direccion->pid;
}

void liberaDireccion(Direccion direccion) {
  if (validoEsc(direccion)) close(direccion->fdEsc);
  if (validoLec(direccion)) close(direccion->fdLec);
}

void agregaMensaje(Mensaje * mensaje, void * fuente, int cantidad) {
  if (NULL == (mensaje->contenido = realloc(mensaje->contenido, mensaje->longitud + cantidad))) {
    perror("realloc");
    exit(EX_OSERR);
  }
  memcpy(mensaje->contenido + mensaje->longitud, fuente, cantidad);
  mensaje->longitud += cantidad;
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



Direccion crear(Actor actor) {
  enum indicePipes { P_ESCRITURA = 1, P_LECTURA = 0 };
  int pipeAbajo[2];
  pipe(pipeAbajo);

  pid_t pid;
  switch (pid = fork()) {
    case -1: {
      perror("fork");
      exit(EX_OSERR);
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

      if (papa) {
        Mensaje mensaje = serializarDireccion(papa);
        enviar(yo, mensaje);
        free(mensaje.contenido);
      }

      correActor(actor);
      exit(EX_OK);
    }

    default: {
      close(pipeAbajo[P_LECTURA]);
      Direccion nuevoActor = crearDireccion(-1, pipeAbajo[P_ESCRITURA], pid);
      return nuevoActor;
    }
  }
}

void enviar(Direccion direccion, Mensaje mensaje) {
  if (!validoEsc(direccion)) {
    errno = EPERM;
    perror("enviar");
    exit(EX_NOPERM);
  }

  //TODO check errors :(
  flock(direccion->fdEsc, LOCK_EX);
  //Seccion critica :)
  {
    if (-1 == write(direccion->fdEsc, &mensaje.longitud, sizeof(mensaje.longitud))) {
      perror("write");
      exit(EX_IOERR);
    }

    while (mensaje.longitud > 0) {
      int escrito = write(direccion->fdEsc, mensaje.contenido, mensaje.longitud);
      if (-1 == escrito) {
        perror("write");
        exit(EX_IOERR);
      }
      mensaje.longitud -= escrito;
      mensaje.contenido += escrito;
    }
  }
  //TODO check errors :(
  flock(direccion->fdEsc, LOCK_UN);
}

void esperar(Direccion direccion) {
  if (!validoEsp(direccion)) {
    errno = EPERM;
    perror("esperar");
    exit(EX_NOPERM);
  }

  if (-1 == waitpid(direccion->pid, NULL, 0)) {
    perror("waitpid");
    exit(EX_OSERR);
  }
}
