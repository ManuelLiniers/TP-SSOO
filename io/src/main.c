#include "main.h"
#include "utils/hello.h"
#include "utils/commons.h"
#include "utils/socket.h"


int main(int argc, char* argv[]) {
    saludar("io");

    if (argc < 2) {
        printf("Debe indicarse el nombre del dispositivo IO como argumento\n");
        return EXIT_FAILURE;
    }

    char* nombre_dispositivo = argv[1];

    t_log* crear_log();

    t_log* logger = log_create("io.log", "[IO]", 1, LOG_LEVEL_DEBUG);



    char* ip;
    char* puerto;
    char* log_level;

    t_config* config = iniciar_config();

    ip = config_get_string_value(config, "IP_KERNEL");
    puerto = config_get_string_value(config, "PUERTO_KERNEL");
    log_level = config_get_string_value(config, "LOG_LEVEL");

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
    int* longitud_nombre_disp = malloc(sizeof(int));
    *longitud_nombre_disp = strlen(nombre_dispositivo) + 1;
    agregar_a_paquete(paqueteID, longitud_nombre_disp, sizeof(int));
    agregar_a_paquete(paqueteID, nombre_dispositivo, *longitud_nombre_disp);
    free(longitud_nombre_disp);
    
    enviar_paquete(paqueteID, conexion_kernel);
    eliminar_paquete(paqueteID);
    log_debug(logger, "PAQUETE ID ENVIADO");
    int operacion = recibir_operacion(conexion_kernel);
    procesar_io(conexion_kernel, logger, operacion);
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

// 🟦 Recibe y procesa peticiones de IO desde el Kernel
void procesar_io(int socket_kernel, t_log* logger, int codigo_operacion) {
    while (1) {
        if (recv(socket_kernel, &codigo_operacion, sizeof(op_code), MSG_WAITALL) <= 0) {
            log_error(logger, "Error al recibir código de operación del Kernel");
            break;
        }

        if (codigo_operacion == PETICION_IO) {
            int size;
            if (recv(socket_kernel, &size, sizeof(int), MSG_WAITALL) <= 0) {
                log_error(logger, "Error al recibir tamaño del stream");
                break;
            }

            void* stream = malloc(size);
            if (recv(socket_kernel, stream, size, MSG_WAITALL) <= 0) {
                log_error(logger, "Error al recibir stream de la petición IO");
                free(stream);
                break;
            }

            t_peticion_io* peticion = deserializar_peticion_io(stream);

            log_info(logger, "## PID: %d - Inicio de IO - Tiempo: %d", peticion->pid, peticion->tiempo);
            usleep(peticion->tiempo * 1000);  // milisegundos → microsegundos
            log_info(logger, "## PID: %d - Fin de IO", peticion->pid);

            // Enviar confirmación de fin al Kernel (opcional: podría ser un código o el PID)
            send(socket_kernel, &peticion->pid, sizeof(int), 0);

            free(stream);
            free(peticion);
        } else {
            log_warning(logger, "Código de operación no esperado: %d", codigo_operacion);
        }
    }
}

// 🟦 Deserializa el stream recibido en un struct
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
