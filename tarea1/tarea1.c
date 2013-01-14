/**
* @file
* @author Gabriela Limonta 10-10385 y John Delgado 10-10196
*
* @section Descripción
*
* Programa que dada una secuencia de arboles binarios, imprime
* un recorrido por nivel de cada arbol.
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef char * Camino;

/**
 * Estructura en la cual se guardan los arboles binarios.
 */
typedef struct NodoT {
  struct NodoT * izq; /**< Subarbol izquierdo */
  struct NodoT * der; /**< Subarbol derecho */
  int valor; /**< Valor entero del nodo */
} Nodo;

/**
 * Tipo enumerado que indica cual es el resultado
 * obtenido de insertar un nodo a un arbol.
 */
enum resultaditosInsertar
  { exitoInsertar /**< La operacion de insertar fue exitosa */
  , errorInsertar /**< Hubo un error al insertar en el arbol */
  , repetidoInsertar /**< Error cuando el arbol sobreescribe un nodo ya existente */
  }
;

/**
 * Se encarga de construir el arbol binario.
 * @param valor Valor que va en el nodo que estamos insertando.
 * @param caminito Camino que debe seguirse para llegar a la posicion
 * correcta del nodo.
 * @param john Apuntador a la direccion de un arbol.
 * @return Retorna el estado de la operacion. exitoInsertar si
 * logra agregar el nodo correctamente, errorInsertar si ocurre un
 * error al añadirlo e repetidoInsertar si consigue que esta
 * sobreescribiendo un nodo ya existente.
 */
enum resultaditosInsertar insertar(int valor, Camino caminito, Nodo * * john){
  // Si el camino o el apuntador a la direccion del arbol es nulo
  // no podemos insertar.
  if (NULL == john || NULL == caminito) return errorInsertar;
  // Si el arbol es vacio, creamos uno nuevo.
  if (NULL == *john) {
    *john = calloc(1, sizeof(Nodo));
    (*john)->valor = -1;
  }

  switch (*caminito) {
    // Caso en el que el camino sea vacio (hay que insertar en la posicion
    // donde estamos
    case '\0': return
      ((*john)->valor == -1)
      // Si no hemos modificado, sustituimos el valor y retornamos un exito.
      ? ((*john)->valor = valor, exitoInsertar)
      // Si estamos reescribiendo un nodo, sustituimos por -2 y retornamos
      // que hubo una falla.
      : ((*john)->valor = -2, repetidoInsertar)
    ;

    // En caso de ser una 'I' o una 'D' llamamos recursivamente a la
    // funcion con el camino restante y el subarbol izquierdo o
    // derecho respectivamente.
    case 'I': return insertar(valor, caminito + 1, &((*john)->izq));
    case 'D': return insertar(valor, caminito + 1, &((*john)->der));

    default: return errorInsertar; // codigo no alcanzable.
  }
}


/**
 * Se encarga de abrir el archivo de texto, llamar a una funcion que
 * trabaja con el y cerrarlo luego de terminar.
 * @param nombrecito Nombre del archivo con el que se va a trabajar.
 * @param funcioncita Funcion que va a trabajar con el archivo indicado.
 * @return Retorna NULL cuando ocurre un error.
 */
void * apertura(const char * nombrecito, void * (*funcioncita)(FILE *)) {
  // Abrimos el archivo para lectura.
  FILE * archivito = fopen(nombrecito, "r");
  // Si el archivo retorna NULL es porque hubo un error en fopen
  // se detiene la ejecucion del programa.
  if (NULL == archivito) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }

  // Llamamos a la funcion que va a trabajar con el archivo.
  void * retornito = funcioncita(archivito);

  // Cerramos el archivo.
  fclose(archivito);
  // Retornamos lo que devuelve la ejecucion de la función.
  return retornito;
}


/**
 * Se encarga de revisar un arbol binario para verificar que
 * el mismo sea valido.
 * @param noditoAct Arbol a revisar.
 * @return retorna 1 si el arbol es valido y cero en caso contrario.
 */
int revisarCompleto(Nodo * noditoAct) {
  // Retorna true si llegamos al final o si el valor de
  // todos los nodos es valido (mayor o igual a cero) y
  // si su hijo izquierdo y derecho son arboles validos
  // tambien.
  return
    !noditoAct || (
      noditoAct->valor >= 0
      && revisarCompleto(noditoAct->izq)
      && revisarCompleto(noditoAct->der)
    )
  ;
}

/**
 * Se encarga de eliminar un arbol binario cuando ya no estamos
 * utilizandolo para liberar ese espacio en la memoria.
 * @param noditoAct Arbol a eliminar.
 */
