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

t_pagina* buscar_pagina(t_tabla_nivel* raiz, int* indices, t_metricas_proceso* metricas) {
    t_tabla_nivel* actual = raiz;

    for (int i = 0; i < CANTIDAD_NIVELES; i++) {
        int idx = indices[i];
        
        usleep(RETARDO_MEMORIA);
        metricas->accesos_tablas_paginas++;

        if (idx >= list_size(actual->entradas)) {
            log_error(memoria_logger, "Error: Índice %d fuera de rango en nivel %d (tamaño: %d)", idx, i, list_size(actual->entradas));
            free(indices);
            return NULL;
        }

        if (i == CANTIDAD_NIVELES - 1) {
            t_pagina* pag = (t_pagina*) list_get(actual->entradas, idx);
            free(indices);
            return pag;
        } else {
            actual = (t_tabla_nivel*) list_get(actual->entradas, idx);
            if (!actual) {
                log_error(memoria_logger, "Error: Subtabla nula al intentar acceder al nivel %d", i);
                free(indices);
                return NULL;
            }
        }
    }

    free(indices);
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