#include "main.h"
#include <utils/hello.h>
#include <utils/commons.h>
#include</home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/socket.h>


int main(int argc, char* argv[]) {
    saludar("io");

    if (argc < 2) {
        printf("Debe indicarse el nombre del dispositivo IO como argumento\n");
        return EXIT_FAILURE;
    }

    char* nombre_dispositivo = argv[1];

    t_log* crear_log();

    t_log* logger = log_create("io.log", "[IO]", 1, LOG_LEVEL_INFO);
    if(logger == NULL){
        perror("error al crear el logger");
        abort();
    } 



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

     int conexion_kernel = crear_conexion(logger, ip, puerto);
    if (conexion_kernel == -1) {
        log_error(logger, "No se pudo conectar al Kernel");
        return EXIT_FAILURE;
    }

    enviar_mensaje(nombre_dispositivo, conexion_kernel);
    log_info(logger, "Me conecto al kernel como: %s", nombre_dispositivo);

    terminar_programa(logger, config, conexion_kernel);
    return 0;
}

t_config* iniciar_config(void) {
	    t_config* nuevo_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/io/io.config");
	    if(nuevo_config == NULL) {
		    perror("Error al intentar cargar el config");
		    exit(EXIT_FAILURE);
	    }
    printf("Se creo exitosamente la config del io\n");
	return nuevo_config;
}

void terminar_programa(t_log* logger, t_config* config, int conexion) {
    log_destroy(logger);
    config_destroy(config);
    close(conexion);
}
