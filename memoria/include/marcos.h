#ifndef MARCOS_H_
#define MARCOS_H_

#include "memoria.h"

void liberar_marco(uint32_t un_marco);
int obtener_marco_libre();
uint32_t cantidad_de_marcos_libres();
bool hay_marcos_libres();
void destruir_todos_los_marcos();
void asignar_marcos_a_tabla(t_tabla_nivel* tabla, int* paginas_restantes);

#endif /* MARCOS_H_ */