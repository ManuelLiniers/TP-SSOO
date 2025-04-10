#include <utils/hello.h>
#include <main.h>

int main(int argc, char* argv[]) {
    saludar("kernel");
    // inicializar_kernel(argv[1], argv[2]);
    t_config* config = iniciar_config();
    // char* clave = config_get_string_value(config, "CLAVE");
    char* ip = config_get_string_value(config, "IP_MEMORIA");
    char* puerto = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
    printf("La clave es: %s", ip, "\n");
    iniciar_conexion();
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

void iniciar_conexion(){

}

/*

void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso){
    printf("El proceso que introdujo se llama %s", nombre_proceso, "\n");
    printf("Su peso es %s", tamanio_proceso, "\n");
}

*/