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


t_proceso* crear_proceso(int pid, int size, char* path_instruc);
void agregar_proceso_a_lista(t_proceso* un_proceso ,t_list* una_lista);
t_list* leer_archivo_y_cargar_instrucciones(char* path_archivo);
t_proceso* obtener_proceso_por_id(int pid);
void* obtener_tabla_por_pid(int pid);
char* obtener_instruccion_por_indice(t_proceso* un_proceso, int indice_instruccion);

#endif /* PROCESO_H_ */