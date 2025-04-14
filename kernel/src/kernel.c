#include <kernel.h>

int main(int argc, char* argv[]) {
    inicializar_kernel(argv[1], argv[2]);
    return 0;
}

t_config* iniciar_config(void){
    t_config* nuevo_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/kernel.config");
    if(nuevo_config == NULL){
        perror("Error al crear el config del Kernel");
        exit(EXIT_FAILURE);
    }
    printf("Se creo exitosamente el config del Kernel \n");
    return nuevo_config;
}


void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso){
    t_config* config = iniciar_config();
    kernel_logger = log_create("kernel.log", "[Kernel]", 1, LOG_LEVEL_INFO);
    char* ip = config_get_string_value(config, "IP_MEMORIA");
    char* puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    log_info(kernel_logger, "La ip de la memoria: %s \n", ip);
    int conexion = crear_conexion(kernel_logger, ip, puerto);
}

