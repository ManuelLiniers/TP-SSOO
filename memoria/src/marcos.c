#include "../include/marcos.h"

t_marco* crear_marco(int base, bool libre, int index){
    t_marco* nuevo_marco = malloc(sizeof(t_marco));

	nuevo_marco->nro_marco = index;
	nuevo_marco->base = base;
	nuevo_marco->libre = libre;
    
    nuevo_marco->info = malloc(sizeof(marco_info));
    if (!nuevo_marco->info) {
        free(nuevo_marco);
        return NULL;
    }

    // Valores por defecto para info
    nuevo_marco->info->pid_proceso = -1;
    nuevo_marco->info->nro_pagina = -1;

	return nuevo_marco;
}

void liberar_marco(t_marco* un_marco){
    if (!un_marco) return;

    pthread_mutex_lock(&mutex_lst_marco);
    un_marco->libre = true;
    
//  Resetear la información 
    if (un_marco->info) {
        un_marco->info->pid_proceso = -1;
        un_marco->info->nro_pagina = -1;
    }
    pthread_mutex_unlock(&mutex_lst_marco);
}

t_marco* obtener_marco_libre(){
    t_marco* marco_libre = NULL;

    pthread_mutex_lock(&mutex_lst_marco);
    for (int i = 0; i < list_size(lst_marcos); i++) {
        t_marco* marco = list_get(lst_marcos, i);
        if (marco->libre) {
            marco_libre = marco;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_lst_marco);
    
    return marco_libre;
}

uint32_t cantidad_de_marcos_libres(){
    uint32_t contador = 0;
    
    pthread_mutex_lock(&mutex_lst_marco);
    for (int i = 0; i < list_size(lst_marcos); i++) {
        t_marco* marco = list_get(lst_marcos, i);
        if (marco->libre) {
            contador++;
        }
    }
    pthread_mutex_unlock(&mutex_lst_marco);
    
    return contador;
}

bool hay_marcos_libres() {
    bool resultado = false;
    
    pthread_mutex_lock(&mutex_lst_marco);
    for (int i = 0; i < list_size(lst_marcos); i++) {
        t_marco* marco = list_get(lst_marcos, i);
        if (marco->libre) {
            resultado = true;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_lst_marco);
    
    return resultado;
}

t_marco* obtener_marco_por_nro_marco(int nro_marco){
    t_marco* marco = NULL;
    
    pthread_mutex_lock(&mutex_lst_marco);
    if (nro_marco >= 0 && nro_marco < list_size(lst_marcos)) {
        marco = list_get(lst_marcos, nro_marco);
    }
    pthread_mutex_unlock(&mutex_lst_marco);
    
    return marco;
}

void destruir_todos_los_marcos() {
    pthread_mutex_lock(&mutex_lst_marco);
    if (lst_marcos) {
        for (int i = 0; i < list_size(lst_marcos); i++) {
            t_marco* marco = list_get(lst_marcos, i);
            if (marco->info) {
                free(marco->info);
            }
            free(marco);
        }
        list_destroy(lst_marcos);
        lst_marcos = NULL;
    }
    pthread_mutex_unlock(&mutex_lst_marco);
}

void asignar_marcos_a_tabla(t_tabla_nivel* tabla, int pid) {
    for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
        t_entrada_tabla* entrada = list_get(tabla->entradas, i);

        if (entrada->es_ultimo_nivel) {
            t_pagina* pagina = (t_pagina*) entrada->siguiente_nivel;
            if (pagina->marco_asignado == -1) {
                t_marco* marco = obtener_marco_libre();
                if (marco != NULL) {
                    marco->libre = false;

                    marco->info->pid_proceso = pid;
                    marco->info->nro_pagina = pagina->nro_pagina;
                    pagina->marco_asignado = marco->nro_marco;
                } else {
                    // No hay más marcos libres (No deberia pasar)
                    log_error(memoria_logger, "No hay marcos libres disponibles para la página %d\n", pagina->nro_pagina);
                }
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) entrada->siguiente_nivel;
            asignar_marcos_a_tabla(subtabla, pid);  // llamada recursiva
        }
    }
}
