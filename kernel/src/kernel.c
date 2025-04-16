#include <kernel.h>

int main(int argc, char* argv[]) {
    kernel_logger = log_create("kernel.log", "[Kernel]", 1, LOG_LEVEL_INFO);
    inicializar_kernel(argv[1], argv[2], kernel_logger);
    
    log_info(kernel_logger,"Iniciando servidor Kernel");

	int server_fd_kernel_io = iniciar_servidor(kernel_logger, IP_KERNEL, PUERTO_KERNEL);

	// t_list* lista;
	while(1){
		int cliente_fd = esperar_cliente(kernel_logger, "Kernel", server_fd_kernel_io);
		if(cliente_fd != -1){
			int cod_op = recibir_operacion(cliente_fd);
			switch (cod_op) {
				case MENSAJE:
					recibir_mensaje(kernel_logger, cliente_fd);
					break;
				// case PAQUETE:
				// 	lista = recibir_paquete(cliente_fd);
				// 	log_info(logger, "Me llegaron los siguientes valores:\n");
				// 	list_iterate(lista, (void*) iterator);
				// 	break;
				case -1:
					log_error(kernel_logger, "El cliente se desconecto. Terminando servidor");
					return EXIT_FAILURE;
				default:
					log_warning(kernel_logger,"Operacion desconocida. No quieras meter la pata");
					break;
			}
		}
	}

    

    return EXIT_SUCCESS;
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


void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso, t_log* kernel_logger){
    t_config* config = iniciar_config();
    char* ip = config_get_string_value(config, "IP_MEMORIA");
    char* puerto = config_get_string_value(config, "PUERTO_MEMORIA");
    log_info(kernel_logger, "La ip de la memoria: %s \n", ip);
    int conexion = crear_conexion(kernel_logger, ip, puerto);
	enviar_mensaje("Conexion desde kernel", conexion);
}

