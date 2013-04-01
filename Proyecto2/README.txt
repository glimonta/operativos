Proyecto 2 ------------ Sistemas de Operacion
--- Gabriela Limonta 10-10385
--- John Delgado 10-10196

  Nos basamos en Carl Hewitt usando la informacion de su paper "Actor
  Model of Computation: Scalable Robust Information Systems" disponible en:
  https://docs.google.com/file/d/0Bykigp0x1j92M0p6b0ZWWE9SS3Frb3loV3NKX2sxdw/edit

  Un actor es un ente que puede ejercer varias funciones (un comportamiento
  determinado o llamar a otro actor a escena y darle una funcion),los
  actores se comunican entre si enviandose mensajes, cada actor tiene
  acceso a la direccion de otros actores mediante una libreta (lista de
  nombres asociados a las direcciones) a los cuales les puede enviar
  mensajes.

  En este proyecto se utilizo el modelo de actores, para generar
  un nivel de abstraccion que facilite la programacion del shell
  tomando como actores propiamente a los directorios que se
  pidieron

  Para el envio de mensajes se implemento una serializacion de mensajes
  ya que entre actores no se pueden enviar trozos de informacion asi
  que para poder hacer la comunicacion entre actores se genero una
  estructura que permite que el envio y recepcion de informacion por los
  pipes sea mas sencillo. Debido a la existencia y uso de una funcion que
  serialice se debe deserealizar la instruccion que se recibe por el pipe
  para poder ser interpretada por el actor.

  Inicialmente se crean tres actores, el frontController (manejador
  principal del main, que se encarga de enviar instrucciones a la raiz),
  la raiz (actor que se encarga de comunicarse con todos los actores que
  representan a los subdirectorios de donde se ejecuta el shell) y
  finalmente el actor impresora, que se encarga de imprimir errores y
  mensajes que son enviados desde los otros actores para mantener un
  orden de impresion.

  Se genero una funcion que despachara las instrucciones como funciones
  puesto a que con el sistema de comunicacion mediante serializacion de
  mensajes era mucho mas facil al momento de decodifica y ver el codigo
  de instruccion saber cual es la funcion que se debe aplicar y cuales
  parametros tomar dado un mensaje.

  Se tomo la decision de separar los errores posibles en dos tipos,
  recuperables y no recuperables, esto se debe a que si el error es
  recuperable el shell debe continuar indicando que hubo un error, para
  el caso de los no recuperables, el shell debe abortar eliminando a
  todos los procesos asociados a el.

  Nuestro shell tiene todos los comandos que se pidio implementar. No se
  trabajo la parte de puntos extra por redireccionamiento.

  El comportamiento de los comandos es el siguiente:

     quit : Mata a todos los procesos asociados al shell terminandolo.

     cat <ruta1> : Se desciende hasta el actor encargado del directorio
                   para leer el archivo, este le manda su contenido a la
                   impresora y este lo imprime por pantalla.

     ls <ruta1> : Analogo al anterior pero se trabaja con el stat.

     find <ruta1> : Se realiza de forma recursiva para todos los
                    subdirectorios asociados a la ruta1 y se imprimen sus
                    nombres, junto con el de los archivos que contengan.

     rm <ruta1> : Se desciende al actor correspondiente y luego se hace
                  unlink de ese archivo.

     rmdir <ruta1> : Una vez en el actor encargado del directorio en ruta1
                     se procede a eliminar el directorio y a matar al actor
                     para luego vaciar su informacion en la libreta.

     mkdir <ruta1> : Se genera un directorio nuevo y con el un actor, el
                     cual es insertado en la libreta de direcciones

     mv <ruta1> <ruta2>: Se baja hasta la ruta1 se lee la informacion del
                         fichero para luego pasarla a la raiz, se baja
                         hasta la ruta2 y se genera el archivo solicitado,
                         en caso de que este archivo ya exista se pega la
                         informacion en forma de apendice; finalmente
                         se elimina el fichero asociado a la ruta1.

     cp <ruta1> <ruta2>: Se desciende hasta la ruta1, se lee la informacion
                         y se la manda a la raiz, luego la raiz se encarga
                         de obtener al actor encargado de la ruta2
                         descendiendo hasta el y pasandole la informacion
                         como un mensaje para que este la escriba como un
                         archivo.
