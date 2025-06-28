#include "kernel.h"

pthread_t cpu_dispatch;
pthread_t cpu_interrupt;

int main(int argc, char* argv[]) {
    inicializar_kernel();
	inicializar_servidores();

	printf("Ingrese ENTER para comenzar la planificacion >> \n");
	getchar(); // bloquea el programa hasta que se ingrese enter

	crear_proceso(argv[1], argv[2]); // Creo proceso inicial con valores recibidos por parametro
	inicializar_planificacion();

	iniciar_servidor_io();
	
    return EXIT_SUCCESS;
}

void iniciar_config(){
    config_kernel = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/kernel.config");
    if(config_kernel == NULL){
        log_error(logger_kernel, "Error al crear el config del Kernel");
    }
	log_debug(logger_kernel, "Config creada existosamente");
}


void inicializar_kernel(char* instrucciones, char* tamanio_proceso){
	logger_kernel = log_create("kernel.log", "[Kernel]", 1, LOG_LEVEL_DEBUG);
    iniciar_config();
	iniciar_semaforos();
	scheduler_init();
	
	ip_memoria = config_get_string_value(config_kernel, "IP_MEMORIA");
    puerto_memoria = config_get_string_value(config_kernel, "PUERTO_MEMORIA");
	ip_kernel = config_get_string_value(config_kernel, "IP_KERNEL");
	puerto_kernel = config_get_string_value(config_kernel, "PUERTO_KERNEL");
	puerto_dispatch = config_get_string_value(config_kernel, "PUERTO_ESCUCHA_DISPATCH");
	puerto_interrupt = config_get_string_value(config_kernel, "PUERTO_ESCUCHA_INTERRUPT");
	puerto_io = config_get_string_value(config_kernel, "PUERTO_ESCUCHA_IO");
	algoritmo_corto_plazo = config_get_string_value(config_kernel, "ALGORITMO_PLANIFICACION"); // FIFO, SJF, SJFDESALOJO
	algoritmo_largo_plazo = config_get_string_value(config_kernel, "ALGORITMO_INGRESO_A_READY"); // FIFO, PMCP
	estimacion_inicial = config_get_int_value(config_kernel, "ESTIMACION_INICIAL");
	estimador_alfa = config_get_double_value(config_kernel, "ALFA");
	tiempo_suspension = config_get_int_value(config_kernel, "TIEMPO_SUSPENSION");

	log_debug(logger_kernel, "kernel inicializado");
}

void inicializar_planificacion(){

	pthread_t planificador_corto_plazo;
	pthread_t planificador_largo_plazo;
	if(strcmp(algoritmo_corto_plazo,"FIFO") == 0){
		log_debug(logger_kernel, "Planificacion corto plazo con FIFO");
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_FIFO, NULL);
		pthread_detach(planificador_corto_plazo);
	}
	if(strcmp(algoritmo_corto_plazo,"SJF") == 0){

		log_debug(logger_kernel, "Planificacion corto plazo con SJF sin desalojo");
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_SJF, NULL);
		pthread_detach(planificador_corto_plazo);
	}
	if(strcmp(algoritmo_corto_plazo,"SJFDESALOJO") == 0){

		log_debug(logger_kernel, "Planificacion corto plazo con SJF sin desalojo");
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_SJF_desalojo, NULL);
		pthread_detach(planificador_corto_plazo);
	}

	if(strcmp(algoritmo_largo_plazo,"FIFO") == 0){

		log_debug(logger_kernel, "Planificacion largo plazo con FIFO");
		pthread_create(&planificador_largo_plazo, NULL, (void *) planificar_largo_plazo_FIFO, NULL);
		pthread_detach(planificador_largo_plazo);
	}
	if(strcmp(algoritmo_largo_plazo,"PMCP") == 0){

		log_debug(logger_kernel, "Planificacion largo plazo con PMCP");
		pthread_create(&planificador_largo_plazo, NULL, (void *) planificar_largo_plazo_PMCP, NULL);
		pthread_detach(planificador_largo_plazo);
	}

	log_debug(logger_kernel, "Planificacion inicializada");
}

void inicializar_servidores(){
	iniciar_dispositivos_io();
	iniciar_cpus();

	pthread_create(&cpu_dispatch, NULL, (void*) iniciar_cpu_dispatch, NULL);
	pthread_detach(cpu_dispatch);

	pthread_create(&cpu_interrupt, NULL, (void*) iniciar_cpu_interrupt, NULL);
	pthread_detach(cpu_interrupt);

	log_debug(logger_kernel, "servidores iniciados");
}

void iniciar_cpu_dispatch(void* arg){
	log_debug(logger_kernel, "entro al hilo dispatch");
	int server_fd_kernel_dispatch = iniciar_servidor(logger_kernel, NULL, puerto_dispatch);

	while (server_fd_kernel_dispatch != -1)
	{
		log_debug(logger_kernel, "escuchando dispatch");
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_dispatch);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_dispatch, args);
			log_debug(logger_kernel, "[THREAD] Creo hilo para atender dispatch");
			pthread_detach(hilo_cliente);
		}
	}
}

