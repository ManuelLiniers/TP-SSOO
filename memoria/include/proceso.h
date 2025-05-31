#ifndef PROCESO_H_
#define PROCESO_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include "utils/socket.h"
#include "atencion_a_cpu.h"
#include "atencion_a_kernel.h"
#include "memoria.h"

typedef struct {
    int accesos_tablas_paginas;
    int instrucciones_solicitadas;
    int bajadas_swap;
    int subidas_memoria;
    int lecturas_memoria;
    int escrituras_memoria;
} t_metricas_proceso;

typedef struct{
	int pid;
	int size;
	t_metricas_proceso metricas;
	char* pathInstrucciones;
	t_list* instrucciones;
	// t_list* tabla_paginas;
	// pthread_mutex_t mutex_TP;
}t_proceso;



t_proceso* crear_proceso(int pid, int size, char* path_instruc);
void agregar_proceso_a_lista(t_proceso* un_proceso ,t_list* una_lista);
t_list* leer_archivo_y_cargar_instrucciones(char* path_archivo);
t_proceso* obtener_proceso_por_id(int pid);
char* obtener_instruccion_por_indice(t_proceso* un_proceso, int indice_instruccion);

#endif /* PROCESO_H_ */