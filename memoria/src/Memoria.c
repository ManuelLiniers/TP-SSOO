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

	// inicializar_memoria();  (De momento no)


	// Aca se almacenan los procesos que llegan,  
	// lista_procesos_recibidos = list_create();

	server_fd_memoria = iniciar_servidor(memoria_logger, IP_MEMORIA, PUERTO_ESCUCHA);

//	while(servidor_escucha(server_fd_memoria));


	// t_list* lista;
	while(1){
		int cliente_fd = esperar_cliente(memoria_logger, "Memoria", server_fd_memoria);
		if(cliente_fd != -1){
			int op_code = recibir_operacion(cliente_fd);
			switch (op_code) {
				case MENSAJE:
					recibir_mensaje(memoria_logger, cliente_fd);
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

	finalizar_memoria();

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
	PATH_INSTRUCCIONES = config_get_string_value(config, "PATH_INSTRUCCIONES");
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
	log_info(memoria_logger, "PATH_INSTRUCCIONES: %s", PATH_INSTRUCCIONES);
}

/*
void inicializar_memoria(){
	espacio_usuario = malloc(TAM_MEMORIA);
	if(espacio_usuario == NULL){
		log_error(memoria_logger, "Error al crear memoria de usuario");
		exit(1);
	}
	log_info(memoria_logger, "Se inicia memoria con Paginacion.");
	
}
*/

void finalizar_memoria(){
	//free(espacio_usuario);
	log_info(memoria_logger, "Memoria terminada")
	log_destroy(memoria_logger);
	config_destroy(memoria_config);
}

/*
**************************************************************************************
**********************************COMUNICACION****************************************
**************************************************************************************
*/

void servidor_escucha(int server_fd_memoria){
	log_info(memoria_logger, "Iniciando servidor Memoria");
	while(1){
		int cliente_fd = esperar_cliente(memoria_logger, "Memoria", server_fd_memoria);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) saludar_cliente, args);
			log_info(memoria_logger, "[THREAD] Creo hilo para atender");
			pthread_detach(hilo_cliente);
			free(args);
		}
	}
}

void saludar_cliente(void *void_args){
	int* cliente_socket = (int*) void_args;
	int code_op = recibir_operacion(*cliente_socket);
	
	switch(code_op){
		case HANDSHAKE:
			int respuesta = 1;
			send(*cliente_socket, &respuesta, sizeof(int), 0);

			int op_code = recibir_operacion(*cliente_socket);
			if (op_code == IDENTIFICACION) {
				t_buffer* buffer = recibir_paquete(*cliente_socket);
				identificar_modulo(buffer, *cliente_socket);
				eliminar_buffer(buffer);
			} else {
				log_error(memoria_logger, "Esperaba IDENTIFICACION después de HANDSHAKE");
				close(*cliente_socket);
			}
			break; 
		case -1;
			log_error(memoria_logger. "Desconexion en el HANDSHAKE");
			break;
		default:
			log_error(memoria_logger. "Desconexion en el HANDSHAKE: Operacion Desconocida");
			break;
	} 
}


void identificar_modulo(t_buffer* unBuffer, int cliente_fd){
	// hacer funcion que reciba el id del modulo especifico
	int modulo_id = recibir_int_del_buffer(unBuffer);
	
	switch (modulo_id) {
		case KERNEL:
			fd_kernel = cliente_fd;
			log_info(memoria_logger, "[[[[[KERNEL CONECTADO]]]]]");
			atender_kernel(fd_kernel);

			break;
		case CPU:
			fd_cpu = cliente_fd;
			log_info(memoria_logger, "[[[[[CPU CONECTADO]]]]]");
			atender_cpu(fd_cpu);

			break;
		default:
			log_error(memoria_logger, "[%d]Error al identificar modulo",modulo_id);
			exit(EXIT_FAILURE);
			break;
	}
}

void atender_kernel(int kernel_fd){
	// hacer
	log_info(memoria_logger, "## Kernel Conectado - FD del socket: %d", kernel_fd);

	while (1) {
		int op_code = recibir_operacion(kernel_fd);

		switch (op_code) {
			case INICIAR_PROCESO: {
				unBuffer = recibir_paquete(kernel_fd);
				// mock de aceptacion
				mock_aceptacion();

				break;
			}
			case -1:{
				log_info(memoria_logger, "[KERNEL] se desconecto. Terminando consulta");
				exit(0);
			}
            default: {
				log_warning(memoria_logger, "Operación desconocida de KERNEL");
                break;
			}
		}
	}
}

void atender_cpu(int cpu_fd){
	// hacer
	while (1) {
		t_buffer* unBuffer;
        int op_code = recibir_operacion(cpu_fd);

		switch (op_code) {
			case PEDIR_INSTRUCCION: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_peticion_de_instruccion(unBuffer);

				break;
			}
			case -1:{
				log_info(memoria_logger, "[CPU] se desconecto. Terminando consulta");
				exit(0);
			}
            default: {
				log_warning(memoria_logger, "Operación desconocida de CPU");
                break;
			}
        }
    }
}