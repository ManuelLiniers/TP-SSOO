//#include "/home/utnso/tp-2025-1c-queCompileALaPrimera/utils/socket.h"
#include</home/utnso/tp-2025-1c-queCompileALaPrimera/utils/src/utils/socket.h>

int iniciar_servidor(t_log* logger, char* ip, char* puerto)
{

	if(puerto == NULL) {
		log_error(logger, "No se encuentra el puerto");
		return -1;
	}

	struct addrinfo hints, *server_info;

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;    // Acepta IPv4 o IPv6
	hints.ai_flags = AI_PASSIVE;	// Indica que se bindea como servidor (escucha)
	hints.ai_socktype = SOCK_STREAM;  // Indica que se va a usar el protocolo TCP

	getaddrinfo(ip, puerto, &hints, &server_info);  // Si IP es NULL, usa el localhost

//  creamos socket de escucha del servidor
	int server_socket = socket(server_info->ai_family, server_info->ai_socktype,server_info->ai_protocol);

//  asociamos el socket a un puerto
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(1));

	if(server_socket == -1 || bind(server_socket, server_info->ai_addr, server_info->ai_addrlen) == -1){
		freeaddrinfo(server_info);
		log_error(logger, "Fallo el bindeo");
		return -1;
	}

//  escuchamos conexiones entrantes
    if(listen(server_socket, SOMAXCONN) == -1){
		log_error(logger, "fallo el listen");
		return -1;
	}

    freeaddrinfo(server_info);
    log_info(logger, "Listo para escuchar");
	
	return server_socket;
}

int esperar_cliente(t_log* logger, const char* name, int socket_servidor) {
    

//  acepta nuevo cliente
    int socket_cliente = accept(socket_servidor, (void*) NULL, NULL);
    if (socket_cliente == -1) {
        log_error(logger, "Error al aceptar cliente");
        return -1;
    }

    log_info(logger, "Cliente conectado a %s", name);

    return socket_cliente;
}

int crear_conexion(t_log* logger, char *ip, char* puerto)
{
	struct addrinfo hints, *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

//  ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype,  server_info->ai_protocol);

//  ahora que tenemos el socket, vamos a conectarlo
    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        log_error(logger, "Error al conectar al servidor");
        
        close(socket_cliente);
        freeaddrinfo(server_info);
        return -1;
    }

	log_info(logger, "Se conecto exitosamente \n");

	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}

void enviar_mensaje(char* mensaje, int socket_cliente)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2*sizeof(int);

	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

void recibir_mensaje(t_log* logger, int socket_cliente)
{
	int size;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}


/*
**************************************************************************************
************************************PAQUETE*******************************************
**************************************************************************************
*/

t_paquete* crear_paquete(op_code codigo_op)
{
	t_paquete* paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = codigo_op;
	crear_buffer(paquete);
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio)
{
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void* serializar_paquete(t_paquete* paquete, int bytes)
{
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente)
{
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

t_buffer* recibir_paquete(int cliente_fd){
	t_buffer* unBuffer = malloc(sizeof(t_buffer));
	int size;
	unBuffer->stream = recibir_buffer(&size,cliente_fd);
	unBuffer->size = size;
	return unBuffer;
}

void eliminar_paquete(t_paquete* paquete)
{
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

/*
**************************************************************************************
*************************************BUFFER*******************************************
**************************************************************************************
*/

void crear_buffer(t_paquete* paquete)
{
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = 0;
	paquete->buffer->stream = NULL;
}

void* recibir_buffer(int* size, int socket_cliente)
{
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

int recibir_int_del_buffer(t_buffer* unBuffer){
	if(unBuffer->size == 0){
		printf("\n[ERROR] Al intentar extraer un INT de un t_buffer vacio\n\n");
		exit(EXIT_FAILURE);
	}

	if(unBuffer->size < 0){
		printf("\n[ERROR] Esto es raro. El t_buffer contiene un size NEGATIVO \n\n");
		exit(EXIT_FAILURE);
	}

	int valor_a_devolver;
	memcpy(&valor_a_devolver, unBuffer->stream, sizeof(int));

	int nuevo_size = unBuffer->size - sizeof(int);
	if(nuevo_size == 0){
		free(unBuffer->stream);
		unBuffer->stream = NULL;
		unBuffer->size = 0;
		return valor_a_devolver;
	}
	if(nuevo_size < 0){
		printf("\n[ERROR_INT]: BUFFER CON TAMAÑO NEGATIVO\n\n");
		//free(valor_a_devolver);
		//return 0;
		exit(EXIT_FAILURE);
	}
	void* nuevo_unBuffer = malloc(nuevo_size);
	memcpy(nuevo_unBuffer, unBuffer->stream + sizeof(int), nuevo_size);
	free(unBuffer->stream);
	unBuffer->stream = nuevo_unBuffer;
	unBuffer->size = nuevo_size;

	return valor_a_devolver;
}