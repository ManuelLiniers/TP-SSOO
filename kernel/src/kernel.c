#include <kernel.h>

int main(int argc, char* argv[]) {
    inicializar_kernel();

	iniciar_planificacion(argv[1], argv[2]);

    log_info(logger_kernel,"Iniciando servidor Kernel");
	int server_fd_kernel_io = iniciar_servidor(logger_kernel, IP_KERNEL, PUERTO_KERNEL); 
	//Si esta ESCUCHANDO en PUERTO KERNEL -> SE CONECTA CON IO PORQUE IO ES CLIENTE NADA MÁS DE ESTE PUERTO. SI ESCUCHA OTRO PUERTO, SE CONECTA A ALGUNO DE CPU

	// t_list* lista;
	while(1){
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_io);
		if(cliente_fd != -1){
			int cod_op = recibir_operacion(cliente_fd);
			switch (cod_op) {
				case MENSAJE:
					recibir_mensaje(logger_kernel, cliente_fd);
					break;
				// case PAQUETE:
				// 	lista = recibir_paquete(cliente_fd);
				// 	log_info(logger, "Me llegaron los siguientes valores:\n");
				// 	list_iterate(lista, (void*) iterator);
				// 	break;
				case -1:
					log_error(logger_kernel, "El cliente se desconecto. Terminando servidor");
					return EXIT_FAILURE;
				default:
					log_warning(logger_kernel,"Operacion desconocida. No quieras meter la pata");
					break;
			}
		}
	}

    

    return EXIT_SUCCESS;
}

void iniciar_config(void){
    config_kernel = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/kernel.config");
    if(config_kernel == NULL){
        perror("Error al crear el config del Kernel");
        exit(EXIT_FAILURE);
    }
	log_info(logger_kernel, "Config creada existosamente");
}


void inicializar_kernel(char* nombre_proceso, char* tamanio_proceso){
    iniciar_config();
	logger_kernel = log_create("kernel.log", "[Kernel]", 1, LOG_LEVEL_INFO);
    
	

	// estas conexiones iniciales creo que no van al inicializar el kernel
	
	/* char* ip = config_get_string_value(config_kernel, "IP_MEMORIA");
    char* puerto = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
    log_info(logger_kernel, "La ip de la memoria: %s \n", ip);

    int conexion = crear_conexion(logger_kernel, ip, puerto);
	enviar_mensaje("Conexion desde kernel", conexion); */
}

void inicializar_planificacion(char* nombre_proceso, char* tamanio_proceso){

	/* 
	Incializar la planificacion creando las colas y entiendo que crear hilo 
	para cada una de las planificaciones.
	Ver si hacer eso aca o derivarlo en el archivo scheduler.c
	*/

	pthread_t planificador_corto_plazo;
	pthread_create(&planificador_corto_plazo, NULL, planificar_corto_plazo, NULL);

	pthread_t planificador_largo_plazo;
	pthread_create(&planificador_largo_plazo, NULL, planificar_largo_plazo, NULL);


	// crear proceso inicial con los argumentos y mandarlo a planificar
}

