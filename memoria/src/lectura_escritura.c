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

int dump_de_memoria(uint32_t pid) {
    t_proceso* proceso = obtener_proceso_por_id(pid);
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

    pthread_mutex_lock(&proceso->mutex_TP);
    escribir_marcos_en_archivo(archivo, proceso->tabla_paginas_raiz);
    pthread_mutex_unlock(&proceso->mutex_TP);

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
    // Validaciones iniciales
    if (archivo == NULL || marco < 0) {
        log_error(memoria_logger, "Parámetros inválidos");
        return;
    }

    const uint8_t* inicio_marco = (uint8_t*)(espacio_usuario + marco);
    const int bytes_por_linea = 16;
    char buffer_linea[bytes_por_linea * 3 + 1]; // 2 chars por byte + espacio + \0

    for (int j = 0; j < TAM_PAGINA; j++) {
        // Alternativa más segura para acceso a memoria
        uint8_t valor = inicio_marco[j];
        
        // Formatear en buffer
        sprintf(buffer_linea + (j % bytes_por_linea) * 3, "%02X ", valor);

        // Escribir línea completa
        if ((j + 1) % bytes_por_linea == 0 || j == TAM_PAGINA - 1) {
            fprintf(archivo, "%s\n", buffer_linea);
        }
    }
}