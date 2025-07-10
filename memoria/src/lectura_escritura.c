#include "../include/lectura_escritura.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return NULL;

    log_info(memoria_logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",
             pid, direccion_fisica, tamanio);

    t_proceso* proceso = obtener_proceso_por_id(pid, procesos_memoria);
    proceso->metricas->lecturas_memoria++;

    return espacio_usuario + direccion_fisica;
}

int escribir_espacio(uint32_t direccion_fisica, int tamanio, void* valor, int pid) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return -1;

    if (direccion_fisica / TAM_PAGINA != (direccion_fisica + tamanio - 1) / TAM_PAGINA) return -1;

    log_info(memoria_logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d",
             pid, direccion_fisica, tamanio);

    t_proceso* proceso = obtener_proceso_por_id(pid, procesos_memoria);
    proceso->metricas->escrituras_memoria++;

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(espacio_usuario + direccion_fisica, valor, tamanio);
    pthread_mutex_unlock(&mutex_espacio_usuario);

    return 0;
}

int dump_de_memoria(uint32_t pid) {
    log_info(memoria_logger, "## PID: %d - Memory Dump solicitado", pid);

    t_proceso* proceso = obtener_proceso_por_id(pid, procesos_memoria);
    if (!proceso) {
        log_error(memoria_logger, "PID: %d - No se encuentra el proceso para el dump", pid);
        return -1;
    }

    char* timestamp = temporal_get_string_time("%H:%M:%S:%MS");

    char path_dump[256];
    snprintf(path_dump, sizeof(path_dump), "%s%d-%s.dmp", DUMP_PATH, pid, timestamp);

    FILE* archivo = fopen(path_dump, "w");
    if (!archivo) {
        log_error(memoria_logger, "No se pudo crear archivo de dump para PID: %d", pid);
        return -1;
    }

    int paginas = proceso->paginas;
    int fd = fileno(archivo);
    ftruncate(fd, paginas * TAM_PAGINA);

    escribir_marcos_en_archivo(archivo, proceso->tabla_paginas_raiz, &paginas);

    fclose(archivo);

    log_debug(memoria_logger, "Marcos escritos");
    return 0;
}

void escribir_marcos_en_archivo(FILE* archivo, t_tabla_nivel* tabla, int* paginas_restantes){
    for (int i = 0; i < ENTRADAS_POR_TABLA && *paginas_restantes > 0; i++) {

        if (tabla->nivel == CANTIDAD_NIVELES) {
            t_pagina* pagina = (t_pagina*) list_get(tabla->entradas, i);

            if (pagina->marco_asignado != -1) {
                poner_marco_en_archivo(archivo, pagina->marco_asignado);
                (*paginas_restantes)--;
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) list_get(tabla->entradas, i);
            escribir_marcos_en_archivo(archivo, subtabla, paginas_restantes);
        }
    }
}

void poner_marco_en_archivo(FILE* archivo, int marco){

    if (archivo == NULL || marco < 0) {
        log_error(memoria_logger, "Parámetros inválidos");
        return;
    }

    void* buffer_para_escritura = malloc(TAM_PAGINA);

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(buffer_para_escritura, espacio_usuario + marco * TAM_PAGINA, TAM_PAGINA);    
    pthread_mutex_unlock(&mutex_espacio_usuario);

    fwrite(buffer_para_escritura, 1, TAM_PAGINA, archivo);
}