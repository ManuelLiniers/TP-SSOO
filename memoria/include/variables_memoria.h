#ifndef VARIABLES_MEMORIA_H_
#define VARIABLES_MEMORIA_H_

#include <commons/log.h>
#include <commons/config.h>
#include <commons/collections/list.h>

// Variables de configuración
extern char* PUERTO_ESCUCHA;
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

// Conexiones
extern int server_fd_memoria;
extern int kernel_fd;
extern int cpu_fd;

// Recursos de ejecución
extern void* espacio_usuario;
extern t_list* procesos_memoria;

// Logs y config
extern t_log* memoria_logger;
extern t_config* memoria_config;

// Paginas
extern t_list* lst_marcos;

#endif
