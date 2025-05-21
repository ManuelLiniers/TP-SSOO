#include <atencion_a_cpu.h>

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
			case -1:{
				log_info(memoria_logger, "[CPU] se desconecto. Terminando consulta");
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

	log_info(memoria_logger, "CPU pide instrucción para PID %d, PC %d", pid, pc);

    /*/ Armar path del archivo de instrucciones
    char path[256];
    sprintf(path, "%s/%d.txt", PATH_INSTRUCCIONES, pid);

    FILE* archivo = fopen(path, "r");
    if (!archivo) {
        log_error(memoria_logger, "Archivo de instrucciones para PID %d no encontrado", pid);
        exit(0);
    }
*/
    char instruccion[128];/*
    int instruccion_actual = 0;

    while (fgets(instruccion, sizeof(instruccion), archivo)) {
        if (instruccion_actual == pc) break;
        instruccion_actual++;
    }

    fclose(archivo);

    // Eliminar salto de línea si hay
    instruccion[strcspn(instruccion, "\n")] = '\0';

    log_info(memoria_logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s", pid, pc, instruccion);
	*/

    /*
	t_proceso* un_proceso = obtener_proceso_por_id(pid);

	// Obtener la instruccion
	char* instruccion = obtener_instruccion_por_indice(un_proceso, pc);

	log_info(memoria_logger, "<PID:%d> <PC:%d> <%s>", pid, pc, instruccion)
    */

	enviar_instruccion_a_cpu(instruccion, cpu_fd);
}

void enviar_instruccion_a_cpu(char* instruccion, int cpu_fd){
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