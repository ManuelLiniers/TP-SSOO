#include "../include/lectura_escritura.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return NULL;

    if (direccion_fisica / TAM_PAGINA != (direccion_fisica + tamanio - 1) / TAM_PAGINA) return NULL;

    int pid = obtener_pid_por_dir_fisica(direccion_fisica);

    log_info(memoria_logger, "## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",
             pid, direccion_fisica, tamanio);

    t_proceso* proceso = obtener_proceso_por_id(pid);
    proceso->metricas->lecturas_memoria++;

    return espacio_usuario + direccion_fisica;
}

int escribir_espacio(uint32_t direccion_fisica, int tamanio, void* valor) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return -1;

    if (direccion_fisica / TAM_PAGINA != (direccion_fisica + tamanio - 1) / TAM_PAGINA) return -1;

    int pid = obtener_pid_por_dir_fisica(direccion_fisica);

    log_info(memoria_logger, "## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d",
             pid, direccion_fisica, tamanio);

    t_proceso* proceso = obtener_proceso_por_id(pid);
    proceso->metricas->escrituras_memoria++;

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(espacio_usuario + direccion_fisica, valor, tamanio);
    pthread_mutex_unlock(&mutex_espacio_usuario);

    return 0;
}

void dump_de_memoria(uint32_t pid) {
    t_proceso* proceso = obtener_proceso_por_id(pid);
    if (!proceso) {
        log_error(memoria_logger, "PID: %d - No se encuentra el proceso para el dump", pid);
        return;
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
        return;
    }

    fprintf(archivo, "===== MEMORY DUMP: PID %d =====\n", pid);

    for (int i = 0; i < TAM_MEMORIA; i += TAM_PAGINA) {
        int pid_actual = obtener_pid_por_dir_fisica(i);
        if (pid_actual != pid) continue;

        fprintf(archivo, "\n-- Dir. Física: %d --\n", i);
        for (int j = 0; j < TAM_PAGINA; j++) {
            uint8_t valor = *((uint8_t*)(espacio_usuario + i + j));
            fprintf(archivo, "%02X ", valor);
            if ((j + 1) % 16 == 0) fprintf(archivo, "\n");
        }
    }

    fclose(archivo);

    log_info(memoria_logger, "## PID: %d - Memory Dump solicitado", pid);
}