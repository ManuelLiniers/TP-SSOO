#include "kernel.h"

int main(int argc, char* argv[]) {
    inicializar_kernel();
	inicializar_planificacion();
	inicializar_servidores();

	crear_proceso(argv[1], argv[2]); // Creo proceso inicial con valores recibidos por parametro


    // log_info(logger_kernel,"Iniciando servidor Kernel");
	// int server_fd_kernel_io = iniciar_servidor(logger_kernel, ip_kernel, puerto_kernel); 
	// Si esta ESCUCHANDO en PUERTO KERNEL -> SE CONECTA CON IO PORQUE IO ES CLIENTE NADA MÁS DE ESTE PUERTO. SI ESCUCHA OTRO PUERTO, SE CONECTA A ALGUNO DE CPU

    return EXIT_SUCCESS;
}

void iniciar_config(){
    config_kernel = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/kernel.config");
    if(config_kernel == NULL){
        perror("Error al crear el config del Kernel");
        exit(EXIT_FAILURE);
    }
	log_info(logger_kernel, "Config creada existosamente");
}


void inicializar_kernel(char* instrucciones, char* tamanio_proceso){
    iniciar_config();
	iniciar_semaforos();
	logger_kernel = log_create("kernel.log", "[Kernel]", 1, LOG_LEVEL_INFO);
	
	ip_memoria = config_get_string_value(config_kernel, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
	ip_kernel = config_get_string_value(config_kernel, "IP_KERNEL");
	puerto_kernel = config_get_string_value(config_kernel, "PUERTO_KERNEL");
	puerto_dispatch = config_get_string_value(config_kernel, "PUERTO_ESCUCHA_DISPATCH");
	puerto_interrupt = config_get_string_value(config_kernel, "PUERTO_ESCUCHA_INTERRUPT");
	puerto_io = config_get_string_value(config_kernel, "PUERTO_ESCUCHA_IO");
    
	// int conexion = crear_conexion(logger_kernel, ip_memoria, puerto_memoria);
	// enviar_mensaje("Conexion desde kernel", conexion);
}

void inicializar_planificacion(){

	/* 
	Incializar la planificacion creando las colas y entiendo que crear hilo 
	para cada una de las planificaciones.
	Ver si hacer eso aca o derivarlo en el archivo scheduler.c
	*/

	scheduler_init();

	pthread_t* planificador_corto_plazo = malloc(sizeof(pthread_t));
	pthread_create(planificador_corto_plazo, NULL, planificar_corto_plazo, NULL);

	pthread_t* planificador_largo_plazo = malloc(sizeof(pthread_t));
	pthread_create(planificador_largo_plazo, NULL, planificar_largo_plazo, NULL);
}

void inicializar_servidores(){
	iniciar_dispositivos_io();
	iniciar_cpus();


	pthread_t* servidor_io = malloc(sizeof(pthread_t));
	pthread_create(servidor_io, NULL, (void*) iniciar_servidor_io, NULL);
	pthread_t* cpu_interrupt = malloc(sizeof(pthread_t));
	pthread_create(cpu_interrupt, NULL, (void*) iniciar_cpu_interrupt, NULL);
	pthread_t* cpu_dispatch = malloc(sizeof(pthread_t));
	pthread_create(cpu_dispatch, NULL, (void*) iniciar_cpu_dispatch, NULL);
}

void iniciar_cpu_dispatch(void* arg){
	int server_fd_kernel_dispatch = iniciar_servidor(logger_kernel, NULL, puerto_dispatch);

	while (server_fd_kernel_dispatch != -1)
	{
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_dispatch);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_dispatch, args);
			log_info(logger_kernel, "[THREAD] Creo hilo para atender dispatch");
			pthread_detach(hilo_cliente);
		}
	}
}

void atender_dispatch(void* arg){
	int* io_socket = (int*) arg;
	int socket_fd = *io_socket;
	free(arg);
	
	int code_op = recibir_operacion(socket_fd);
	switch(code_op){
		case HANDSHAKE:
		int respuesta = 1;
			send(socket_fd, &respuesta, sizeof(int), 0);
			int code_op = recibir_operacion(socket_fd);
			if(code_op == IDENTIFICACION){
				t_buffer* buffer = recibir_paquete(socket_fd);
				identificar_cpu(buffer, socket_fd, modificar_interrupt);
				eliminar_buffer(buffer);
			}
			else{
				log_error(logger_kernel, "Esperaba IDENTIFICACION después de HANDSHAKE");
				close(socket_fd);
			}
			break; 
		case -1:
			log_error(logger_kernel, "Desconexion en el HANDSHAKE");
			break;
		default:
			log_error(logger_kernel, "Desconexion en el HANDSHAKE: Operacion Desconocida");
			break;
	}
}