void eliminarCompleto(Nodo * noditoAct) {
  // Si llego al final retorna.
  if (!noditoAct) return;
  // Si tiene hijo izquierdo, lo elimina.
  if (noditoAct->izq) eliminarCompleto(noditoAct->izq);
  // Si tiene hijo derecho, lo elimina.
  if (noditoAct->der) eliminarCompleto(noditoAct->der);
  // Libera el espacio de memoria ocupado por el nodo actual.
  free(noditoAct);
}

/**
 * Se encarga de imprimir completo los valores de un
 * nivel del arbol.
 * @param noditoAct Arbol binario.
 * @param nivel Nivel del arbol que queremos imprimir.
 * @return Retorna 1 si logra imprimir en caso contrario 
 * retorna 0.
 */
int imprimirNivel(Nodo * noditoAct, int nivel) {
  // Si el arbol es vacio no imprime.
  if (NULL == noditoAct) return 0;

  // Si llegamos al nivel que queremos, imprimimos en pantalla.
  if (0 == nivel) {
    printf("%d ",noditoAct->valor);
    return 1;
  }

  // Hacemos la llamada recursiva para los subarboles
  // izquierdo y derecho si no llegamos al nivel que
  // queriamos imprimir.
  return
      imprimirNivel(noditoAct->izq, nivel - 1)
    | imprimirNivel(noditoAct->der, nivel - 1)
  ;
}


Nodo * arbolitoJG = NULL;

/**
 * Se encarga de leer el archivo e ir construyendo el arbol
 * binario con el que se va a trabajar.
 * @param archivito Archivo de entrada de donde leeremos la
 * informacion.
 */
void * gabbi(FILE * archivito) {
  // El ciclo de lectura va a continuar mientras lea un formato correcto
  // (que inicie en "("), no haya algun error al leer el archivo o sea
  // fin del archivo.
  while (fscanf(archivito, " ("), !ferror(archivito) && !feof(archivito)) {
    {
      // Leemos el siguiente caracter del archivo.
      char c = fgetc(archivito);
      // Si es un ")" entonces es el final de un arbol.
      if (')' == c) {
        // En caso de no ser un arbol completo, imprimimos incompleto.
        if (0 == revisarCompleto(arbolitoJG)) printf("incompleto\n");
        // Si es un arbol valido, lo imprimimos en pantalla.
        else {
          int nivel;
          // Imprimimos todos los niveles del arbol desde el 0 hasta el 
          // ultimo nivel.
          for (nivel = 0; imprimirNivel(arbolitoJG, nivel); ++nivel);
          puts("");
        }

        //Eliminamos el arbol con el que ya trabajamos.
        eliminarCompleto(arbolitoJG);
        // Reinicializamos a NULL el apuntador al arbol.
        arbolitoJG = NULL;
        continue;
      }
      // Si no es el fin del arbol devolvemos el caracter que leimos
      // y procedemos a leer el valor y camino del nodo.
      ungetc(c, archivito);
    }

    int valorcito;

    // Leemos el valor que debe tener el nodo. En caso de que haya
    // un error leyendo, retornamos null.
    if (1 != fscanf(archivito, "%d,", &valorcito)) {
      fprintf(stderr, "El formato de entrada es incorrecto\n");
      return NULL;
    }

    Camino caminito;
    {
      // Obtenemos el proximo caracter del archivo.
      char c = fgetc(archivito);
      // Si es un ")" es porque el camino es vacio (es la raiz)
      if (')' == c) {
        caminito = "";
      } else {
        //Sino devolvemos el caracter y procedemos a leer el camino.
        ungetc(c, archivito);
        // Leemos la cadena de Is y Ds que debe seguir el nodo para
        // posicionarse en el lugar del arbol que va y lo guardamos
        // en caminito. En caso de que haya un error leyendo esto
        // retornamos NULL.
        if (1 != fscanf(archivito, "%m[ID]) ", &caminito)) {
          fprintf(stderr, "El formato de entrada es incorrecto\n");
          return NULL;
        }
      }
    }

    // Procedemos a insertar el nodo en el arbol.
    insertar(valorcito, caminito, &arbolitoJG);
    if (*caminito) free(caminito);
  }
  return NULL;
}

/**
 * Main del programa.
 * @param argc Cantidad de elementos del arreglo de entrada
 * @param argv Arreglo que contiene los argumentos con los
 * que fue llamado el programa.
 */
int main(int argc, char * argv[]) {
  // Si el numero de argumentos con el que se invoca al programa
  // es incorrecto se imprime un mensaje de error indicandole al
  // usuario como invocar al programa.
  if (2 != argc) {
    fprintf(stderr, "Numero de argumentos incorrecto, invoque el programa con: %s archivo_de_entrada\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Abrimos el archivo y le pasamos a la funcion
  // gabbi para que trabaje con el mismo.
  apertura(argv[1], gabbi);

  return 0;
}
