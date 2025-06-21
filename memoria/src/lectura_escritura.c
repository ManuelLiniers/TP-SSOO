#include "../include/lectura_escritura.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio, int pid) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return NULL;

    if (direccion_fisica / TAM_PAGINA != (direccion_fisica + tamanio - 1) / TAM_PAGINA) return NULL;

    log_info(memoria_logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",
             pid, direccion_fisica, tamanio);

    t_proceso* proceso = obtener_proceso_por_id(pid);
    proceso->metricas->lecturas_memoria++;

    return espacio_usuario + direccion_fisica;
}

int escribir_espacio(uint32_t direccion_fisica, int tamanio, void* valor, int pid) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return -1;

    if (direccion_fisica / TAM_PAGINA != (direccion_fisica + tamanio - 1) / TAM_PAGINA) return -1;

    log_info(memoria_logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d",
             pid, direccion_fisica, tamanio);

    t_proceso* proceso = obtener_proceso_por_id(pid);
    proceso->metricas->escrituras_memoria++;

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(espacio_usuario + direccion_fisica, valor, tamanio);
    pthread_mutex_unlock(&mutex_espacio_usuario);

    return 0;
}


void leer_pagina_completa(uint32_t nro_marco, int socket_destino) {
    void* ptr_base = espacio_usuario + nro_marco * TAM_PAGINA;

    char* contenido = malloc(TAM_PAGINA);
    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(contenido, ptr_base, TAM_PAGINA);
    pthread_mutex_unlock(&mutex_espacio_usuario);

    uint32_t tamanio = TAM_PAGINA;
    send(socket_destino, &tamanio, sizeof(uint32_t), 0);
    send(socket_destino, contenido, tamanio, 0);

    free(contenido);

    log_debug(memoria_logger, "Marco %d enviado a CPU (%d bytes)", nro_marco, tamanio);
}

void escribir_pagina_completa(uint32_t nro_marco, void* contenido) {
    void* ptr_base = espacio_usuario + nro_marco * TAM_PAGINA;

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(ptr_base, contenido, TAM_PAGINA);
    pthread_mutex_unlock(&mutex_espacio_usuario);

    //log_debug(memoria_logger, "PID: %d - Escritura completa - Marco: %d - Pagina: %d", pid, nro_marco, pagina);
}

int dump_de_memoria(uint32_t pid) {
    t_proceso* proceso = obtener_proceso_por_id(pid);
    if (!proceso) {
        log_error(memoria_logger, "PID: %d - No se encuentra el proceso para el dump", pid);
        return -1;
    }

    time_t ahora = time(NULL);
    struct tm* t = localtime(&ahora);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", t);

    char path_dump[256];
    snprintf(path_dump, sizeof(path_dump), "%s/pid%d_dump_%s.txt", DUMP_PATH, pid, timestamp);

    FILE* archivo = fopen(path_dump, "w");
    if (!archivo) {
        log_error(memoria_logger, "No se pudo crear archivo de dump para PID: %d", pid);
        return -1;
    }

    pthread_mutex_lock(&proceso->mutex_TP);
    escribir_marcos_en_archivo(archivo, proceso->tabla_paginas_raiz);
    pthread_mutex_unlock(&proceso->mutex_TP);

    /*for (int i = 0; i < TAM_MEMORIA; i += TAM_PAGINA) {
        int pid_actual = obtener_pid_por_dir_fisica(i);
        if (pid_actual != pid) continue;

        fprintf(archivo, "\n-- Dir. Física: %d --\n", i);
        for (int j = 0; j < TAM_PAGINA; j++) {
            uint8_t valor = *((uint8_t*)(espacio_usuario + i + j));
            fprintf(archivo, "%02X ", valor);
            if ((j + 1) % 16 == 0) fprintf(archivo, "\n");
        }
    }*/

    fclose(archivo);

    // Log obligatorio según el enunciado
    log_info(memoria_logger, "## PID: %d - Memory Dump solicitado", pid);

    return 0;
}

void escribir_marcos_en_archivo(FILE* archivo, t_tabla_nivel* tabla){
    for (int i = 0; i < ENTRADAS_POR_TABLA; i++) {
        t_entrada_tabla* entrada = list_get(tabla->entradas, i);

        if (entrada->es_ultimo_nivel) {
            t_pagina* pagina = (t_pagina*) entrada->siguiente_nivel;
            if (pagina->marco_asignado != -1) {
                poner_marco_en_archivo(archivo, pagina->marco_asignado);
            }
        } else {
            t_tabla_nivel* subtabla = (t_tabla_nivel*) entrada->siguiente_nivel;
            escribir_marcos_en_archivo(archivo, subtabla);  // llamada recursiva
        }
    }
}

void poner_marco_en_archivo(FILE* archivo, int marco){
    for (int j = 0; j < TAM_PAGINA; j++) {
        uint8_t valor = *((uint8_t*)(espacio_usuario + marco + j));
        fprintf(archivo, "%02X ", valor);
        if ((j + 1) % 16 == 0) fprintf(archivo, "\n");
    }
}