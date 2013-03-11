#ifndef ACTORES_H
#define ACTORES_H

typedef struct direccion * Direccion;

extern Direccion yo;

typedef struct mensaje {
  ssize_t longitud;
  void * contenido;
} Mensaje;

typedef struct actor {
  struct actor (*actor)(Mensaje mensajito, void * datos);
  void * datos;
} Actor;

void enviar(Direccion, Mensaje);
Direccion crear(Actor);
void esperar(Direccion);

#endif
