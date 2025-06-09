#ifndef TABLAS_H_
#define TABLAS_H_

#include "memoria.h"

t_tabla_nivel* crear_tabla_multinivel(int nivel_actual, int nivel_final);
t_list* calcular_indices_por_nivel(uint32_t nro_pagina_logica);
t_pagina* buscar_pagina_en_tabla(t_tabla_nivel* raiz, uint32_t nro_pagina_logica);
void liberar_tablas(t_tabla_nivel* tabla);
uint32_t traducir_direccion_logica(t_proceso* proceso, uint32_t direccion_logica);

#endif /* TABLAS_H_ */