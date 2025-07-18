#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include "utils/socket.h"
#include "variables_memoria.h"
#include "atencion_a_kernel.h"
#include "atencion_a_cpu.h"
#include "proceso.h"
#include "marcos.h"
#include "tablas.h"
#include "lectura_escritura.h"
#include "manejo_swap.h"



void leer_config(t_config* config, t_config* pruebas);
char* inicializar_memoria();
void finalizar_memoria(char* bits);
int servidor_escucha(int server_fd_memoria);
void saludar_cliente(void *void_args);
void identificar_modulo(t_buffer* unBuffer, int cliente_fd);
void atender_kernel(int kernel_fd);
void atender_cpu(int cpu_fd);

#endif /* MEMORIA_H_ */