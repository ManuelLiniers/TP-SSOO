#ifndef MEMORIA_H_
#define MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include</home/utnso/tp-2025-1c-queCompileALaPrimera/utils/socket.h>

char* IP_MEMORIA;
char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_PAGINA;
int ENTRADAS_POR_TABLA;
int CANTIDAD_NIVELES;
int RETARDO_MEMORIA;
char* PATH_SWAPFILE;
int RETARDO_SWAP;
char* LOG_LEVEL;
char* DUMP_PATH;


int server_fd_memoria;

// LOGS Y CONFIG

t_log* memoria_logger;
t_config* memoria_config;
t_config* iniciar_config(void);
void leer_config();
void leer_log();
void recibir_mensaje(int socket_cliente);

#endif /* MEMORIA_H_ */