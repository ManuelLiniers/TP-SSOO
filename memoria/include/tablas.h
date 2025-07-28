#ifndef TABLAS_H_
#define TABLAS_H_

#include "memoria.h"

t_tabla_nivel* crear_tabla_multinivel(int nivel_actual, int* paginas_restantes, int* contador_paginas);
t_pagina* buscar_pagina(t_tabla_nivel* raiz, int* indices, t_metricas_proceso* metricas);
void liberar_tablas(void* puntero);
void liberar_pagina_y_marcos(void* puntero);

#endif /* TABLAS_H_ */