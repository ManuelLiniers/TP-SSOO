#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <readline/readline.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/socket.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_cpu.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/atencion_a_kernel.h>
#include </home/utnso/tp-2025-1c-queCompileALaPrimera/memoria/include/proceso.h>
#include "variables_memoria.h"

/*char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_PAGINA;
int ENTRADAS_POR_TABLA;
int CANTIDAD_NIVELES;
int RETARDO_MEMORIA;
char* PATH_SWAPFILE;
int RETARDO_SWAP;
char* LOG_LEVEL;
char* DUMP_PATH;
char* PATH_INSTRUCCIONES;


int server_fd_memoria;
int fd_kernel;
int fd_cpu;
void* espacio_usuario;

t_list* procesos_memoria;

// LOGS Y CONFIG

t_log* memoria_logger;
t_config* memoria_config;
t_config* iniciar_config(void);*/


/*extern char* PUERTO_ESCUCHA;
extern int TAM_MEMORIA;
extern int TAM_PAGINA;
extern int ENTRADAS_POR_TABLA;
extern int CANTIDAD_NIVELES;
extern int RETARDO_MEMORIA;
extern char* PATH_SWAPFILE;
extern int RETARDO_SWAP;
extern char* LOG_LEVEL;
extern char* DUMP_PATH;
extern char* PATH_INSTRUCCIONES;

extern int server_fd_memoria;
extern int fd_kernel;
extern int fd_cpu;
extern void* espacio_usuario;

extern t_list* procesos_memoria;

extern t_log* memoria_logger;
extern t_config* memoria_config;*/

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