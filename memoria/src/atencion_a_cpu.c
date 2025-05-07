#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_cpu.h>

void atender_peticion_de_instruccion(t_buffer* unBuffer){
    uint32_t pid;
	uint32_t pc;

	pid = recibir_int_del_buffer(unBuffer);
	pc = recibir_int_del_buffer(unBuffer);

	log_info(memoria_logger, "CPU pide instrucción para PID %d, PC %d", pid, pc);

    // Armar path del archivo de instrucciones
    char path[256];
    sprintf(path, "%s/%d.txt", PATH_INSTRUCCIONES, pid);

    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        log_error(memoria_logger, "Archivo de instrucciones para PID %d no encontrado", pid);
        exit(0);
    }

    char instruccion[128];
    int instruccion_actual = 0;

    while (fgets(instruccion, sizeof(instruccion), archivo)) {
        if (instruccion_actual == pc) break;
        instruccion_actual++;
    }

    fclose(archivo);

    // Eliminar salto de línea si hay
    instruccion[strcspn(instruccion, "\n")] = '\0';

    log_info(memoria_logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);

    /*
	t_proceso* un_proceso = obtener_proceso_por_id(pid);

	// Obtener la instruccion
	char* instruccion = obtener_instruccion_por_indice(un_proceso, pc);

	log_info(memoria_logger, "<PID:%d> <PC:%d> <%s>", pid, pc, instruccion)
    */

	enviar_instruccion_a_cpu(instruccion);
}

void enviar_una_instruccion_a_cpu(char* instruccion){
    // Enviar op_code primero
	op_code codigo = DEVOLVER_INSTRUCCION;
	send(cpu_fd, &codigo, sizeof(op_code), 0);

	// Enviar tamaño
	uint32_t size = strlen(instruccion) + 1;
	send(cpu_fd, &size, sizeof(uint32_t), 0);

	// Enviar instrucción
	send(cpu_fd, instruccion, size, 0);

	log_info(memoria_logger, "Instrucción enviada: %s", instruccion);
}