#include "../include/atencion_a_cpu.h"

/*
	Explicacion CPU PEDIR_INSTRUCCION:
	1- CPU manda un paquete que incluye el pid del proceso junto con el pc que necesita 
	2- 
	3- Luego cada cliente indica lo que necesita mandando otro paquete
*/


void atender_cpu(int cpu_fd){
	while (1) {
		t_buffer* unBuffer;
        int op_code = recibir_operacion(cpu_fd);

		switch (op_code) {
			case PEDIR_INSTRUCCION: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_peticion_de_instruccion(unBuffer, cpu_fd);

				break;
			}
			case PEDIR_MARCO: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_peticion_marco(unBuffer, cpu_fd);

				break;
			}
			case LEER_MEMORIA: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_lectura_espacio_usuario(unBuffer, cpu_fd);

				break;
			}
			case ESCRIBIR_MEMORIA: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_escritura_espacio_usuario(unBuffer, cpu_fd);

				break;
			}
			case DUMP_MEMORY: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_dump_memory(unBuffer, cpu_fd);

				break;
			}
			case LEER_PAGINA_COMPLETA: {
				unBuffer = recibir_paquete(cpu_fd);
				atender_lectura_pagina_completa(unBuffer, cpu_fd);
				break;
			}
			case -1:{
				log_debug(memoria_logger, "[CPU] se desconecto. Terminando consulta");
				exit(0);
			}
            default: {
				log_warning(memoria_logger, "Operación desconocida de CPU");
                break;
			}
        }
		eliminar_buffer(unBuffer);
    }
}

void atender_peticion_de_instruccion(t_buffer* unBuffer, int cpu_fd){
    uint32_t pid;
	uint32_t pc;

	pid = recibir_uint32_del_buffer(unBuffer);
	pc = recibir_uint32_del_buffer(unBuffer);

	log_debug(memoria_logger, "CPU pide instrucción para PID %d, PC %d", pid, pc);

	t_proceso* un_proceso = obtener_proceso_por_id(pid);

	// Obtener la instruccion
	char* instruccion = obtener_instruccion_por_indice(un_proceso, pc);

	log_info(memoria_logger, "## PID: %d - Obtener instruccion: %d - Instruccion: %s", pid, pc, instruccion);
	
	enviar_instruccion_a_cpu(instruccion, cpu_fd);
}

void enviar_instruccion_a_cpu(char* instruccion, int cpu_fd){
    if (instruccion == NULL || cpu_fd < 0) {
        log_error(memoria_logger, "Parámetros inválidos");
        return;
    }

    op_code codigo = DEVOLVER_INSTRUCCION;
    send(cpu_fd, &codigo, sizeof(op_code), 0);

    uint32_t size = strlen(instruccion) + 1;
    send(cpu_fd, &size, sizeof(uint32_t), 0);

    send(cpu_fd, instruccion, size, 0);

    log_debug(memoria_logger, "Instrucción enviada: %s", instruccion);
}

void atender_peticion_marco(t_buffer* unBuffer, int cpu_fd){
	uint32_t pid;
	uint32_t nro_pagina_logica;

	pid = recibir_uint32_del_buffer(unBuffer);
	nro_pagina_logica = recibir_uint32_del_buffer(unBuffer);

	t_proceso* proceso = obtener_proceso_por_id(pid);

	t_tabla_nivel* tabla = proceso->tabla_paginas_raiz;
	if (tabla == NULL) {
        log_error(memoria_logger, "Tabla no encontrada para PID: %d", pid);
        int marco_invalido = -1;
        send(cpu_fd, &marco_invalido, sizeof(int), 0);
        return;
    }

	t_pagina* pagina = buscar_pagina_en_tabla(tabla, nro_pagina_logica, proceso->metricas);

	int marco = pagina->marco_asignado; 
	log_debug(memoria_logger, "Marco obtenido: %d", marco);

	send(cpu_fd, &marco, sizeof(int), 0);

	log_debug(memoria_logger, "Marco Enviado");
}

void atender_lectura_espacio_usuario(t_buffer* unBuffer, int cpu_fd){
	uint32_t direccion_fisica;
	uint32_t tamanio;

	direccion_fisica = recibir_uint32_del_buffer(unBuffer);
	tamanio = recibir_uint32_del_buffer(unBuffer);

	// void* lectura = malloc(tamanio);
	// memcpy(lectura, leer_valor(direccion_fisica, tamanio), tamanio);
	// send(cpu_fd, lectura, tamanio, 0);

	void* lectura = obtener_lectura(direccion_fisica, tamanio);

    if (lectura == NULL) {
        log_error(memoria_logger, "Fallo al leer memoria: dirección inválida o fuera de rango");
        return;
    }

	void* copia_lectura = malloc(tamanio);
	pthread_mutex_lock(&mutex_espacio_usuario);
    memcpy(copia_lectura, lectura, tamanio);
	pthread_mutex_unlock(&mutex_espacio_usuario);

    send(cpu_fd, copia_lectura, tamanio, 0);

    log_debug(memoria_logger, "Lectura enviada a CPU: dirección %u, tamaño %u", direccion_fisica, tamanio);

	free(copia_lectura);
}

void atender_escritura_espacio_usuario(t_buffer* unBuffer, int cpu_fd){
	uint32_t direccion_fisica;
	int tamanio;

	direccion_fisica = recibir_uint32_del_buffer(unBuffer);
	tamanio = recibir_int_del_buffer(unBuffer);
	void* valor = malloc(tamanio);
	valor = recibir_informacion_del_buffer(unBuffer, tamanio);

	int respuesta = OK;
	if(escribir_espacio(direccion_fisica, tamanio, valor) == -1){
		respuesta = -1;
	}

	send(cpu_fd, &respuesta, sizeof(int), 0);

	free(valor);
}

void atender_dump_memory(t_buffer* unBuffer, int cpu_fd) {
    uint32_t pid = recibir_uint32_del_buffer(unBuffer);

    int resultado = dump_de_memoria(pid);  // 0 si OK, -1 si hubo error

    int codigo_respuesta = resultado == 0 ? OK : -1;
    send(cpu_fd, &codigo_respuesta, sizeof(int), 0);

    if (codigo_respuesta == OK) {
        log_info(memoria_logger, "Dump de memoria exitoso para PID: %d", pid);
    } else {
        log_error(memoria_logger, "Fallo el dump de memoria para PID: %d", pid);
    }
}

void atender_lectura_pagina_completa(t_buffer* unBuffer, int cpu_fd) {
	uint32_t nro_marco;

	nro_marco = recibir_uint32_del_buffer(unBuffer);

	t_marco* marco = obtener_marco_por_nro_marco(nro_marco);
	if (!marco || marco->libre) {
		log_error(memoria_logger, "No se pudo leer marco %d: inválido o libre", nro_marco);
		uint32_t tamanio_error = 0;
		send(cpu_fd, &tamanio_error, sizeof(uint32_t), 0);
		return;
	}

	// Extraer contenido
	pthread_mutex_lock(&mutex_espacio_usuario);
	void* contenido = malloc(TAM_PAGINA);
	memcpy(contenido, espacio_usuario + marco->base, TAM_PAGINA);
	pthread_mutex_unlock(&mutex_espacio_usuario);

	// Enviar tamaño
	uint32_t tamanio = TAM_PAGINA;
	send(cpu_fd, &tamanio, sizeof(uint32_t), 0);

	// Enviar contenido
	send(cpu_fd, contenido, tamanio, 0);

	log_debug(memoria_logger, "Marco %d leído y enviado a CPU", nro_marco);

	free(contenido);
}