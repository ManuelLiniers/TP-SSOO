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

	uint32_t cantidad_de_marcos_libres = cantidad_de_marcos_libres();
	uint32_t espacio_disponible = cantidad_de_marcos_libres * TAM_PAGINA;

	if(espacio_disponible >= tamanio){
    	int respuesta = OK;
    	send(kernel_fd, &respuesta, sizeof(int), 0);
		log_debug(memoria_logger, "Creando proceso PID %d", pid);

		char* path_instrucciones;
		path_instrucciones = recibir_informacion_del_buffer(unBuffer, sizeof(char*));

		t_proceso* procesoNuevo = crear_proceso(pid, tamanio, path_instrucciones);
	
		log_info(memoria_logger, "## PID: <%d> - Proceso Creado - Tamaño: <%d>", pid, tamanio);

		agregar_proceso_a_lista(procesoNuevo, procesos_memoria);
	} else {
		int respuesta = SIN_ESPACIO;
    	send(kernel_fd, &respuesta, sizeof(int), 0);
		log_debug(memoria_logger, "No hay espacio para proceso PID %d", pid);
	}
}