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
#include</home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/commons.h>

typedef enum
{
	MENSAJE,
	PAQUETE,
	PEDIR_INSTRUCCION,
	DEVOLVER_INSTRUCCION,
} op_code;

typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;



int iniciar_servidor(t_log* logger, char* ip, char* puerto);
int esperar_cliente(t_log* logger, const char* name, int socket_servidor);
int crear_conexion(t_log* logger, char *ip, char* puerto);
void liberar_conexion(int socket_cliente);
void enviar_mensaje(char* mensaje, int socket_cliente);
void recibir_mensaje(t_log* logger, int cliente_fd);
int recibir_operacion(int socket_cliente);

// Paquete
t_paquete* crear_paquete(op_code codigo_op)
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
t_buffer* recibir_paquete(int cliente_fd);
void eliminar_paquete(t_paquete* paquete);
void crear_buffer(t_paquete* paquete)
void* recibir_buffer(int* size, int socket_cliente);

#endif /* INCLUDE_SOCKET_H_ */