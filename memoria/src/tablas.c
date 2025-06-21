#include "../include/tablas.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

// Crea una tabla de páginas multinivel recursivamente
t_tabla_nivel* crear_tabla_multinivel(int nivel_actual, int nivel_final) {
    t_tabla_nivel* tabla = malloc(sizeof(t_tabla_nivel));
    tabla->nivel = nivel_actual;
    tabla->entradas = list_create();

    for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
        t_entrada_tabla* entrada = malloc(sizeof(t_entrada_tabla));
        entrada->numero_entrada = i;

        if (nivel_actual == nivel_final) {
            t_pagina* pagina = malloc(sizeof(t_pagina));
            pagina->nro_pagina = i;
            pagina->marco_asignado = -1;
            entrada->siguiente_nivel = pagina;
            entrada->es_ultimo_nivel = true;
        } else {
            entrada->siguiente_nivel = crear_tabla_multinivel(nivel_actual + 1, nivel_final);
            entrada->es_ultimo_nivel = false;
        }

        list_add(tabla->entradas, entrada);
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

    for (int i = 0; i < list_size(indices); i++) {
        int idx = (int)(intptr_t)list_get(indices, i);

        usleep(RETARDO_MEMORIA);
        metricas->accesos_tablas_paginas++;

        if (i == CANTIDAD_NIVELES - 1) {
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
        liberar_marco(pagina->marco_asignado);
    }
    free(pagina);
}