#include "cache.h"


t_list* cache_paginas;
int entradas_cache;
char* algoritmo_cache;
int puntero_clock;

void inicializar_cache(t_log* logger, t_config* cpu_config) {
    entradas_cache = config_get_int_value(cpu_config, "ENTRADAS_CACHE");
    algoritmo_cache = strdup(config_get_string_value(cpu_config, "REEMPLAZO_CACHE"));
    cache_paginas = list_create();
    puntero_clock = 0;
}

bool buscar_en_cache(int pid, uint32_t pagina, char** contenido_resultado, uint32_t* marco_resultado, t_log* logger) {
    for (int i = 0; i < list_size(cache_paginas); i++) {
        t_entrada_cache* entrada = list_get(cache_paginas, i);
        if (entrada->pid == pid && entrada->pagina == pagina) {
            entrada->bit_uso = true;
            *contenido_resultado = strdup(entrada->contenido);
            *marco_resultado = entrada->marco;
            log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pid, pagina);
            return true;
        }
    }
    log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pid, pagina);
    return false;
}

void reemplazar_con_clock(t_entrada_cache* nueva, t_log* logger, int conexion_memoria, int pid) {
    while (1) {
        t_entrada_cache* actual = list_get(cache_paginas, puntero_clock);
        if (!actual->bit_uso) {
            if (actual->bit_modificado) {
                escribir_pagina_memoria(pid, actual->marco, actual->contenido, conexion_memoria, logger, actual->pagina, actual->marco);
                log_debug(logger, "PID: %d - Memory Update - Página: %d", actual->pid, actual->pagina);
            }
            list_replace_and_destroy_element(cache_paginas, puntero_clock, nueva, free);
            log_info(logger, "PID: %d - Cache Add - Pagina: %d", nueva->pid, nueva->pagina);
            puntero_clock = (puntero_clock + 1) % entradas_cache;
            return;
        }
        actual->bit_uso = false;
        puntero_clock = (puntero_clock + 1) % entradas_cache;
    }
}

void reemplazar_con_clock_m(t_entrada_cache* nueva, t_log* logger, int conexion_memoria, int pid) {
    int vueltas = 0;
    while (1) {
        t_entrada_cache* actual = list_get(cache_paginas, puntero_clock);

        if (!actual->bit_uso && !actual->bit_modificado) {
            list_replace_and_destroy_element(cache_paginas, puntero_clock, nueva, free);
            log_info(logger, "PID: %d - Cache Add - Pagina: %d", nueva->pid, nueva->pagina);
            puntero_clock = (puntero_clock + 1) % entradas_cache;   //p ej si las entradas son 4 (indice 0 a 3) y el ptr esta en 3 hace (3+1)%4 = 0 y reincia
            return;
        }

        actual->bit_uso = false;
        puntero_clock = (puntero_clock + 1) % entradas_cache;
        vueltas++;

        if (vueltas >= entradas_cache) {  //termina la primera vuelta (BU = 0) entonces para la segunda aplica lo mismo que hace clock normal
            reemplazar_con_clock(nueva, logger, conexion_memoria, pid);
            return;
        }
    }
}

void agregar_a_cache(int pid, uint32_t pagina, char* contenido, uint32_t marco, t_log* logger, int conexion_memoria) {
    t_entrada_cache* nueva = malloc(sizeof(t_entrada_cache));
    nueva->pid = pid;
    nueva->pagina = pagina;
    nueva->contenido = strdup(contenido);
    nueva->marco = marco;
    nueva->bit_uso = true;
    nueva->bit_modificado = false;

    if (list_size(cache_paginas) < entradas_cache) {
        list_add(cache_paginas, nueva);
        log_info(logger, "PID: %d - Cache Add - Pagina: %d", pid, pagina);
    } else {
        if (string_equals_ignore_case(algoritmo_cache, "CLOCK")) {
            reemplazar_con_clock(nueva, logger, conexion_memoria, pid);
        } else if (string_equals_ignore_case(algoritmo_cache, "CLOCK-M")) {
            reemplazar_con_clock_m(nueva, logger, conexion_memoria, pid);
        }
    }
}

void limpiar_cache_por_pid(int pid, int conexion_memoria, t_log* logger) {
    for (int i = list_size(cache_paginas) - 1; i >= 0; i--) {
        t_entrada_cache* entrada = list_get(cache_paginas, i);
        if (entrada->pid == pid) {
            if (entrada->bit_modificado) {
                uint32_t direccion_fisica = entrada->marco * TAMANIO_PAGINA;
                log_debug(logger, "Escribiendo página modificada. PID: %d, Pag: %d, Marco: %d, Dir física: %d",
                entrada->pid, entrada->pagina, entrada->marco, direccion_fisica);
                escribir_pagina_memoria(pid, direccion_fisica, entrada->contenido, conexion_memoria, logger, entrada->pagina, entrada->marco);
            }
            list_remove_and_destroy_element(cache_paginas, i, free);
        }
    }
}

void escribir_pagina_memoria(int pid, uint32_t direccion_fisica, void* valor, int conexion_memoria, t_log* logger, uint32_t pagina, uint32_t marco) {
    t_paquete* paquete = crear_paquete(ESCRIBIR_MEMORIA);
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
    agregar_a_paquete(paquete, &TAMANIO_PAGINA, sizeof(int));
    agregar_a_paquete(paquete, valor, TAMANIO_PAGINA);
    enviar_paquete(paquete, conexion_memoria);
    eliminar_paquete(paquete);

    int respuesta;
    recv(conexion_memoria, &respuesta, sizeof(int), 0);
    if(respuesta != OK){
        log_error(logger, "No se escribio correctamente");
    }

    log_info(logger, "PID: %d - Memory Update - Página: %d - Frame: %d", pid, pagina, marco);
}
