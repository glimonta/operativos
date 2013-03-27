#ifndef ACTORES_H
#define ACTORES_H

typedef struct direccion * Direccion;

extern Direccion yo;

void liberaDireccion(Direccion direccion);


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
Direccion crear(Actor);
int esperar(Direccion);
Direccion deserializarDireccion(Mensaje);

int agregaMensaje(Mensaje * mensaje, void * fuente, int cantidad);

#endif
