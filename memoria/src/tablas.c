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
    pthread_mutex_init(&tabla->mutex, NULL);

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









t_pagina* buscar_pagina_en_tabla(t_tabla_nivel* raiz, uint32_t nro_pagina_logica) {
    t_list* indices = calcular_indices_por_nivel(nro_pagina_logica);
    t_tabla_nivel* actual = raiz;

    for (int i = 0; i < list_size(indices); i++) {
        int idx = (int)(intptr_t)list_get(indices, i);

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
            free((t_pagina*)entrada->siguiente_nivel);
        } else {
            liberar_tablas((t_tabla_nivel*)entrada->siguiente_nivel);
        }

        free(entrada);
    }

    list_destroy(tabla->entradas);
    pthread_mutex_destroy(&tabla->mutex);
    free(tabla);
}
