#include "main.h"
#include <utils/hello.h>

int main(int argc, char* argv[]) {
    saludar("io");

    char* ip;
    char* puerto;
    char* log_level;

    t_config* config = iniciar_config();

    ip = config_get_string_value(config, "IP_KERNEL");
    puerto = config_get_string_value(config, "PUERTO_KERNEL");
    log_level = config_get_string_value(config, "LOG_LEVEL");

    printf("El valor de la ip es: %s", ip);
    printf("\nEl valor del puerto es: %s", puerto);
    printf("\nEl valor del log level es: %s", log_level);

    return 0;
}

t_config* iniciar_config(void) {
	    t_config* nuevo_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/io/io.config");
	    if(nuevo_config == NULL) {
		    perror("Error al intentar cargar el config");
		    exit(EXIT_FAILURE);
	    }
    printf("Se creo exitosamente la config del io");
	return nuevo_config;
}