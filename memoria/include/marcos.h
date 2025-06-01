#ifndef MARCOS_H_
#define MARCOS_H_

#include "memoria.h"

t_marco* crear_marco(int base, bool libre, int index);
void liberar_marco(t_marco* un_marco);
t_marco* obtener_marco_por_nro_marco(int nro_marco);
void destruir_list_marcos_y_todos_los_marcos();

#endif /* MARCOS_H_ */