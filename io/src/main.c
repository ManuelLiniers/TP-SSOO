#include "main.h"
#include "utils/hello.h"
#include "utils/commons.h"
#include "utils/socket.h"


int main(int argc, char* argv[]) {
    //saludar("io");

    if (argc < 2) {
        printf("Debe indicarse el nombre del dispositivo IO como argumento\n");
        return EXIT_FAILURE;
    }

    char* nombre_dispositivo = argv[1];

    char* ip;
    char* puerto;
    char* log_level;

    t_config* config = iniciar_config();

    ip = config_get_string_value(config, "IP_KERNEL");
    puerto = config_get_string_value(config, "PUERTO_KERNEL");
    log_level = config_get_string_value(config, "LOG_LEVEL");

    t_log* logger = log_create("io.log", "[IO]", 1, log_level_from_string(log_level));

    log_debug(logger, "El valor de la ip es: %s", ip);
    log_debug(logger, "El valor del puerto es: %s", puerto);
    log_debug(logger, "El valor del log level es: %s", log_level);

    int conexion_kernel = crear_conexion(logger, ip, puerto);
    if (conexion_kernel == -1) {
        log_error(logger, "No se pudo conectar al Kernel");
        return EXIT_FAILURE;
    }


    log_debug(logger, "Me conecto al kernel como: %s", nombre_dispositivo);
    
    hacer_handshake(logger, conexion_kernel);
    t_paquete* paqueteID = crear_paquete(IDENTIFICACION);
    int longitud_nombre_disp = strlen(nombre_dispositivo) + 1;
    agregar_a_paquete(paqueteID, &longitud_nombre_disp, sizeof(int));
    agregar_a_paquete(paqueteID, nombre_dispositivo, strlen(nombre_dispositivo) + 1);
    
    enviar_paquete(paqueteID, conexion_kernel);
    eliminar_paquete(paqueteID);
    log_debug(logger, "PAQUETE ID ENVIADO");
    procesar_io(conexion_kernel, logger);
    terminar_programa(logger, config, conexion_kernel);
    return 0;
}

t_config* iniciar_config(void) {
	t_config* nuevo_config = config_create("io.config");
	if(nuevo_config == NULL) {
        perror("Error al intentar cargar el config");
        exit(EXIT_FAILURE);
	}
    //printf("Se creo exitosamente la config del io\n");
	return nuevo_config;
}

void procesar_io(int socket_kernel, t_log* logger) {
    while (1) {
        int codigo_operacion = recibir_operacion(socket_kernel);
        if (codigo_operacion <= 0) {
            log_error(logger, "Error al recibir código de operación del Kernel");
            break;
        }
        if (codigo_operacion == PETICION_IO) {
            /* int size;
            if (recv(socket_kernel, &size, sizeof(int), MSG_WAITALL) <= 0) {
                log_error(logger, "Error al recibir tamaño del stream");
                break;
            }

            
            void* stream = malloc(size);
            if (recv(socket_kernel, stream, size, MSG_WAITALL) <= 0) {
                log_error(logger, "Error al recibir stream de la petición IO");
                free(stream);
                break;
            } */

            t_buffer* paquete = recibir_paquete(socket_kernel);
            t_peticion_io* peticion = malloc(sizeof(t_peticion_io));
            peticion->pid = recibir_int_del_buffer(paquete);
            peticion->tiempo = recibir_int_del_buffer(paquete);

            log_info(logger, "## PID: %d - Inicio de IO - Tiempo: %d", peticion->pid, peticion->tiempo);
            usleep(peticion->tiempo * 1000);  // milisegundos → microsegundos
            log_info(logger, "## PID: %d - Fin de IO", peticion->pid);

            // Enviar confirmación de fin al Kernel (opcional: podría ser un código o el PID)
            t_paquete* paquete_vuelta = crear_paquete(FINALIZACION_IO);
            agregar_a_paquete(paquete_vuelta, &(peticion->pid), sizeof(int));
            enviar_paquete(paquete_vuelta, socket_kernel);

            free(peticion);
            eliminar_buffer(paquete);
        } else {
            log_warning(logger, "Código de operación no esperado: %d", codigo_operacion);
        }
    }
}

t_peticion_io* deserializar_peticion_io(void* stream) {
    t_peticion_io* peticion = malloc(sizeof(t_peticion_io));
    memcpy(&peticion->pid, stream, sizeof(int));
    memcpy(&peticion->tiempo, stream + sizeof(int), sizeof(int));
    return peticion;
}

void terminar_programa(t_log* logger, t_config* config, int conexion) {
    log_destroy(logger);
    config_destroy(config);
    close(conexion);
}
