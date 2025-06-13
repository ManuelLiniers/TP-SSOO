#ifndef MARCOS_H_
#define MARCOS_H_

#include "memoria.h"

t_marco* crear_marco(int base, bool libre, int index);
void liberar_marco(t_marco* un_marco);
t_marco* obtener_marco_libre();
uint32_t cantidad_de_marcos_libres();
bool hay_marcos_libres();
t_marco* obtener_marco_por_nro_marco(int nro_marco);
void destruir_list_marcos_y_todos_los_marcos();
void asignar_marcos_a_tabla(t_tabla_nivel* tabla, int pid);

#endif /* MARCOS_H_ */