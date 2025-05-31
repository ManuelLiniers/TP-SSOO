#include "../include/memoria.h"

int main(int argc, char* argv[]) {
	inicializar_memoria();

	while(servidor_escucha(server_fd_memoria));


	finalizar_memoria();

    return EXIT_SUCCESS;
}

void leer_config(t_config* config){
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


void inicializar_memoria(){
	
	memoria_logger = log_create("Memoria.log", "[Memoria]", 1, LOG_LEVEL_INFO);
	
    memoria_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/Memoria.config");

	if (memoria_config == NULL) {
		log_error(memoria_logger, "No se pudo crear el config de la memoria");
		exit(1);
	}

	leer_config(memoria_config);
	//  leer_log(memoria_logger);  		(Para pruebas)


	espacio_usuario = malloc(TAM_MEMORIA);
	if(espacio_usuario == NULL){
    	log_error(memoria_logger, "Error al crear memoria de usuario");
    	exit(1);
	}

	// Inicializar lista para administración de marcos libres
	int cantidad_marcos = TAM_MEMORIA / TAM_PAGINA;
	lst_marcos = list_create();
	for(int i = 0; i < cantidad_marcos; i++) {
		t_marco* marco_nuevo  = crear_marco(TAM_PAGINA*i, true, i);
    	
		list_add(lst_marcos, marco_nuevo); // Todos los marcos libres inicialmente
	}


	log_info(memoria_logger,"Iniciando servidor Memoria");

	// Aca se almacenan los procesos que llegan,  
	procesos_memoria = list_create();

	server_fd_memoria = iniciar_servidor(memoria_logger, NULL, PUERTO_ESCUCHA);
}


void finalizar_memoria(){
	//free(espacio_usuario);
	log_info(memoria_logger, "Memoria terminada correctamente");
	log_destroy(memoria_logger);
	config_destroy(memoria_config);
	list_destroy(procesos_memoria);
}

/*
**************************************************************************************
**********************************COMUNICACION****************************************
**************************************************************************************
*/

int servidor_escucha(int server_fd_memoria){

	while(server_fd_memoria != -1){
		int cliente_fd = esperar_cliente(memoria_logger, "Memoria", server_fd_memoria);

		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) saludar_cliente, args);
			log_info(memoria_logger, "[THREAD] Creo hilo para atender cliente");
			pthread_detach(hilo_cliente);
		}
	}

	return EXIT_SUCCESS;
}

/*
	Explicacion:
	1- Llega el cliente y se hace el handshake (codigo de operacion HANDSHAKE)
	2- Cliente manda un paquete con codigo de operacion IDENTIFICACION
	      2-a El Contenido del paquete es un unico int que indica el modulo correspondiente
	3- Luego cada cliente indica lo que necesita mandando otro paquete
*/

void saludar_cliente(void *void_args){
	while(1){
		saludar_cliente_generico(memoria_logger, void_args, identificar_modulo);
	}
}


void identificar_modulo(t_buffer* unBuffer, int cliente_fd){
	// hacer funcion que reciba el id del modulo especifico
	int modulo_id = recibir_int_del_buffer(unBuffer);
	
	switch (modulo_id) {
		case KERNEL:
			int kernel_fd = cliente_fd;
			log_info(memoria_logger, "[[[[[KERNEL CONECTADO]]]]]");
			atender_kernel(kernel_fd);

			break;
		case CPU:
			int cpu_fd = cliente_fd;
			log_info(memoria_logger, "[[[[[CPU CONECTADO]]]]]");
			atender_cpu(cpu_fd);
			break;
		default:
			log_error(memoria_logger, "[%d] Error al identificar modulo",modulo_id);
			exit(EXIT_FAILURE);
			break;
	}
}