void atender_dispatch(void* arg){
	saludar_cliente_generico(logger_kernel, arg, (void*) identificar_cpu_distpatch);
}

void identificar_cpu_distpatch(t_buffer* buffer, int socket){
	identificar_cpu(buffer, socket, modificar_dispatch);
}


bool comparar_cpu_id(t_cpu* existente, t_cpu* buscada){
	return existente->cpu_id == buscada->cpu_id;
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
	int id = recibir_int_del_buffer(buffer);
	//void* nombre_raw = recibir_informacion_del_buffer(buffer, tamanio_nombre);
	//memcpy(cpu_nueva->cpu_id, cpu_id, tamanio_nombre);
	t_cpu* cpu_nueva = malloc(sizeof(t_cpu));
	cpu_nueva->cpu_id = id;
	t_cpu* encontrada = cpu_ya_existe(lista_cpus, cpu_nueva);
	if(encontrada == NULL){
		cpu_nueva->esta_libre = 1;
		funcion(cpu_nueva, socket_fd);
		list_add(lista_cpus, cpu_nueva);
		//log_debug(logger_kernel, "Tamanio del nombre: %d", tamanio_nombre);
		log_debug(logger_kernel, "Se identifico la CPU: %d", cpu_nueva->cpu_id);
	}
	else{ 
		funcion(encontrada, socket_fd);
		free(cpu_nueva);
	}
	mostrar_cpus();
}

void modificar_dispatch(t_cpu* una_cpu, int socket_fd){
	una_cpu->socket_dispatch = socket_fd;
}

void iniciar_cpu_interrupt(void* arg){
	log_debug(logger_kernel, "entre al hilo interrupt");
	int server_fd_kernel_interrupt = iniciar_servidor(logger_kernel, NULL, puerto_interrupt);

	while (server_fd_kernel_interrupt != -1)
	{
		log_debug(logger_kernel, "escuchando interrupt");
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_interrupt);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_interrupt, args);
			log_debug(logger_kernel, "[THREAD] Creo hilo para atender interrupt");
			pthread_detach(hilo_cliente);
		}
	}
}

void atender_interrupt(void* arg){
	saludar_cliente_generico(logger_kernel, arg, (void*) identificar_cpu_interrupt);
}

void identificar_cpu_interrupt(t_buffer* buffer, int socket){
	identificar_cpu(buffer, socket, modificar_interrupt);
}

void modificar_interrupt(t_cpu* una_cpu, int socket_fd){
	una_cpu->socket_interrupt = socket_fd;
}

void iniciar_servidor_io(){
	log_debug(logger_kernel, "entro al hilo IO");
	int server_fd_kernel_io = iniciar_servidor(logger_kernel, NULL, puerto_io);

	while(server_fd_kernel_io != -1){
		log_debug(logger_kernel, "escuchando io");
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_io);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_io, args);
			log_debug(logger_kernel, "[THREAD] Creo hilo para atender io");
			pthread_detach(hilo_cliente);
		}
	}

}

void atender_io(void* arg){
	saludar_cliente_generico(logger_kernel, arg, (void*) identificar_io);
}

void identificar_io(t_buffer* unBuffer, int socket_fd){
	int tamanio_nombre = recibir_int_del_buffer(unBuffer);
	void* nombre_raw = recibir_informacion_del_buffer(unBuffer, tamanio_nombre);

	t_dispositivo_io* dispositivo = malloc(sizeof(t_dispositivo_io));
	memcpy(dispositivo->nombre, nombre_raw, tamanio_nombre);
	dispositivo->id = id_io_incremental;
	dispositivo->socket = socket_fd;

	list_add(lista_dispositivos_io, dispositivo);

	t_queue* cola_io = queue_create();
	list_add_in_index(queue_block, id_io_incremental, (void*) cola_io);

	id_io_incremental++;
	log_debug(logger_kernel,"Se registro dispositivo: %s", dispositivo->nombre);
}

// Se crea un proceso y se pushea a new
void crear_proceso(char* instrucciones, char* tamanio_proceso){
    t_pcb* pcb_nuevo = pcb_create();
    pcb_nuevo->instrucciones = instrucciones;
    pcb_nuevo->tamanio_proceso = atoi(tamanio_proceso);
	pcb_nuevo->estimacion_anterior = estimacion_inicial;
	pcb_nuevo->estimacion_actual = estimacion_inicial;
	pcb_nuevo->estado = NEW;
	log_info(logger_kernel, "## (<%d>) Se crea el proceso - Estado: NEW", pcb_nuevo->pid);

	wait_mutex(&mutex_queue_new);
	list_add(queue_new, pcb_nuevo);
	signal_mutex(&mutex_queue_new);
	signal_sem(&nuevo_proceso);
}

