#include "kernel.h"


int main(int argc, char* argv[]) {
    inicializar_kernel();
	inicializar_servidores();

	printf("Ingrese ENTER para comenzar la planificacion >> \n");
	getchar(); // bloquea el programa hasta que se ingrese enter

	crear_proceso(argv[1], atoi(argv[2])); // Creo proceso inicial con valores recibidos por parametro
	inicializar_planificacion();

	while(1){
		wait_sem(&bloqueante_sem);
	}

    return EXIT_SUCCESS;
}

void iniciar_config(){
    config_kernel = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/kernel.config");
    if(config_kernel == NULL){
        log_error(logger_kernel, "Error al crear el config del Kernel");
    }
	//config_pruebas = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/corto_plazo.config");
	config_pruebas = config_create("/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/estabilidad_general.config");
	//log_debug(logger_kernel, "Config creada existosamente");
}


void inicializar_kernel(char* instrucciones, char* tamanio_proceso){
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
	log_level = config_get_string_value(config_kernel, "LOG_LEVEL");


	algoritmo_corto_plazo = config_get_string_value(config_pruebas, "ALGORITMO_PLANIFICACION"); // FIFO, SJF, SRT
	algoritmo_largo_plazo = config_get_string_value(config_pruebas, "ALGORITMO_INGRESO_A_READY"); // FIFO, PMCP
	estimacion_inicial = config_get_int_value(config_pruebas, "ESTIMACION_INICIAL");
	estimador_alfa = config_get_double_value(config_pruebas, "ALFA");
	tiempo_suspension = config_get_int_value(config_pruebas, "TIEMPO_SUSPENSION");

	logger_kernel = log_create("kernel.log", "[Kernel]", 1, log_level_from_string(log_level));
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

		log_debug(logger_kernel, "Planificacion corto plazo con SJF");
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_SJF, NULL);
		pthread_detach(planificador_corto_plazo);
	}
	if(strcmp(algoritmo_corto_plazo,"SRT") == 0){

		log_debug(logger_kernel, "Planificacion corto plazo con SRT");
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

	//log_debug(logger_kernel, "Planificacion inicializada");
}

void inicializar_servidores(){
	iniciar_dispositivos_io();
	iniciar_cpus();
	pthread_t cpu_dispatch;
	pthread_t cpu_interrupt;
	pthread_t io;

	pthread_create(&io, NULL, (void*) iniciar_servidor_io, NULL);
	pthread_detach(io);

	pthread_create(&cpu_dispatch, NULL, (void*) iniciar_cpu_dispatch, NULL);
	pthread_detach(cpu_dispatch);

	pthread_create(&cpu_interrupt, NULL, (void*) iniciar_cpu_interrupt, NULL);
	pthread_detach(cpu_interrupt);

	//log_debug(logger_kernel, "servidores iniciados");
}

void* iniciar_cpu_dispatch(void* arg){
	//log_debug(logger_kernel, "entro al hilo dispatch");
	int server_fd_kernel_dispatch = iniciar_servidor(logger_kernel, NULL, puerto_dispatch);

	while (server_fd_kernel_dispatch != -1)
	{
		//log_debug(logger_kernel, "escuchando dispatch");
		int cliente_fd = esperar_cliente(logger_kernel, "Kernel", server_fd_kernel_dispatch);
		if(cliente_fd != -1){
			pthread_t hilo_cliente;
			int *args = malloc(sizeof(int));
			*args = cliente_fd;
			pthread_create(&hilo_cliente, NULL, (void*) atender_dispatch, args);
			//log_debug(logger_kernel, "[THREAD] Creo hilo para atender dispatch");
			pthread_detach(hilo_cliente);
		}
	}
	return NULL;
}

void* atender_dispatch(void* arg){
	saludar_cliente_generico(logger_kernel, arg, (void*) identificar_cpu_distpatch);
	return NULL;
}

void* identificar_cpu_distpatch(t_buffer* buffer, int socket){
	identificar_cpu(buffer, socket, modificar_dispatch);
	return NULL;
}

