#ifndef PROCESO_H_
#define PROCESO_H_

#include</home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/commons.h>

typedef struct{
	int pid;
	int size;
	char* pathInstrucciones;
	t_list* instrucciones;
	t_list* tabla_paginas;
	//pthread_mutex_t mutex_TP;
}t_proceso;


#endif /* PROCESO_H_ */