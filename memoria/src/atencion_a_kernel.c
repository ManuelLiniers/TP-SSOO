#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_kernel.h>

void mock_aceptacion(t_buffer* unBuffer){
    uint32_t pid;
	uint32_t tamanio;

	pid = recibir_int_del_buffer(unBuffer);
	tamanio = recibir_int_del_buffer(unBuffer);

    log_info(memoria_logger, "Pedido de crear proceso PID %d con tamaño %d", pid, tamanio);

    int respuesta = OK;
    send(kernel_fd, &respuesta, sizeof(int), 0);
}