void* esperar_dispatch(void* arg){
    t_cpu* cpu_encargada = (t_cpu*) arg;
	int respuesta = OK;
	bool ignorar_interrupcion = false;
	int motivo_interrupcion = -1;

    while(1){
		int op_code = recibir_operacion(cpu_encargada->socket_dispatch);
        if(CONTEXTO_PROCESO != op_code){
            log_error(logger_kernel, "Se esperaba recibir CONTEXTO_PROCESO, se recibio: %d", op_code);
            return NULL;
        }
        t_buffer* paquete = recibir_paquete(cpu_encargada->socket_dispatch);

        uint32_t pid = recibir_uint32_del_buffer(paquete);
        t_pcb* proceso = buscar_proceso_pid(pid);
        uint32_t pc = recibir_uint32_del_buffer(paquete);
        int motivo = recibir_int_del_buffer(paquete);
		if(proceso != NULL){
        	proceso->program_counter = pc;
		}

        //log_debug(logger_kernel, "Motivo: %d", motivo);
        
        switch (motivo)
        {
            case INTERRUPCION:
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
				t_unidad_ejecucion* proceso_ejecutando = NULL;
				t_unidad_ejecucion* proceso_a_ejecutar = NULL;
				if(!ignorar_interrupcion || motivo_interrupcion == FINALIZADO || motivo_interrupcion == CAUSA_IO || motivo_interrupcion == MEMORY_DUMP){
					wait_mutex(&mutex_procesos_ejecutando);
					for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
						t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
						if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
							if(unidad->interrumpido == INTERRUMPIDO){
								if(proceso == NULL){
									proceso_ejecutando = NULL;
								} else {
									proceso_ejecutando = unidad;
								}
							} else {
								if(unidad->interrumpido == AEJECUTAR){
									proceso_a_ejecutar = unidad;
								}
							}
						}
					}
					signal_mutex(&mutex_procesos_ejecutando);
					if(motivo_interrupcion != FINALIZADO){
						if(proceso_ejecutando != NULL){
							poner_en_ready(proceso_ejecutando->proceso, true);
							log_info(logger_kernel, "## (<%d>) - Desalojado por algoritmo SJF/SRT", proceso_ejecutando->proceso->pid); 
							log_debug(logger_kernel, " -----------------------------");
							temporal_destroy(proceso_ejecutando->tiempo_ejecutando);
							wait_mutex(&mutex_procesos_ejecutando);
							list_remove_element(lista_procesos_ejecutando, proceso_ejecutando);
							signal_mutex(&mutex_procesos_ejecutando);
//							free(proceso_ejecutando);
						}
						if(proceso_a_ejecutar == NULL){
							log_error(logger_kernel, "ERROR: la unidad de ejecucion es NULL");
						}
						if(proceso_a_ejecutar->proceso == NULL){
				 			log_error(logger_kernel, "ERROR: el proceso a ejecutar es NULL");
						}
					}
				} else {
					wait_mutex(&mutex_procesos_ejecutando);
					for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
						t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
						if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
							if(unidad->interrumpido == INTERRUMPIDO){
								proceso_a_ejecutar = unidad;
							}
						}
					}
					signal_mutex(&mutex_procesos_ejecutando);
				}
				//cpu_encargada->esta_libre = 0;
                poner_en_ejecucion(proceso_a_ejecutar->proceso, cpu_encargada);
				
				//proceso_a_ejecutar->tiempo_ejecutando = temporal_create();
				//cambiar_estado(proceso_a_ejecutar->proceso, EXEC);
				signal_sem(&desalojando);
				ignorar_interrupcion = false;
				motivo_interrupcion = -1;
				//log_debug(logger_kernel, "Proceso removido por hilo principal pid <%d>", proceso->pid);
                break;
            case FINALIZADO:
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
                //log_debug(logger_kernel, "Metricas del proceso %d: %d", proceso->pid, list_size(proceso->metricas_tiempo));

                //log_debug(logger_kernel, "Metricas del proceso (<%d>): %d", proceso->pid, list_size(proceso->metricas_tiempo));
				if(strcmp(algoritmo_corto_plazo,"SJF") == 0){
				wait_mutex(&mutex_procesos_ejecutando);
				for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
					t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
					if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
						if(unidad->interrumpido == INTERRUMPIDO){
							ignorar_interrupcion = true;
							motivo_interrupcion = FINALIZADO;
						}
					}
				}
				signal_mutex(&mutex_procesos_ejecutando);}

				log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "EXIT");
				
				cambiar_estado(proceso, EXIT);

                sacar_proceso_ejecucion(proceso);
                
				finalizar_proceso(proceso);
                
                break;
            case CAUSA_IO:
				log_debug(logger_kernel, "Envio respuesta: %d a cpu ID: %d", respuesta, cpu_encargada->cpu_id);
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
                int tamanio = recibir_int_del_buffer(paquete);
                char* nombre_io = recibir_informacion_del_buffer(paquete, tamanio);
                int io_tiempo = recibir_int_del_buffer(paquete);

                tiempo_en_io* proceso_bloqueado = malloc(sizeof(t_dispositivo_io));
                proceso_bloqueado->pcb = proceso;
                proceso_bloqueado->tiempo = io_tiempo;

				if(strcmp(algoritmo_corto_plazo,"SJF") == 0){
				wait_mutex(&mutex_procesos_ejecutando);
				for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
					t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
					if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
						if(unidad->interrumpido == INTERRUMPIDO){
							ignorar_interrupcion = true;
							motivo_interrupcion = CAUSA_IO;
						}
					}
				}
				signal_mutex(&mutex_procesos_ejecutando);}

				cambiar_estado(proceso, BLOCKED);

                sacar_proceso_ejecucion(proceso);
				

                t_dispositivo_io* dispositivo = buscar_io_libre(nombre_io);
                if(dispositivo != NULL){
                    enviar_proceso_a_io(proceso, dispositivo, io_tiempo);
                }
                else{
                    dispositivo = buscar_io_menos_ocupada(nombre_io);
                }

                if(dispositivo == NULL){
                    log_info(logger_kernel, "No se encontro la IO: %s", nombre_io);
					finalizar_proceso(proceso);
                    break;
                }

                wait_mutex(&mutex_queue_block);
                queue_push(obtener_cola_io(dispositivo->id), proceso_bloqueado);
                signal_mutex(&mutex_queue_block);

                log_debug(logger_kernel, "Proceso <%d> en dispositivo %s: ",proceso->pid ,dispositivo->nombre);

                

                pthread_t comprobacion_suspendido;
                pthread_create(&comprobacion_suspendido, NULL, (void*) comprobar_suspendido, proceso);
                pthread_detach(comprobacion_suspendido);
                break;

            case INIT_PROC:
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
                int tamanio_proceso = recibir_int_del_buffer(paquete);
                int longitud = recibir_int_del_buffer(paquete);
                char* archivo = recibir_informacion_del_buffer(paquete, longitud);

				if(strcmp(algoritmo_corto_plazo,"SJF") == 0){
				wait_mutex(&mutex_procesos_ejecutando);
				for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
					t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
					if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
						if(unidad->interrumpido == INTERRUMPIDO){
							ignorar_interrupcion = true;
						}
					}
				}
				signal_mutex(&mutex_procesos_ejecutando);}

                log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "INIT_PROC");
                
				poner_en_ejecucion(proceso, cpu_encargada);

                crear_proceso(archivo, tamanio_proceso);
                break;
            case MEMORY_DUMP:
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
				cambiar_estado(proceso, BLOCKED);
                bool resultado_dump = paquete_memoria_pid(proceso, DUMP_MEMORY); 

				if(strcmp(algoritmo_corto_plazo,"SJF") == 0){
				wait_mutex(&mutex_procesos_ejecutando);
				for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
					t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
					if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
						if(unidad->interrumpido == INTERRUMPIDO){
							ignorar_interrupcion = true;
							motivo_interrupcion = MEMORY_DUMP;
						}
					}
				}
				signal_mutex(&mutex_procesos_ejecutando);}

                sacar_proceso_ejecucion(proceso);

				
                if(resultado_dump){
                    poner_en_ready(proceso, false);
                } else {
                    cambiar_estado(proceso, EXIT);

                    wait_mutex(&mutex_queue_exit);
                    queue_push(queue_exit, proceso);
                    signal_mutex(&mutex_queue_exit);

                    log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);

                    log_metricas_estado(proceso);
                    paquete_memoria_pid(proceso, FINALIZAR_PROCESO);
                    signal_sem(&espacio_memoria);
                }
                break;
            default:
                log_error(logger_kernel, "Motivo de desalojo desconocido");
                break;
            }
        free(paquete);
    }
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

