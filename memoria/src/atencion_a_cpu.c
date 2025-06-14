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
				atender_lectura_memoria(unBuffer, cpu_fd);

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

void atender_lectura_memoria(t_buffer* unBuffer, int cpu_fd){
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