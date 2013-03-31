/**
 * @file actores.h
 * @author Gabriela Limonta 10-10385
 * @author John Delgado 10-10196
 *
 * @section Grupo 09
 *
 * Libreria que contiene las funciones necesarias para
 * el trabajo con actores.
 *
 * Segun Carl Hewitt en su paper "Actor Model of Computation: Scalable
 * Robust Information Systems" disponible en:
 * https://docs.google.com/file/d/0Bykigp0x1j92M0p6b0ZWWE9SS3Frb3loV3NKX2sxdw/edit
 * Un actor es un ente que puede ejercer varias funciones,
 * entre ellas se encuentran enviar mensajes a otros actores,
 * crear nuevos actores y definir que va a hacer con el siguiente mensaje
 * que reciba. Estas acciones no se llevan a cabo en un orden especifico.
 *
 * Los actores se comunican entre si enviandose mensajes, cada actor tiene
 * la direccion de otros actores a los cuales les puede enviar mensajes.
 * Tambien tienen un determinado comportamiento que ejecutan cuando reciben
 * un mensaje. Y finalmente son capaces de crear nuevos actores.
 *
 * Aca los actores se asocian a procesos, para crear nuevos actores se
 * hace fork, asi el proceso padre asociado a un actor crea un proceso hijo
 * que se encontrara asociado al nuevo actor. La comunicacion entre los
 * actores se hace mediante pipes, enviando mensajes a traves de ellos.
 * Finalmente cada actor tiene un estado, en el se encuentra contemplado
 * el comportamiento que el mismo tiene ante un mensaje recibido.
 *
 * Para trabajar con actores es preciso definir ciertos terminos.
 * Los actores se comunican enviandose mensajes; a traves de ellos
 * se pasa informacion y el actor trabaja con esa informacion de determinada
 * manera. Para poder enviarse mensajes los actores deben tener direcciones;
 * las direcciones utilizadas en esta implementacion son los file descriptors
 * de los pipes asociados a cada proceso que se asocia con un actor.
 *
 */
#ifndef ACTORES_H
#define ACTORES_H

/** Direcciones
 * A continuacion se encuentra la implementacion de las direcciones
 * se tienen ciertas funciones para trabajar con las direcciones de
 * los actores.
 */

/**
 * Apuntador a una estructura direccion de tipo opaco
 * que guarda una direccion
 */
typedef struct direccion * Direccion;

/**
 * Se encarga de liberar el espacio de memoria de una direccion dada.
 * @param direccion Indica la direccion cuyo espacio de memoria
 * se desea liberar.
 */
void liberaDireccion(Direccion direccion);

/**
 * Se encarga de retornar la direccion del actor que llama a la funcion.
 * @return direccion del actor que llama a la funcion en caso de que
 * no tenga direccion retorna NULL.
 */
Direccion miDireccion();


/** Mensajes
 * Para representar los mensajes que se envian entre actores se
 * utiliza un struct que guarda la longitud del mensaje y
 * el contenido del mensaje con su respectivo constructor
 * para crear mensajes a partir de contenidos y longitudes.
 */

/**
 * Estructura que permite guardar los mensajes que son enviados a los
 * actores. Guarda la longitud del contenido y el contenido del mensaje.
 */
typedef struct mensaje {
  ssize_t longitud; /**< longitud del mensaje  */
  void * contenido; /**< contenido del mensaje */
} Mensaje;

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
;


/** Actores
 * Para representar a un actor se tiene un struct que contiene
 * los datos del actor y la funcion que determina su comportamiento.
 * Esta funcion toma un mensaje y unos datos y retorna un actor;
 * este actor es el comportamiento que el actor define para el proximo
 * mensaje que le llegue
 */

/**
 * Estructura que permite guardar la informacion de un actor.
 * En ella se incluyen los datos del estado actual del actor y
 * la funcion que describe el comportamiento del mismo.
 */
typedef struct actor {
  struct actor (*actor)(Mensaje mensajito, void * datos); /**< funcion que describe el comportamiento del actor. */
  void * datos; /**< datos que posee este actor. */
} Actor;

/**
 * Es el constructor de actor, se encarga de construir un actor
 * del tipo actor dada una funcion que determine su comportamiento
 * y los datos que necesite el mismo.
 * @param actor es un apuntador a una funcion que toma un mensaje
 * y unos datos y retorna un actor, esta determina el comportamiento
 * del actor.
 * @return retorna el actor creado.
 */
Actor mkActor
  ( Actor (*actor)(Mensaje mensajito, void * datos)
  , void * datos
  )
;

/**
 * Se encarga de terminar a un actor, esto lo hace retornando
 * un actor cuya funcion de comportamiento sea nula.
 * @return retorna un actor nulo.
 */
Actor finActor();


/**
 * Se encarga de enviar un mensaje a un actor dada su direccion.
 * @param direccion direccion del actor al que vamos a enviar el mensaje.
 * @param mensaje mensaje que vamos a enviar al actor.
 * @return retorna un entero que sera cero cuando logre enviar el mensaje
 * y -1 si ocurre un error.
 */
int enviar(Direccion direccion, Mensaje mensaje);

/**
 * Permite que un actor pueda enviarse un mensaje a si mismo.
 * @param mensaje mensaje que se desea enviar al mismo actor
 * @return retorna 0 si fue exitoso y -1 si falla
 */
int enviarme(Mensaje mensaje);

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
Direccion crear(Actor actor);

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
Direccion crearSinEnlazar(Actor actor);

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
int esperar(Direccion direccion);

/**
 * Se encarga de deserializar una direccion contenida en
 * un mensaje. Cuando se envia una direccion a traves de
 * los datos de un mensaje esta funcion se encarga de
 * deserializarla y devolverla en una forma del tipo Direccion.
 * @param mensaje mensaje que contiene la direccion a deserializar.
 * @return Retorna la direccion contenida en el mensaje en algo
 * de tipo Direccion.
 */
Direccion deserializarDireccion(Mensaje mensaje);

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
int agregaMensaje(Mensaje * mensaje, void * fuente, int cantidad);

#endif
