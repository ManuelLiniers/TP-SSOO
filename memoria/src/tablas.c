#include "../include/tablas.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

// Crea una tabla de páginas multinivel recursivamente
t_tabla_nivel* crear_tabla_multinivel(int nivel_actual, int* paginas_restantes, int* contador_paginas) {
    if (*paginas_restantes == 0) return NULL;

    t_tabla_nivel* tabla = malloc(sizeof(t_tabla_nivel));
    tabla->nivel = nivel_actual;
    tabla->entradas = list_create();

    for (int i = 0; i < ENTRADAS_POR_TABLA && *paginas_restantes > 0; i++){
        if (nivel_actual == CANTIDAD_NIVELES) {

            t_pagina* pag = malloc(sizeof(t_pagina));
            pag->nro_pagina = (*contador_paginas)++;
            pag->marco_asignado = -1;

            list_add(tabla->entradas, pag);
            (*paginas_restantes)--;

        } else {
            t_tabla_nivel* sub = crear_tabla_multinivel(nivel_actual + 1, paginas_restantes, contador_paginas);

            list_add(tabla->entradas, sub);
        }
    }
    return tabla;
}


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

t_pagina* buscar_pagina_en_tabla(t_tabla_nivel* raiz, uint32_t nro_pagina_logica, t_metricas_proceso* metricas) {
    t_list* indices = calcular_indices_por_nivel(nro_pagina_logica);
    t_tabla_nivel* actual = raiz;

    int niveles = list_size(indices);

    for (int i = 0; i < niveles; i++) {
        int idx = (int)(intptr_t)list_get(indices, i);

        usleep(RETARDO_MEMORIA);
        metricas->accesos_tablas_paginas++;

        if (idx >= list_size(actual->entradas)) {
            log_error(memoria_logger, "Error: Índice %d fuera de rango en nivel %d (tamaño: %d)",
              idx, i, list_size(actual->entradas));
            list_destroy(indices);
            return NULL;
        }

        if (i == CANTIDAD_NIVELES - 1) {
            t_pagina* pag = (t_pagina*) list_get(actual->entradas, idx);
            list_destroy(indices);
            return pag;
        } else {
            actual = (t_tabla_nivel*) list_get(actual->entradas, idx);
            if (!actual) {
                log_error(memoria_logger, "Error: Subtabla nula al intentar acceder al nivel %d", i);
                list_destroy(indices);
                return NULL;
            }
        }
    }

    list_destroy(indices);
    return NULL;
}

void liberar_tablas(void* puntero) {
    t_tabla_nivel* tabla = (t_tabla_nivel*) puntero;
    if (tabla->nivel == CANTIDAD_NIVELES) {
        /* Hojas: liberar cada t_pagina */
        list_destroy_and_destroy_elements(tabla->entradas, liberar_pagina_y_marcos);
    } else {
        /* Intermedios: destruir sub‑tablas recursivamente */
        list_iterate(tabla->entradas, liberar_tablas);
        list_destroy(tabla->entradas);
    }
    free(tabla);
}

void liberar_pagina_y_marcos(void* puntero){
    t_pagina* pagina = (t_pagina*) puntero;
    if(pagina->marco_asignado != -1){
        liberar_marco(pagina->marco_asignado);
    }
    free(pagina);
}