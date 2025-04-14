#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/Memoria.h>



int main(int argc, char* argv[]) {
    memoria_logger = log_create("Memoria.log", "[Memoria]", 1, LOG_LEVEL_INFO);
	
    memoria_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/Memoria.config");

	if (memoria_config == NULL) {
// 		¡No se pudo crear el config!
		log_error(memoria_logger, "No se pudo crear el config de la memoria");
// 		Terminemos el programa
		exit(1);
	}

	leer_config(memoria_config);

//  leer_log(memoria_logger);  		(Para pruebas)

	log_info(memoria_logger,"Iniciando servidor Memoria");

	server_fd_memoria = iniciar_servidor(memoria_logger, IP_MEMORIA, PUERTO_ESCUCHA);

	// t_list* lista;
	while(1){
		int cliente_fd = esperar_cliente(memoria_logger, "Memoria", server_fd_memoria);
		if(cliente_fd != -1){
			int cod_op = recibir_operacion(cliente_fd);
			switch (cod_op) {
				case MENSAJE:
					recibir_mensaje(cliente_fd);
					break;
				// case PAQUETE:
				// 	lista = recibir_paquete(cliente_fd);
				// 	log_info(logger, "Me llegaron los siguientes valores:\n");
				// 	list_iterate(lista, (void*) iterator);
				// 	break;
				case -1:
					log_error(memoria_logger, "El cliente se desconecto. Terminando servidor");
					return EXIT_FAILURE;
				default:
					log_warning(memoria_logger,"Operacion desconocida. No quieras meter la pata");
					break;
			}
		}
	}

    return EXIT_SUCCESS;
}

void leer_config(t_config* config){
//	IP_MEMORIA = NULL;
	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	TAM_MEMORIA = config_get_int_value(config, "TAM_MEMORIA");
	TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");
	ENTRADAS_POR_TABLA = config_get_int_value(config, "ENTRADAS_POR_TABLA");
	CANTIDAD_NIVELES = config_get_int_value(config, "CANTIDAD_NIVELES");
	RETARDO_MEMORIA = config_get_int_value(config, "RETARDO_MEMORIA");
	PATH_SWAPFILE = config_get_string_value(config, "PATH_SWAPFILE");
	RETARDO_SWAP = config_get_int_value(config, "RETARDO_SWAP");
	LOG_LEVEL = config_get_string_value(config, "LOG_LEVEL");
	DUMP_PATH = config_get_string_value(config, "DUMP_PATH");
}

void leer_log(t_log* logger){
	log_info(memoria_logger, "PUERTO_ESCUCHA: %s", PUERTO_ESCUCHA);
	log_info(memoria_logger, "TAM_MEMORIA: %d", TAM_MEMORIA);
	log_info(memoria_logger, "TAM_PAGINA: %d", TAM_PAGINA);
	log_info(memoria_logger, "ENTRADAS_POR_TABLA: %d", ENTRADAS_POR_TABLA);
	log_info(memoria_logger, "CANTIDAD_NIVELES: %d", CANTIDAD_NIVELES);
	log_info(memoria_logger, "RETARDO_MEMORIA: %d", RETARDO_MEMORIA);
	log_info(memoria_logger, "PATH_SWAPFILE: %s", PATH_SWAPFILE);
	log_info(memoria_logger, "RETARDO_SWAP: %d", RETARDO_SWAP);
	log_info(memoria_logger, "LOG_LEVEL: %s", LOG_LEVEL);
	log_info(memoria_logger, "DUMP_PATH: %s", DUMP_PATH);
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(memoria_logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}