#include "../include/lectura_escritura.h"

void* obtener_lectura(uint32_t direccion_fisica, uint32_t tamanio) {
    if (direccion_fisica + tamanio > TAM_MEMORIA) return NULL;

    if(direccion_fisica/TAM_PAGINA != (direccion_fisica + tamanio - 1)/TAM_PAGINA) return NULL;

    int pid = obtener_pid_por_dir_fisica(direccion_fisica);

    log_info(memoria_logger,"## PID: %d - Lectura - Dir. Física: %d - Tamaño: %d",
    pid, (int) direccion_fisica, (int) tamanio);
    
    return espacio_usuario + direccion_fisica;
}

int escribir_espacio(uint32_t direccion_fisica, int tamanio, void* valor){
    if (direccion_fisica + tamanio > TAM_MEMORIA) return -1;

    if(direccion_fisica/TAM_PAGINA != (direccion_fisica + tamanio - 1)/TAM_PAGINA) return -1;

    int pid = obtener_pid_por_dir_fisica(direccion_fisica);

    log_info(memoria_logger,"## PID: %d - Escritura - Dir. Física: %d - Tamaño: %d",
    pid, (int) direccion_fisica, (int) tamanio);

    pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(espacio_usuario + direccion_fisica, valor, tamanio);
    pthread_mutex_unlock(&mutex_espacio_usuario);

    return 0;
}