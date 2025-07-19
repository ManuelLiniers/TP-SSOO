#include "../include/memoria.h"
#include <math.h>

int main(int argc, char* argv[]) {
	char* bits = inicializar_memoria();

	while(servidor_escucha(server_fd_memoria));


	finalizar_memoria(bits);

    return EXIT_SUCCESS;
}

void leer_config(t_config* config, t_config* pruebas){
	PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	LOG_LEVEL = config_get_string_value(config, "LOG_LEVEL");
	DUMP_PATH = config_get_string_value(config, "DUMP_PATH");
	PATH_INSTRUCCIONES = config_get_string_value(config, "PATH_INSTRUCCIONES");
	PATH_SWAPFILE = config_get_string_value(config, "PATH_SWAPFILE");
	
	TAM_MEMORIA = config_get_int_value(pruebas, "TAM_MEMORIA");
	TAM_PAGINA = config_get_int_value(pruebas, "TAM_PAGINA");
	ENTRADAS_POR_TABLA = config_get_int_value(pruebas, "ENTRADAS_POR_TABLA");
	CANTIDAD_NIVELES = config_get_int_value(pruebas, "CANTIDAD_NIVELES");
	RETARDO_MEMORIA = config_get_int_value(pruebas, "RETARDO_MEMORIA") * 1000;
	RETARDO_SWAP = config_get_int_value(pruebas, "RETARDO_SWAP") * 1000;
}

char* inicializar_memoria(){	
    memoria_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/Memoria.config");
	//pruebas_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/corto_plazo.config");
	//pruebas_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/largo_plazo.config");
	pruebas_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/estabilidad_general.config");
	if (memoria_config == NULL) {
		log_error(memoria_logger, "No se pudo crear el config de la memoria");
		exit(1);
	}
	
	leer_config(memoria_config, pruebas_config);
	
	memoria_logger = log_create("Memoria.log", "[Memoria]", 1, log_level_from_string(LOG_LEVEL));

	espacio_usuario = calloc(1, TAM_MEMORIA);
	if(espacio_usuario == NULL){
    	log_error(memoria_logger, "Error al crear memoria de usuario");
    	return NULL;
	}
	pthread_mutex_init(&mutex_espacio_usuario, NULL);

	// Inicializar lista para administración de marcos libres
	marcos_totales = TAM_MEMORIA / TAM_PAGINA;
	int cantidad_bytes = ceil((float)marcos_totales /8);
	char* bits = calloc(marcos_totales, 1);
	bit_marcos = bitarray_create_with_mode(bits, cantidad_bytes, LSB_FIRST);
	pthread_mutex_init(&mutex_bit_marcos, NULL);


	log_debug(memoria_logger,"Iniciando servidor Memoria");

	// Aca se almacenan los procesos que llegan,  
	procesos_memoria = list_create();

	procesos_swap = list_create();
	lista_swap = list_create();

	pthread_mutex_init(&mutex_procesos_memoria, NULL);
	pthread_mutex_init(&mutex_procesos_swap, NULL);
	pthread_mutex_init(&mutex_lista_swap, NULL);
	pthread_mutex_init(&mutex_manejando_swap, NULL);

	FILE* swap = fopen(PATH_SWAPFILE, "w");
	if(!swap){
		log_error(memoria_logger, "Error al crear archivo de swap");
	}
	fclose(swap);

	server_fd_memoria = iniciar_servidor(memoria_logger, NULL, PUERTO_ESCUCHA);

	return bits;
}


void finalizar_memoria(char* bits){
	free(espacio_usuario);
	free(bits);
	log_debug(memoria_logger, "Memoria terminada correctamente");
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
			log_debug(memoria_logger, "[THREAD] Creo hilo para atender cliente");
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
	saludar_cliente_generico(memoria_logger, void_args, identificar_modulo);
}


void identificar_modulo(t_buffer* unBuffer, int cliente_fd){
	// hacer funcion que reciba el id del modulo especifico
	int modulo_id = recibir_int_del_buffer(unBuffer);
	
	switch (modulo_id) {
		case KERNEL:
			int kernel_fd = cliente_fd;
			log_debug(memoria_logger, "[[[[[KERNEL CONECTADO]]]]]");
			atender_kernel(kernel_fd);

			break;
		case CPU:
			int cpu_fd = cliente_fd;
			log_debug(memoria_logger, "[[[[[CPU CONECTADO]]]]]");
			atender_cpu(cpu_fd);
			break;
		default:
			log_error(memoria_logger, "[%d] Error al identificar modulo",modulo_id);
			return;
			break;
	}
}