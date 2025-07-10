#include "../include/atencion_a_kernel.h"


void atender_kernel(int kernel_fd){ // agregar que reciba el buffer

	log_info(memoria_logger, "## Kernel Conectado - FD del socket: %d", kernel_fd);

	while (1) {
		int op_code = recibir_operacion(kernel_fd); // sugiero cambiar recibir_operacion(kernel_fd) por recibir_int_del_buffer(buffer)
		t_buffer* unBuffer;

		switch (op_code) {
			case INICIAR_PROCESO: {
				unBuffer = recibir_paquete(kernel_fd);
				
				iniciar_proceso(unBuffer, kernel_fd);

				break;
			}
			case FINALIZAR_PROCESO: {
				unBuffer = recibir_paquete(kernel_fd);
				
				fin_proceso(unBuffer, kernel_fd);

				break;
			}
			case DUMP_MEMORY: {
				unBuffer = recibir_paquete(kernel_fd);
				
				atender_dump_memory(unBuffer, kernel_fd);

				break;
			}
			case SWAP: {
				unBuffer = recibir_paquete(kernel_fd);
				
				atender_swap(unBuffer, kernel_fd);

				break;
			}
			case VUELTA_SWAP: {
				unBuffer = recibir_paquete(kernel_fd);
				
				atender_vuelta_swap(unBuffer, kernel_fd);

				break;
			}
			case -1:{
				log_debug(memoria_logger, "[KERNEL] se desconecto. Terminando consulta");
				exit(0);
			}
            default: {
				log_warning(memoria_logger, "Operación desconocida de KERNEL");
                break;
			}
		}
		eliminar_buffer(unBuffer);
	}
}

void iniciar_proceso(t_buffer* unBuffer, int kernel_fd){
    uint32_t pid;
	uint32_t tamanio;

	pid = recibir_int_del_buffer(unBuffer);
	tamanio = recibir_int_del_buffer(unBuffer);
// 	Espero a verificar si hay espacio para asignar el path

    log_debug(memoria_logger, "Pedido de crear proceso PID %d con tamaño %d", pid, tamanio);

	uint32_t marcos_libres = cantidad_de_marcos_libres();
	uint32_t espacio_disponible = marcos_libres * ((uint32_t) TAM_PAGINA);

	if(espacio_disponible >= tamanio){
    	int respuesta = OK;
    	send(kernel_fd, &respuesta, sizeof(int), 0);
		log_debug(memoria_logger, "Creando proceso PID %d", pid);

		int longitud = recibir_int_del_buffer(unBuffer);
		char* path_instrucciones = recibir_informacion_del_buffer(unBuffer, longitud);

		t_proceso* procesoNuevo = crear_proceso(pid, tamanio, path_instrucciones);

		if(tamanio != 0){
			int paginas = procesoNuevo->paginas;
			asignar_marcos_a_tabla(procesoNuevo->tabla_paginas_raiz, &paginas);
		}

		procesoNuevo->metricas->subidas_memoria++;
	
		log_info(memoria_logger, "## PID: <%d> - Proceso Creado - Tamaño: <%d>", pid, tamanio);

		agregar_proceso_a_lista(procesoNuevo, procesos_memoria);
		int aviso = PROC_CREADO;
    	send(kernel_fd, &aviso, sizeof(int), 0);
	} else {
		int respuesta = SIN_ESPACIO;
    	send(kernel_fd, &respuesta, sizeof(int), 0);
		log_debug(memoria_logger, "No hay espacio para proceso PID %d", pid);
	}
}

void fin_proceso(t_buffer* unBuffer, int kernel_fd){
	uint32_t pid;

	pid = recibir_int_del_buffer(unBuffer);

	t_proceso* proceso = obtener_proceso_por_id(pid, procesos_memoria);

	exponer_metricas(proceso->metricas, pid);

	list_remove_element(procesos_memoria, proceso);

	finalizar_proceso(proceso);
}

void atender_dump_memory(t_buffer* unBuffer, int kernel_fd) {
    uint32_t pid = recibir_uint32_del_buffer(unBuffer);

    int resultado = dump_de_memoria(pid);  // 0 si OK, -1 si hubo error

    int codigo_respuesta = OK;
	if(resultado != 0){
		codigo_respuesta = -1;
	}
    send(kernel_fd, &codigo_respuesta, sizeof(int), 0);

    if (codigo_respuesta == OK) {
        log_debug(memoria_logger, "Dump de memoria exitoso para PID: %d", pid);
    } else {
        log_error(memoria_logger, "Fallo el dump de memoria para PID: %d", pid);
    }
}

void atender_swap(t_buffer* unBuffer, int kernel_fd){
	uint32_t pid = recibir_uint32_del_buffer(unBuffer);

    int resultado = enviar_a_swap(pid);  // 0 si OK, -1 si hubo error

    int codigo_respuesta = OK;
	if(resultado != 0){
		codigo_respuesta = -1;
	}
    send(kernel_fd, &codigo_respuesta, sizeof(int), 0);

    if (codigo_respuesta == OK) {
        log_debug(memoria_logger, "Swap exitoso para PID: %d", pid);
    } else {
        log_error(memoria_logger, "Fallo el envio a swap para PID: %d", pid);
    }
}

void atender_vuelta_swap(t_buffer* unBuffer, int kernel_fd){
	uint32_t pid = recibir_uint32_del_buffer(unBuffer);

	t_proceso* proceso = obtener_proceso_por_id(pid, procesos_swap);
	if(!proceso){
		log_error(memoria_logger, "Error al obtener proceso de lista de procesos en swap");
	}

	uint32_t marcos_libres = cantidad_de_marcos_libres();
	uint32_t espacio_disponible = marcos_libres * ((uint32_t) TAM_PAGINA);

	if(espacio_disponible >= proceso->paginas * TAM_PAGINA){
    	int resultado = sacar_de_swap(pid);  // 0 si OK, -1 si hubo error

		int codigo_respuesta = OK;
		if(resultado != 0){
			codigo_respuesta = -1;
		}
		send(kernel_fd, &codigo_respuesta, sizeof(int), 0);

		if (codigo_respuesta == OK) {
    		log_debug(memoria_logger, "Dump de memoria exitoso para PID: %d", pid);
    	} else {
        	log_error(memoria_logger, "Fallo el dump de memoria para PID: %d", pid);
    	}
	
	} else {
		int respuesta = SIN_ESPACIO;
    	send(kernel_fd, &respuesta, sizeof(int), 0);
		log_debug(memoria_logger, "No hay espacio para proceso PID %d", pid);
	}
}