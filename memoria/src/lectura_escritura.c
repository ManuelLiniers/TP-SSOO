#include "../include/lectura_escritura.h"

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return NULL;

    int pid = obtener_pid_por_dir_fisica(direccion_fisica);

    log_info(memoria_logger,"## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",
    pid, (int) direccion_fisica, (int) tamanio);
    
    return espacio_usuario + direccion_fisica;
}