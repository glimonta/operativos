enum parametrosHijo
  { PH_INICIO = 1
  , PH_FIN
  , PH_NIVEL
  , PH_ID
  , PH_ARCHIVODEENTEROSDESORDENADOS
  , PH_NUMNIVELES
  }
;

struct configuracionHijo {
  int inicio;
  int fin;
  int nivel;
  int id;
  char const * archivoDesordenado;
  int niveles;
};
