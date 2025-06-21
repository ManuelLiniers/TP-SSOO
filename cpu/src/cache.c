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
        if (entrada->pid == pid && entrada->pagina == pagina) { //comparo numeros de pagina y proceso
            entrada->bit_uso = true; //pone el bit de uso en 1 xq la entrada ahora esta siendo usada
            *contenido_resultado = strdup(entrada->contenido);
            *marco_resultado = entrada->marco; // encuentra marco correspondiente
            log_info(logger, "PID: %d - Cache Hit - Pagina: %d", pid, pagina);  //LOGEA QUE ENCONTRO LA PAGINA Y EL VALOR DEL MARCO SE PASA POR REFERENCIA(en valor de cpu.c)
            return true;
        }
    }
    log_info(logger, "PID: %d - Cache Miss - Pagina: %d", pid, pagina); //LOGEA QUE NO SE ENCONTRO => SE PASA A BUSCAR EN LAS ENTRDAAS DE LA TLB
    return false;
}


void reemplazar_con_clock(t_entrada_cache* nueva, t_log* logger, int conexion_memoria, int pid) {
    while (1) {
        t_entrada_cache* actual = list_get(cache_paginas, puntero_clock);
        if (!actual->bit_uso) { //Si el bit de uso esta en falso (0), esta entrada es candidata a ser reemplazada
            if (actual->bit_modificado) { 
                escribir_pagina_memoria(pid, actual->marco, actual->contenido, conexion_memoria, logger, actual->pagina, actual->marco);
/*              t_paquete* paquete = crear_paquete(ESCRIBIR_PAGINA_COMPLETA); // ENVIO A MEMORIA EL CONTENIDO ACTUALIZADO(XQ EL BM ES 1)
                agregar_a_paquete(paquete, &pid, sizeof(int));
                agregar_a_paquete(paquete, &(actual->marco), sizeof(uint32_t));
                agregar_a_paquete(paquete, actual->contenido, strlen(actual->contenido) + 1);
                enviar_paquete(paquete, conexion_memoria);
                eliminar_paquete(paquete);

                int respuesta;
                recv(conexion_memoria, &respuesta, sizeof(int), 0);
                if(respuesta != OK){
                    log_error(logger, "No se escribio correctamente");
                }

                log_info(logger, "PID: %d - Memory Update - Página: %d - Frame: %d", actual->pid, actual->pagina, actual->marco);
*/
                log_debug(logger, "PID: %d - Memory Update - Página: %d", actual->pid, actual->pagina);
            }
            list_replace_and_destroy_element(cache_paginas, puntero_clock, nueva, free);  //@brief Remueve un elemento de la lista de una determinada posicion y loretorna.
            log_info(logger, "PID: %d - Cache Add - Pagina: %d", nueva->pid, nueva->pagina);
            puntero_clock = (puntero_clock + 1) % entradas_cache;  //AVANZA EL CLOCK HASTA LLEGAR A LA CANT DE ENTRADAS, LUEGO REPITE
            return;
        }
        actual->bit_uso = false;
        puntero_clock = (puntero_clock + 1) % entradas_cache;
    }
}

void agregar_a_cache(int pid, uint32_t pagina, char* contenido, t_log* logger, int conexion_memoria) {
    t_entrada_cache* nueva = malloc(sizeof(t_entrada_cache));
    nueva->pid = pid;
    nueva->pagina = pagina;
    nueva->contenido = strdup(contenido);
    nueva->bit_uso = true;
    nueva->bit_modificado = false; 

    if (list_size(cache_paginas) < entradas_cache) {
        list_add(cache_paginas, nueva);
        log_info(logger, "PID: %d - Cache Add - Pagina: %d", pid, pagina);
    } else {
        reemplazar_con_clock(nueva, logger, conexion_memoria, pid);
    }
}

void limpiar_cache_por_pid(int pid, int conexion_memoria, t_log* logger) {  //PARA CUANDO SE DESALOJA UN PROCESO
    for (int i = list_size(cache_paginas) - 1; i >= 0; i--) {
        t_entrada_cache* entrada = list_get(cache_paginas, i);
        if (entrada->pid == pid) {
            if (entrada->bit_modificado) {  // Calcular dir fisica y enviar write
                uint32_t direccion_fisica = entrada->marco * TAMANIO_PAGINA; //DESPLAZAMIENTO SERIA 0 xq escribo la pagina entera
                escribir_pagina_memoria(pid, direccion_fisica, entrada->contenido, conexion_memoria, logger, entrada->pagina, entrada->marco);
                /*
                t_paquete* paquete = crear_paquete(ESCRIBIR_PAGINA_COMPLETA);  
                agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
                agregar_a_paquete(paquete, entrada->contenido, strlen(entrada->contenido) + 1);
                enviar_paquete(paquete, conexion_memoria);
                eliminar_paquete(paquete);
                log_info(logger, "PID: %d - Memory Update - Página: %d - Frame: %d", entrada->pid, entrada->pagina, direccion_fisica / tamanio_pagina);
*/            }
            list_remove_and_destroy_element(cache_paginas, i, free);
        }
    }
}

void escribir_pagina_memoria(int pid, uint32_t direccion_fisica, void* valor, int conexion_memoria, t_log* logger, uint32_t pagina, uint32_t marco){
    t_paquete* paquete = crear_paquete(ESCRIBIR_PAGINA_COMPLETA); // ENVIO A MEMORIA EL CONTENIDO ACTUALIZADO(XQ EL BM ES 1)
    agregar_a_paquete(paquete, &pid, sizeof(int));
    agregar_a_paquete(paquete, &direccion_fisica, sizeof(uint32_t));
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