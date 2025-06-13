#ifndef INCLUDE_SOCKET_H_
#define INCLUDE_SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <assert.h>
#include "commons.h"

typedef enum
{
	HANDSHAKE,
	IDENTIFICACION,
	MENSAJE,
	PAQUETE,
	INICIAR_PROCESO,
	FINALIZAR_PROCESO,
	PEDIR_INSTRUCCION,
	DEVOLVER_INSTRUCCION,
	ESCRIBIR_MEMORIA,
	LEER_MEMORIA,
	PEDIR_MARCO,
	PETICION_IO,
	INIT_PROC,
	DEVOLVER_CONTEXTO,
	OK,
	SIN_ESPACIO,
	INTERRUPCION_CPU,
	CONTEXTO_PROCESO
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
t_paquete* crear_paquete(op_code codigo_op);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void* serializar_paquete(t_paquete* paquete, int bytes);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
t_buffer* recibir_paquete(int cliente_fd);
void eliminar_paquete(t_paquete* paquete);

// buffer
void crear_buffer(t_paquete* paquete);
void* recibir_buffer(int* size, int socket_cliente);
int recibir_int_del_buffer(t_buffer* unBuffer);
uint32_t recibir_uint32_del_buffer(t_buffer* unBuffer);
char* recibir_string_del_buffer(t_buffer* unBuffer);
void* recibir_informacion_del_buffer(t_buffer* unBuffer, size_t tamanio);
void obtener_del_buffer(t_buffer *buffer, void *dest, int size);
void eliminar_buffer(t_buffer* unBuffer);

void hacer_handshake(t_log* logger, int conexion);
void enviarCodigo(int conexion, int codigoOp);
void saludar_cliente_generico(t_log* logger, void *void_args, void (*funcion_identificacion)(t_buffer*, int));

#endif /* INCLUDE_SOCKET_H_ */