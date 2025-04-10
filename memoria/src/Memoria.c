#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/Memoria.h>



int main(int argc, char* argv[]) {
    memoria_logger = log_create("Memoria.log", "[Memoria]", 1, LOG_LEVEL_INFO);
	
    memoria_config = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/Memoria.config");

	if (memoria_config == NULL) {
    	// ¡No se pudo crear el config!
		log_error(memoria_logger, "No se pudo crear el config de la memoria");
    	// Terminemos el programa
		exit(1);
	}

	leer_config(memoria_config);

	leer_log(memoria_logger);

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