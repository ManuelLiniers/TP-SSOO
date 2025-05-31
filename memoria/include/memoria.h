#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include "utils/socket.h"
#include "atencion_a_kernel.h"
#include "atencion_a_cpu.h"
#include "proceso.h"
#include "variables_memoria.h"
#include "marcos.h"


void leer_config();
void leer_log();
void inicializar_memoria();
void finalizar_memoria();
int servidor_escucha(int server_fd_memoria);
void saludar_cliente(void *void_args);
void identificar_modulo(t_buffer* unBuffer, int cliente_fd);
void atender_kernel(int kernel_fd);
void atender_cpu(int cpu_fd);

#endif /* MEMORIA_H_ */