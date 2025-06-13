#include "../include/tablas.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

// Crea una tabla de páginas multinivel recursivamente
// nivel_actual comienza en 1, nivel_final es CANTIDAD_NIVELES

t_tabla_nivel* crear_tabla_multinivel(int nivel_actual, int nivel_final) {
    t_tabla_nivel* tabla = malloc(sizeof(t_tabla_nivel));
    tabla->nivel = nivel_actual;
    tabla->entradas = list_create();

    for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
        t_entrada_tabla* entrada = malloc(sizeof(t_entrada_tabla));
        entrada->numero_entrada = i;

        if (nivel_actual == nivel_final) {
            // Ultimo nivel: apunta a pagina
            t_pagina* pagina = malloc(sizeof(t_pagina));
            pagina->nro_pagina = i; // relativo dentro de este nivel
            pagina->marco_asignado = -1; // sin asignar

            entrada->siguiente_nivel = pagina;
            entrada->es_ultimo_nivel = true;
        } else {
            // Intermedio: apunta a otra tabla
            entrada->siguiente_nivel = crear_tabla_multinivel(nivel_actual + 1, nivel_final);
            entrada->es_ultimo_nivel = false;
        }

        list_add(tabla->entradas, entrada);
    }

    return tabla;
}

// Calcula los índices por nivel a partir del número de página lógica
// Devuelve una lista de enteros con tantos elementos como niveles

t_list* calcular_indices_por_nivel(uint32_t nro_pagina_logica) {
    t_list* indices = list_create();

    int bits_por_nivel = log2(ENTRADAS_POR_TABLA);

    for (int i = 0; i < CANTIDAD_NIVELES; i++) {
        int shift = bits_por_nivel * (CANTIDAD_NIVELES - 1 - i);
        uint32_t index = (nro_pagina_logica >> shift) & ((1 << bits_por_nivel) - 1);
        list_add(indices, (void*)(intptr_t)index);
    }

    return indices;
}

// Navega recursivamente desde la raiz hasta obtener la pagina correspondiente

// raiz: tabla de nivel 1
// nro_pagina_logica: index completo de la direccion

t_pagina* buscar_pagina_en_tabla(t_tabla_nivel* raiz, uint32_t nro_pagina_logica, t_metricas_proceso* metricas) {
    t_list* indices = calcular_indices_por_nivel(nro_pagina_logica);
    t_tabla_nivel* actual = raiz;

    for (int i = 0; i < list_size(indices); i++) {
        int idx = (int)(intptr_t)list_get(indices, i);

        usleep(RETARDO_MEMORIA);
        metricas->accesos_tablas_paginas++; // METRICAS

        if (i == CANTIDAD_NIVELES - 1) {
            // Último nivel → obtenemos página
            t_entrada_tabla* entrada = list_get(actual->entradas, idx);
            t_pagina* pag = (t_pagina*) entrada->siguiente_nivel;
            list_destroy(indices);
            return pag;
        } else {
            t_entrada_tabla* entrada = list_get(actual->entradas, idx);
            actual = (t_tabla_nivel*) entrada->siguiente_nivel;
        }
    }

    list_destroy(indices);
    return NULL;
}

// Libera todas las estructuras de tablas y páginas recursivamente
void liberar_tablas(t_tabla_nivel* tabla) {
    for (int i = 0; i < list_size(tabla->entradas); i++) {
        t_entrada_tabla* entrada = list_get(tabla->entradas, i);

        if (entrada->es_ultimo_nivel) {
            liberar_pagina_y_marcos((t_pagina*)entrada->siguiente_nivel);
        } else {
            liberar_tablas((t_tabla_nivel*)entrada->siguiente_nivel);
        }

        free(entrada);
    }

    list_destroy(tabla->entradas);
    free(tabla);
}

void liberar_pagina_y_marcos(t_pagina* pagina){
    if(pagina->marco_asignado != -1){
        t_marco* marco = obtener_marco_por_nro_marco(pagina->marco_asignado);
        marco->libre = true;
        marco->info->pid_proceso = -1;
    }
    free(pagina);
}

// No se si va esto porque la MMU de cpu es la que se encarga de la traduccion, 
// ademas de que no se asignan los marcos libres porque los procesos se cargan
// enteros en memoria (pq no hay memoria virtual)
/*
// Traduce una dirección lógica a dirección física para un proceso dado
// Si la página no tiene marco asignado, lo asigna si hay uno libre
uint32_t traducir_direccion_logica(t_proceso* proceso, uint32_t direccion_logica) {
    uint32_t pagina = direccion_logica / TAM_PAGINA;
    uint32_t desplazamiento = direccion_logica % TAM_PAGINA;

    pthread_mutex_lock(&proceso->mutex_TP);
    t_pagina* pagina_virtual = buscar_pagina_en_tabla((t_tabla_nivel*)proceso->tabla_paginas_raiz, pagina);

    if (pagina_virtual == NULL) {
        log_error(memoria_logger, "PID: %d - Error: página virtual no encontrada", proceso->pid);
        pthread_mutex_unlock(&proceso->mutex_TP);
        return -1;
    }

    // Si no tiene marco asignado
    if (pagina_virtual->marco_asignado == -1) {
        t_marco* marco_libre = obtener_marco_libre();
        if (marco_libre == NULL) {
            log_error(memoria_logger, "PID: %d - No hay marcos libres para asignar página %d", proceso->pid, pagina);
            pthread_mutex_unlock(&proceso->mutex_TP);
            return -1;
        }

        marco_libre->libre = false;
        marco_libre->info->pid_proceso = proceso->pid;
        marco_libre->info->nro_pagina = pagina;

        pagina_virtual->marco_asignado = marco_libre->nro_marco;
        list_add(proceso->marcos_asignados, marco_libre);

        proceso->metricas.subidas_memoria++;
    }

    proceso->metricas.accesos_tablas_paginas++;
    pthread_mutex_unlock(&proceso->mutex_TP);

    t_marco* marco = obtener_marco_por_nro_marco(pagina_virtual->marco_asignado);
    if (!marco) {
        log_error(memoria_logger, "PID: %d - Error al obtener marco %d", proceso->pid, pagina_virtual->marco_asignado);
        return -1;
    }

    return marco->base + desplazamiento;
}
*/