t_cpu* identificar_cpu(t_buffer* buffer, int socket_fd, void (*funcion)(t_cpu*, int)){
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
		wait_mutex(&mutex_cpu);
		signal_sem(&cpu_libre);
		log_debug(logger_kernel, "Semaforo cpu_libre: %ld", cpu_libre.__align);
		signal_mutex(&mutex_cpu);
		//log_debug(logger_kernel, "Tamanio del nombre: %d", tamanio_nombre);
		log_debug(logger_kernel, "Se identifico la CPU: %d", cpu_nueva->cpu_id);
		mostrar_cpus();
		return cpu_nueva;
	}
	else{ 
		funcion(encontrada, socket_fd);
		free(cpu_nueva);
		mostrar_cpus();

		pthread_t esperar_devolucion;
    	pthread_create(&esperar_devolucion, NULL, esperar_dispatch, (void*) encontrada);
		pthread_detach(esperar_devolucion);
		return encontrada;
	}

}

void modificar_dispatch(t_cpu* una_cpu, int socket_fd){
	una_cpu->socket_dispatch = socket_fd;
}

void* iniciar_cpu_interrupt(void* arg){
	//log_debug(logger_kernel, "entre al hilo interrupt");
	int server_fd_kernel_interrupt = iniciar_servidor(logger_kernel, NULL, puerto_interrupt);

	while (server_fd_kernel_interrupt != -1)
	{
		//log_debug(logger_kernel, "escuchando interrupt");
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
	return NULL;
}

void* atender_interrupt(void* arg){
	saludar_cliente_generico(logger_kernel, arg, (void*) identificar_cpu_interrupt);
	return NULL;
}

void* identificar_cpu_interrupt(t_buffer* buffer, int socket){
	identificar_cpu(buffer, socket, modificar_interrupt);
	return NULL;
}

void modificar_interrupt(t_cpu* una_cpu, int socket_fd){
	una_cpu->socket_interrupt = socket_fd;
}

void* iniciar_servidor_io(void* arg){
	//log_debug(logger_kernel, "entro al hilo IO");
	int server_fd_kernel_io = iniciar_servidor(logger_kernel, NULL, puerto_io);

	while(server_fd_kernel_io != -1){
		//log_debug(logger_kernel, "escuchando io");
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
	return NULL;
}

void* atender_io(void* arg){
	saludar_cliente_generico(logger_kernel, arg, (void*) identificar_io);
	return NULL;
}

void* identificar_io(t_buffer* unBuffer, int socket_fd){
	int tamanio_nombre = recibir_int_del_buffer(unBuffer);
	char* nombre_disp = recibir_informacion_del_buffer(unBuffer, tamanio_nombre);

	t_dispositivo_io* dispositivo = malloc(sizeof(t_dispositivo_io));
	dispositivo->nombre = nombre_disp;
	//memcpy(dispositivo->nombre, nombre_raw, tamanio_nombre);
	dispositivo->id = id_io_incremental;
	dispositivo->socket = socket_fd;

	list_add(lista_dispositivos_io, dispositivo);

	t_queue* cola_io = queue_create();
	list_add_in_index(queue_block, dispositivo->id, cola_io);
	id_io_incremental++;
	signal_sem(&dispositivo_libre);

	pthread_t hilo_escucha_io;
	pthread_create(&hilo_escucha_io, NULL, (void*) escuchar_socket_io, dispositivo);
	pthread_detach(hilo_escucha_io);

	log_debug(logger_kernel,"Se registro dispositivo: %s", dispositivo->nombre);
	return NULL;
}

void* escuchar_socket_io(void* arg){
	t_dispositivo_io* dispositivo = (t_dispositivo_io*) arg;
	while(1){
		int codigo = recibir_operacion(dispositivo->socket);
		switch (codigo){
			case FINALIZACION_IO:
				wait_mutex(&mutex_queue_block);
				t_queue* cola = obtener_cola_io(dispositivo->id);
				tiempo_en_io* proceso = queue_pop(cola);

				
				if(proceso->pcb->estado == SUSP_BLOCKED){
					cambiar_estado(proceso->pcb, SUSP_READY);
					wait_mutex(&mutex_queue_susp_ready);
					queue_push(queue_susp_ready, proceso->pcb);
					signal_mutex(&mutex_queue_susp_ready);
					signal_sem(&nuevo_proceso);
					signal_sem(&planificador_largo_plazo);
					log_debug(logger_kernel, "Signal del planificador_largo_plazo (%ld) por proceso suspendido pid <%d>", 
						planificador_largo_plazo.__align, proceso->pcb->pid);
					if(nuevo_proceso_suspendido_ready.__align < 0){
						signal_sem(&nuevo_proceso_suspendido_ready);
						//log_debug(logger_kernel, "Signal nuevo proceso suspendido a ready");
					}
				}
				else{
					poner_en_ready(proceso->pcb, false);
				}
				signal_mutex(&mutex_queue_block);
				log_info(logger_kernel, "## %d finalizó IO", proceso->pcb->pid);

				comprobar_cola_bloqueados(dispositivo);
				break;
			case -1:
				log_debug(logger_kernel, "[Dispositivo %s IO Desconectado]", dispositivo->nombre);
				wait_mutex(&mutex_queue_block);
				queue_clean_and_destroy_elements(obtener_cola_io(dispositivo->id), finalizar_proceso_io);
				signal_mutex(&mutex_queue_block);
				list_remove_element(lista_dispositivos_io, dispositivo);
				pthread_exit(NULL);
				break;
			default:
				log_warning(logger_kernel, "Operación desconocida de IO");
				break;
		}
	}
	return NULL;
}

// Se crea un proceso y se pushea a new
void crear_proceso(char* instrucciones, int tamanio_proceso){
    t_pcb* pcb_nuevo = pcb_create();
    pcb_nuevo->instrucciones = instrucciones;
    pcb_nuevo->tamanio_proceso = tamanio_proceso;
	pcb_nuevo->estimacion_anterior = estimacion_inicial;
	pcb_nuevo->estimacion_actual = estimacion_inicial;
	pcb_nuevo->metricas_estado[0] = 1;
	t_metricas_estado_tiempo* metrica = malloc(sizeof(t_metricas_estado_tiempo));
	metrica->estado = NEW;
	metrica->tiempo_inicio =  temporal_get_string_time("%H:%M:%S:%MS");
	list_add(pcb_nuevo->metricas_tiempo, metrica);
	log_info(logger_kernel, "## (<%d>) Se crea el proceso - Estado: NEW", pcb_nuevo->pid);

	wait_mutex(&mutex_queue_new);
	list_add(queue_new, pcb_nuevo);
	signal_mutex(&mutex_queue_new);
	signal_sem(&nuevo_proceso);
}

void finalizar_proceso(t_pcb* proceso){

	if(paquete_memoria_pid(proceso, FINALIZAR_PROCESO)){
		cambiar_estado(proceso, EXIT);

		log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);
		
		log_metricas_estado(proceso);
		free(proceso);
		signal_sem(&espacio_memoria);
		signal_sem(&planificador_largo_plazo);
		log_debug(logger_kernel, "Signal del planificador_largo_plazo (%ld) por finalizar proceso pid <%d>", 
			planificador_largo_plazo.__align, proceso->pid);
		log_debug(logger_kernel, "HAY ESPACIO FINALIZACION EN MEMORIA");
	}
	else{
		log_error(logger_kernel, "Error en memoria al finalizar proceso PID: <%d>", proceso->pid);
	}
}

void finalizar_proceso_io(void* arg){
	t_unidad_ejecucion* proceso_io = (t_unidad_ejecucion*) arg;
	finalizar_proceso(proceso_io->proceso);
	free(proceso_io);
}