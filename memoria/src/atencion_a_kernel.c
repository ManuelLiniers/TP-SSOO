#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_kernel.h>


void atender_kernel(int kernel_fd){ // agregar que reciba el buffer

	log_info(memoria_logger, "## Kernel Conectado - FD del socket: %d", kernel_fd);

	while (1) {
		int op_code = recibir_operacion(kernel_fd); // sugiero cambiar recibir_operacion(kernel_fd) por recibir_int_del_buffer(buffer)
		t_buffer* unBuffer;

		switch (op_code) {
			case INICIAR_PROCESO: {
				unBuffer = recibir_paquete(kernel_fd);
				// mock de aceptacion
				mock_aceptacion(unBuffer, kernel_fd);

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
		eliminar_buffer(unBuffer);
	}
}

void mock_aceptacion(t_buffer* unBuffer, int kernel_fd){
    uint32_t pid;
	uint32_t tamanio;

	pid = recibir_int_del_buffer(unBuffer);
	tamanio = recibir_int_del_buffer(unBuffer);

    log_info(memoria_logger, "Pedido de crear proceso PID %d con tamaño %d", pid, tamanio);

    int respuesta = OK;
    send(kernel_fd, &respuesta, sizeof(int), 0);
}