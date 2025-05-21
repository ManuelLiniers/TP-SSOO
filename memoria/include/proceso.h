#ifndef PROCESO_H_
#define PROCESO_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/socket.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_cpu.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_kernel.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/Memoria.h>

typedef struct{
	int pid;
	int size;
	char* pathInstrucciones;
	t_list* instrucciones;
	t_list* tabla_paginas;
	pthread_mutex_t mutex_TP;
}t_proceso;

#endif /* PROCESO_H_ */