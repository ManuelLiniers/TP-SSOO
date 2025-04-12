#ifndef INCLUDE_SOCKET_H_
#define INCLUDE_SOCKET_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<assert.h>

int iniciar_servidor(t_log* logger, char* ip, char* puerto);
int esperar_cliente(t_log* logger, const char* name, int socket_servidor);
int crear_conexion(t_log* logger, char *ip, char* puerto);
void liberar_conexion(int socket_cliente);
void enviar_mensaje(char* mensaje, int socket_cliente);

#endif /* INCLUDE_SOCKET_H_ */