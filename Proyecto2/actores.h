#ifndef ACTORES_H
#define ACTORES_H

// DEBUG: borrar esto!
struct direccion {
  int fdLec;
  int fdEsc;
  pid_t pid;
};

typedef struct direccion * Direccion;

void liberaDireccion(Direccion direccion);
Direccion miDireccion();


typedef struct mensaje {
  ssize_t longitud;
  void * contenido;
} Mensaje;

Mensaje mkMensaje
  ( ssize_t longitud
  , void * contenido
  )
;



typedef struct actor {
  struct actor (*actor)(Mensaje mensajito, void * datos);
  void * datos;
} Actor;

Actor mkActor
  ( Actor (*actor)(Mensaje mensajito, void * datos)
  , void * datos
  )
;

Actor finActor();



int enviar(Direccion, Mensaje);
int enviarme(Mensaje);
Direccion crear(Actor);
Direccion crearSinEnlazar(Actor);
int esperar(Direccion);
Direccion deserializarDireccion(Mensaje);

int agregaMensaje(Mensaje * mensaje, void * fuente, int cantidad);

#endif
