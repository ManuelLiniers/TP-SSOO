#include "/home/utnso/tp-2025-1c-queCompileALaPrimera/utils/socket.h"

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
    log_trace(logger, "Listo para escuchar");
	
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

	freeaddrinfo(server_info);

	return socket_cliente;
}

void liberar_conexion(int socket_cliente)
{
	close(socket_cliente);
}