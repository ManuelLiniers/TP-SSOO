#ifndef MARCOS_H_
#define MARCOS_H_

#include "memoria.h"

typedef struct {
    int nro_marco;
    int base;
    bool libre;
//    marco_info* info_new;
//    marco_info* info_old;

    //int orden_carga;
    //t_temporal* ultimo_uso;
} t_marco;

/*typedef struct {
	t_proceso* proceso;
	int nro_pagina;
}marco_info;*/

t_marco* crear_marco(int base, bool libre, int index);
void liberar_marco(t_marco* un_marco);
t_marco* obtener_marco_por_nro_marco(int nro_marco);
void destruir_list_marcos_y_todos_los_marcos();

#endif /* MARCOS_H_ */