bool comparar_cpu_id(t_cpu* existente, t_cpu* buscada){
	return strcmp(existente->cpu_id, buscada->cpu_id) == 0;
}

t_cpu* cpu_ya_existe(t_list* lista, t_cpu* buscada){
	for(int i = 0; i<list_size(lista); i++){
		t_cpu* actual = (t_cpu*) list_get(lista, i);
		if(comparar_cpu_id(actual, buscada)){
			return actual;
		}
	}
	return NULL;
}

void identificar_cpu(t_buffer* buffer, int socket_fd, void (*funcion)(t_cpu*, int)){
	t_cpu* cpu_nueva = malloc(sizeof(t_cpu));
	strcpy(cpu_nueva->cpu_id, recibir_string_del_buffer(buffer));
	t_cpu* encontrada = cpu_ya_existe(lista_cpus, cpu_nueva);
	if(encontrada == NULL){
		funcion(cpu_nueva, socket_fd);
		list_add(lista_cpus, cpu_nueva);
	}
	else{ 
		funcion(encontrada, socket_fd);
		free(cpu_nueva);
	}
}

void modificar_dispatch(t_cpu* una_cpu, int socket_fd){
	una_cpu->socket_dispatch = socket_fd;
}

void iniciar_cpu_interrupt(void* arg){
	int server_fd_kernel_interrupt = iniciar_servidor(logger_kernel, NULL, puerto_interrupt);

	while (server_fd_kernel_interrupt != -1)
	{
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_interrupt);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_interrupt, args);
			log_info(logger_kernel, "[THREAD] Creo hilo para atender dispatch");
			pthread_detach(hilo_cliente);
		}
	}
}

void atender_interrupt(void* arg){
	int* io_socket = (int*) arg;
	int socket_fd = *io_socket;
	free(arg);
	
	int code_op = recibir_operacion(socket_fd);
	switch(code_op){
		case HANDSHAKE:
		int respuesta = 1;
			send(socket_fd, &respuesta, sizeof(int), 0);
			int code_op = recibir_operacion(socket_fd);
			if(code_op == IDENTIFICACION){
				t_buffer* buffer = recibir_paquete(socket_fd);
				identificar_cpu(buffer, socket_fd, modificar_interrupt);
				eliminar_buffer(buffer);
			}
			else{
				log_error(logger_kernel, "Esperaba IDENTIFICACION después de HANDSHAKE");
				close(socket_fd);
			}
			break; 
		case -1:
			log_error(logger_kernel, "Desconexion en el HANDSHAKE");
			break;
		default:
			log_error(logger_kernel, "Desconexion en el HANDSHAKE: Operacion Desconocida");
			break;
	}
}

void modificar_interrupt(t_cpu* una_cpu, int socket_fd){
	una_cpu->socket_interrupt = socket_fd;
}

void iniciar_servidor_io(void* arg){
	int server_fd_kernel_io = iniciar_servidor(logger_kernel, NULL, puerto_io);

	while(server_fd_kernel_io != -1){
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_io);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_io, args);
			log_info(logger_kernel, "[THREAD] Creo hilo para atender io");
			pthread_detach(hilo_cliente);
		}
	}

}

void atender_io(void* arg){
	int* io_socket = (int*) arg;
	int socket_fd = *io_socket;
	free(arg);
	
	int code_op = recibir_operacion(socket_fd);
	switch(code_op){
		case HANDSHAKE:
		int respuesta = 1;
			send(socket_fd, &respuesta, sizeof(int), 0);
			int code_op = recibir_operacion(socket_fd);
			if(code_op==IDENTIFICACION){
				t_buffer* buffer = recibir_paquete(socket_fd);
				identificar_io(buffer, socket_fd);
				eliminar_buffer(buffer);
			}
			else{
				log_error(logger_kernel, "Esperaba IDENTIFICACION después de HANDSHAKE");
				close(socket_fd);
			}
		case -1:
			log_error(logger_kernel, "Desconexion en el HANDSHAKE");
			break;
		default:
			log_error(logger_kernel, "Desconexion en el HANDSHAKE: Operacion Desconocida");
			break;
	}
}

void identificar_io(t_buffer* unBuffer, int socket_fd){
	t_dispositivo_io* dispositivo = malloc(sizeof(t_dispositivo_io));
	strcpy(dispositivo->nombre, recibir_string_del_buffer(unBuffer));
	dispositivo->id = id_io_incremental;
	id_io_incremental++;
	dispositivo->socket = socket_fd;
	list_add(lista_dispositivos_io, dispositivo);
}