#include "kernel.h"


int main(int argc, char* argv[]) {
	iniciar_config(argv[3]);
    inicializar_kernel();
	inicializar_servidores();

	printf("Ingrese ENTER para comenzar la planificacion >> \n");
	getchar(); // bloquea el programa hasta que se ingrese enter

	wait_mutex(&mutex_queue_new);
	crear_proceso(argv[1], atoi(argv[2])); // Creo proceso inicial con valores recibidos por parametro
	signal_mutex(&mutex_queue_new);
	inicializar_planificacion();

	while(1){
		wait_sem(&bloqueante_sem);
	}

    return EXIT_SUCCESS;
}

void iniciar_config(char* pruebas){
    config_kernel = config_create("kernel.config");
    if(config_kernel == NULL){
        log_error(logger_kernel, "Error al crear el config del Kernel");
    }
	//char* base_path = "/home/utnso/tp-2025-1c-queCompileALaPrimera/kernel/";
	//char* resultado = malloc(strlen(base_path) + strlen(pruebas) + 1);
	//strcpy(resultado, base_path);
	//strcat(resultado, pruebas);

	config_pruebas = config_create(pruebas);
	if (config_pruebas == NULL) {
    	log_error(logger_kernel, "config_kernel es NULL");
    	exit(EXIT_FAILURE);
	}	
//	free(resultado);
}


void inicializar_kernel(char* instrucciones, char* tamanio_proceso){
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
		wait_mutex(&mutex_creacion_hilos);
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_FIFO, NULL);
		signal_mutex(&mutex_creacion_hilos);
		pthread_detach(planificador_corto_plazo);
	}
	if(strcmp(algoritmo_corto_plazo,"SJF") == 0){

		log_debug(logger_kernel, "Planificacion corto plazo con SJF");
		wait_mutex(&mutex_creacion_hilos);
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_SJF, NULL);
		signal_mutex(&mutex_creacion_hilos);
		pthread_detach(planificador_corto_plazo);
	}
	if(strcmp(algoritmo_corto_plazo,"SRT") == 0){

		log_debug(logger_kernel, "Planificacion corto plazo con SRT");
		wait_mutex(&mutex_creacion_hilos);
		pthread_create(&planificador_corto_plazo, NULL, (void*) planificar_corto_plazo_SJF_desalojo, NULL);
		signal_mutex(&mutex_creacion_hilos);
		pthread_detach(planificador_corto_plazo);
	}

	if(strcmp(algoritmo_largo_plazo,"FIFO") == 0){

		log_debug(logger_kernel, "Planificacion largo plazo con FIFO");
		wait_mutex(&mutex_creacion_hilos);
		pthread_create(&planificador_largo_plazo, NULL, (void *) planificar_largo_plazo_FIFO, NULL);
		signal_mutex(&mutex_creacion_hilos);
		pthread_detach(planificador_largo_plazo);
	}
	if(strcmp(algoritmo_largo_plazo,"PMCP") == 0){

		log_debug(logger_kernel, "Planificacion largo plazo con PMCP");
		wait_mutex(&mutex_creacion_hilos);
		pthread_create(&planificador_largo_plazo, NULL, (void *) planificar_largo_plazo_PMCP, NULL);
		signal_mutex(&mutex_creacion_hilos);
		pthread_detach(planificador_largo_plazo);
	}

}

void inicializar_servidores(){
	iniciar_dispositivos_io();
	iniciar_cpus();
	pthread_t cpu_dispatch;
	pthread_t cpu_interrupt;
	pthread_t io;

	pthread_mutex_lock(&mutex_creacion_hilos);
	pthread_create(&io, NULL, (void*) iniciar_servidor_io, NULL);
	pthread_create(&cpu_dispatch, NULL, (void*) iniciar_cpu_dispatch, NULL);
	pthread_create(&cpu_interrupt, NULL, (void*) iniciar_cpu_interrupt, NULL);
	pthread_mutex_unlock(&mutex_creacion_hilos);
	
	pthread_detach(io);
	pthread_detach(cpu_dispatch);
	pthread_detach(cpu_interrupt);
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
			pthread_mutex_lock(&mutex_creacion_hilos);
			pthread_create(&hilo_cliente, NULL, (void*) atender_dispatch, args);
			pthread_mutex_unlock(&mutex_creacion_hilos);
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

    while(1){
		log_debug(logger_kernel, "Me quedo esperando dispatch de CPU ID: %d", cpu_encargada->cpu_id);
		int op_code = recibir_operacion(cpu_encargada->socket_dispatch);
        if(CONTEXTO_PROCESO != op_code){
            log_error(logger_kernel, "Se esperaba recibir CONTEXTO_PROCESO, se recibio: %d", op_code);
            return NULL;
        }
		else{
			log_debug(logger_kernel, "Recibi CONTEXTO_PROCESO de CPU ID: %d", cpu_encargada->cpu_id);
		}
        t_buffer* paquete = recibir_paquete(cpu_encargada->socket_dispatch);
		log_debug(logger_kernel, "Recibi el paquete de la cpu ID: %d", cpu_encargada->cpu_id);
        uint32_t pid = recibir_uint32_del_buffer(paquete);
        uint32_t pc = recibir_uint32_del_buffer(paquete);
        int motivo = recibir_int_del_buffer(paquete);
		log_debug(logger_kernel, "Recibo motivo: %d", motivo);
		t_pcb* proceso = NULL;
		//log_debug(logger_kernel, "Estado antes de buscar proceso<%d>: ignorar_interrupcion = %d, motivo_interrupcion = %d", cpu_encargada->cpu_id, ignorar_interrupcion, motivo_interrupcion);
		//if(motivo_interrupcion != 1 && motivo_interrupcion != 2 && motivo_interrupcion != 3){
		wait_mutex(&mutex_procesos_ejecutando);
		proceso = buscar_proceso_pid(pid);
		signal_mutex(&mutex_procesos_ejecutando);
		//log_debug(logger_kernel, "Proceso recibido por cpu PID <%d>", proceso->pid);
		if(proceso != NULL){
			proceso->program_counter = pc;
		}

        log_debug(logger_kernel, "Envio respuesta: %d a cpu ID: %d", respuesta, cpu_encargada->cpu_id);
        switch (motivo)
        {
            case INTERRUPCION:
				t_unidad_ejecucion* proceso_ejecutando = NULL;
				t_unidad_ejecucion* proceso_a_ejecutar = NULL;
				wait_mutex(&mutex_queue_ready);
				wait_mutex(&mutex_lista_cpus);
				wait_mutex(&mutex_procesos_ejecutando);
				wait_mutex(&mutex_pcb);
				//log_debug(logger_kernel, "Estado durante el checkeo<%d>: ignorar_interrupcion = %d, motivo_interrupcion = %d",cpu_encargada->cpu_id, ignorar_interrupcion, motivo_interrupcion);
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
				if(proceso_ejecutando != NULL){
					poner_en_ready(proceso_ejecutando->proceso, true);
					log_info(logger_kernel, "## (<%d>) - Desalojado por algoritmo SJF/SRT", proceso_ejecutando->proceso->pid); 
					log_debug(logger_kernel, " -----------------------------");
					temporal_destroy(proceso_ejecutando->tiempo_ejecutando);
					
					list_remove_element(lista_procesos_ejecutando, proceso_ejecutando);
				}
				if(proceso_a_ejecutar == NULL){
					log_error(logger_kernel, "ERROR: la unidad de ejecucion es NULL");
				}
				if(proceso_a_ejecutar->proceso == NULL){
					log_error(logger_kernel, "ERROR: el proceso a ejecutar es NULL");
				}
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
				//send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
				//cpu_encargada->esta_libre = 0;
                poner_en_ejecucion(proceso_a_ejecutar->proceso, cpu_encargada);
				proceso_a_ejecutar->tiempo_ejecutando = temporal_create();
				cambiar_estado(proceso_a_ejecutar->proceso, EXEC);
				proceso_a_ejecutar->interrumpido = EJECUTANDO;
				signal_mutex(&mutex_pcb);
				signal_mutex(&mutex_queue_ready);
				signal_mutex(&mutex_lista_cpus);
				signal_mutex(&mutex_procesos_ejecutando);
				signal_mutex(&desalojando);
				//log_debug(logger_kernel, "Proceso removido por hilo principal pid <%d>", proceso->pid);
                break;
            case FINALIZADO:
                //log_debug(logger_kernel, "Metricas del proceso %d: %d", proceso->pid, list_size(proceso->metricas_tiempo));

                //log_debug(logger_kernel, "Metricas del proceso (<%d>): %d", proceso->pid, list_size(proceso->metricas_tiempo));
				wait_mutex(&mutex_queue_exit);
				wait_mutex(&mutex_lista_cpus);
				wait_mutex(&mutex_procesos_ejecutando);
				wait_mutex(&mutex_pcb);
			
				/* if(strcmp(algoritmo_corto_plazo,"SRT") == 0){
					for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
						t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
						if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
							if(unidad->interrumpido == INTERRUMPIDO){
								ignorar_interrupcion = true;
								motivo_interrupcion = 1;
							}
						}
					}
					int valorDesalojo = pthread_mutex_trylock(&desalojando);
					if(!ignorar_interrupcion && valorDesalojo != 0){
						signal_mutex(&desalojando);
					} else if(valorDesalojo == 0){
						signal_mutex(&desalojando);
					}
				} */


				log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "EXIT");

				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
                sacar_proceso_ejecucion(proceso);
				//signal_sem(&espacio_memoria);
                
				finalizar_proceso(proceso);
				
				signal_mutex(&mutex_queue_exit);
				signal_mutex(&mutex_pcb);
				signal_mutex(&mutex_lista_cpus);
				signal_mutex(&mutex_procesos_ejecutando);
                
                break;
            case CAUSA_IO:
				
                int tamanio = recibir_int_del_buffer(paquete);
                char* nombre_io = recibir_informacion_del_buffer(paquete, tamanio);
                int io_tiempo = recibir_int_del_buffer(paquete);

				if(proceso == NULL){
					log_debug(logger_kernel, "Proceso nulo al enviar a IO");
				} else {
                tiempo_en_io* proceso_bloqueado = malloc(sizeof(tiempo_en_io));
                proceso_bloqueado->pcb = proceso;
                proceso_bloqueado->tiempo = io_tiempo;

				//log_debug(logger_kernel, "Estado antes de CAUSA_IO<%d>: ignorar_interrupcion = %d, motivo_interrupcion = %d",cpu_encargada->cpu_id, ignorar_interrupcion, motivo_interrupcion);

				wait_mutex(&mutex_queue_block);
				wait_mutex(&mutex_lista_cpus);
				wait_mutex(&mutex_procesos_ejecutando);
				wait_mutex(&mutex_lista_dispositivos_io);
				wait_mutex(&mutex_pcb);
				wait_mutex(&mutex_diccionario_esperando_io);
				wait_mutex(&mutex_diccionario_ejecutando_io);

 				/* if(strcmp(algoritmo_corto_plazo,"SRT") == 0){
					for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
						t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
						if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
							if(unidad->interrumpido == INTERRUMPIDO){
								ignorar_interrupcion = true;
								motivo_interrupcion = 2;
								//log_debug(logger_kernel, "Seteo ignorar interrupcion por IO al PID <%d>", unidad->proceso->pid);
							}
						}
					}
					int valorDesalojo = pthread_mutex_trylock(&desalojando);
					if(!ignorar_interrupcion && valorDesalojo != 0){
						signal_mutex(&desalojando);
					} else if(valorDesalojo == 0){
						signal_mutex(&desalojando);
					}
				} */

				//log_debug(logger_kernel, "Estado despues de CAUSA_IO<%d>: ignorar_interrupcion = %d, motivo_interrupcion = %d",cpu_encargada->cpu_id, ignorar_interrupcion, motivo_interrupcion);


				cambiar_estado(proceso, BLOCKED);
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);

                sacar_proceso_ejecucion(proceso);
				
                t_dispositivo_io* dispositivo = buscar_io_libre(nombre_io);

				if(dispositivo != NULL){
					t_list* lista = obtener_ejecutando_io(dispositivo->nombre);
					proceso_bloqueado->dispositivo = dispositivo;
					list_add(lista, proceso_bloqueado);
					//log_debug(logger_kernel, "Proceso <%d> bloqueado agregado en posicion %d", proceso_bloqueado->pcb->pid, pos);
					enviar_proceso_a_io(proceso, dispositivo, io_tiempo);
					dispositivo->esta_libre = false;
				} else {
					dispositivo = buscar_io(nombre_io);
                	if(dispositivo == NULL){
                    	log_info(logger_kernel, "No se encontro la IO: %s", nombre_io);
						finalizar_proceso(proceso);
						signal_mutex(&mutex_pcb);
						signal_mutex(&mutex_lista_cpus);
						signal_mutex(&mutex_procesos_ejecutando);
						signal_mutex(&mutex_lista_dispositivos_io);
						signal_mutex(&mutex_queue_block);
						signal_mutex(&mutex_diccionario_ejecutando_io);
						signal_mutex(&mutex_diccionario_esperando_io);
                    	break;
                	} else {
						t_queue* cola_io = obtener_esperando_io(nombre_io);
						proceso_bloqueado->dispositivo = dispositivo;
						queue_push(cola_io, proceso_bloqueado);
					}
				}
				

				//log_info(logger_kernel, "Cola de IO: (%s, %d) - Cantidad de procesos en cola: %d",
				//dispositivo->nombre, dispositivo->id, queue_size(obtener_esperando_io(dispositivo->id)));

                log_debug(logger_kernel, "Proceso <%d> en dispositivo %s: ",proceso->pid ,dispositivo->nombre);
                

				//log_debug(logger_kernel, "Creo hilo para comprobar suspendido");
				signal_mutex(&mutex_pcb);
				signal_mutex(&mutex_lista_cpus);
				signal_mutex(&mutex_procesos_ejecutando);
				signal_mutex(&mutex_lista_dispositivos_io);
				signal_mutex(&mutex_queue_block);
				signal_mutex(&mutex_diccionario_ejecutando_io);
				signal_mutex(&mutex_diccionario_esperando_io);

				pthread_t comprobacion_suspendido;
				wait_mutex(&mutex_creacion_hilos);
                pthread_create(&comprobacion_suspendido, NULL, (void*) comprobar_suspendido, proceso);
				signal_mutex(&mutex_creacion_hilos);
                pthread_detach(comprobacion_suspendido);
				}

                break;

            case INIT_PROC:
				
                int tamanio_proceso = recibir_int_del_buffer(paquete);
                int longitud = recibir_int_del_buffer(paquete);
                char* archivo = recibir_informacion_del_buffer(paquete, longitud);

				wait_mutex(&mutex_queue_new);
				wait_mutex(&mutex_lista_cpus);
				wait_mutex(&mutex_procesos_ejecutando);
				wait_mutex(&mutex_pcb);
				bool hay_interrupcion = false;
				log_debug(logger_kernel, "Pase los mutex");
				if(strcmp(algoritmo_corto_plazo,"SRT") == 0){
					for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
						t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
						if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
							if(unidad->interrumpido == INTERRUMPIDO){
								hay_interrupcion = true;
							}
						}
					}
				}

				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);

                log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "INIT_PROC");
                
				// wait_mutex(&mutex_lista_cpus);
				// wait_mutex(&mutex_procesos_ejecutando);
				if(!hay_interrupcion){
					poner_en_ejecucion(proceso, cpu_encargada);
				}
				
				crear_proceso(archivo, tamanio_proceso);

				signal_mutex(&mutex_pcb);
				signal_mutex(&mutex_queue_new);
				signal_mutex(&mutex_lista_cpus);
				signal_mutex(&mutex_procesos_ejecutando);
				log_debug(logger_kernel, "Libere los mutex");
                break;
            case MEMORY_DUMP:
				wait_mutex(&mutex_queue_ready);
                wait_mutex(&mutex_queue_exit);
				wait_mutex(&mutex_lista_cpus);
				wait_mutex(&mutex_procesos_ejecutando);
				wait_mutex(&mutex_pcb);

				/* if(strcmp(algoritmo_corto_plazo,"SRT") == 0){
					for(int i = 0; i<list_size(lista_procesos_ejecutando); i++){
						t_unidad_ejecucion* unidad = (t_unidad_ejecucion*) list_get(lista_procesos_ejecutando, i);
						if(unidad->cpu->cpu_id == cpu_encargada->cpu_id){
							if(unidad->interrumpido == INTERRUMPIDO){
								ignorar_interrupcion = true;
								motivo_interrupcion = 3;
							}
						}
					}
					int valorDesalojo = pthread_mutex_trylock(&desalojando);
					if(!ignorar_interrupcion && valorDesalojo != 0){
						signal_mutex(&desalojando);
					} else if(valorDesalojo == 0){
						signal_mutex(&desalojando);
					}
				} */

                sacar_proceso_ejecucion(proceso);
				send(cpu_encargada->socket_dispatch, &respuesta, sizeof(int), 0);
				cambiar_estado(proceso, BLOCKED);
                bool resultado_dump = paquete_memoria_pid(proceso, DUMP_MEMORY); 
				
				
                if(resultado_dump){
                    poner_en_ready(proceso, false);
                } else {
					log_debug(logger_kernel, "Fallo el DUMP de memoria del proceso <%d>", proceso->pid);
                    cambiar_estado(proceso, EXIT);

                    queue_push(queue_exit, proceso);

                    log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);

                    log_metricas_estado(proceso);
                    paquete_memoria_pid(proceso, FINALIZAR_PROCESO);
                    signal_sem(&espacio_memoria);
                }
			
				signal_mutex(&mutex_pcb);
				signal_mutex(&mutex_lista_cpus);
				signal_mutex(&mutex_procesos_ejecutando);
                signal_mutex(&mutex_queue_exit);
				signal_mutex(&mutex_queue_ready);
            
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
	wait_mutex(&mutex_lista_cpus);
	//void* nombre_raw = recibir_informacion_del_buffer(buffer, tamanio_nombre);
	//memcpy(cpu_nueva->cpu_id, cpu_id, tamanio_nombre);
	t_cpu* cpu_nueva = malloc(sizeof(t_cpu));
	cpu_nueva->cpu_id = id;
	t_cpu* encontrada = cpu_ya_existe(lista_cpus, cpu_nueva);
	if(encontrada == NULL){
		cpu_nueva->esta_libre = 1;
		funcion(cpu_nueva, socket_fd);
		list_add(lista_cpus, cpu_nueva);
		signal_sem(&cpu_libre);
		//signal_sem(&planificacion_principal);
		int valor = -1;
		sem_getvalue(&cpu_libre, &valor);
		//log_debug(logger_kernel, "Semaforo cpu_libre: %d", valor);
		//log_debug(logger_kernel, "Tamanio del nombre: %d", tamanio_nombre);
		log_debug(logger_kernel, "Se identifico la CPU: %d", cpu_nueva->cpu_id);
		mostrar_cpus();
		signal_mutex(&mutex_lista_cpus);
		return cpu_nueva;
	}
	else{ 
		funcion(encontrada, socket_fd);
		free(cpu_nueva);
		mostrar_cpus();

		pthread_t esperar_devolucion;
		pthread_mutex_lock(&mutex_creacion_hilos);
    	pthread_create(&esperar_devolucion, NULL, esperar_dispatch, (void*) encontrada);
		pthread_mutex_unlock(&mutex_creacion_hilos);
		pthread_detach(esperar_devolucion);
		signal_mutex(&mutex_lista_cpus);
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
			pthread_mutex_lock(&mutex_creacion_hilos);
			pthread_create(&hilo_cliente, NULL, (void*) atender_interrupt, args);
			pthread_mutex_unlock(&mutex_creacion_hilos);
			//log_debug(logger_kernel, "[THREAD] Creo hilo para atender interrupt");
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
			pthread_mutex_lock(&mutex_creacion_hilos);
			pthread_create(&hilo_cliente, NULL, (void*) atender_io, args);
			pthread_mutex_unlock(&mutex_creacion_hilos);
			//log_debug(logger_kernel, "[THREAD] Creo hilo para atender io");
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
	dispositivo->esta_libre = true;

	list_add(lista_dispositivos_io, dispositivo);

	wait_mutex(&mutex_diccionario_esperando_io);
	wait_mutex(&mutex_diccionario_ejecutando_io);
	if(!dictionary_has_key(diccionario_esperando_io, nombre_disp)){
		t_queue* cola_io = queue_create();
		dictionary_put(diccionario_esperando_io, nombre_disp, cola_io);
	}
	if(!dictionary_has_key(diccionario_ejecutando_io, nombre_disp)){
		t_list* ejecutando_io = list_create();
		dictionary_put(diccionario_ejecutando_io, nombre_disp, ejecutando_io);
	}
	signal_mutex(&mutex_diccionario_ejecutando_io);
	signal_mutex(&mutex_diccionario_esperando_io);

	id_io_incremental++;
	signal_sem(&dispositivo_libre);

	pthread_t hilo_escucha_io;
	pthread_mutex_lock(&mutex_creacion_hilos);
	pthread_create(&hilo_escucha_io, NULL, (void*) escuchar_socket_io, dispositivo);
	pthread_mutex_unlock(&mutex_creacion_hilos);
	pthread_detach(hilo_escucha_io);

	log_debug(logger_kernel,"Se registro dispositivo: %s", dispositivo->nombre);
	wait_mutex(&mutex_diccionario_esperando_io);
	wait_mutex(&mutex_diccionario_ejecutando_io);
	comprobar_cola_bloqueados(dispositivo);
	signal_mutex(&mutex_diccionario_ejecutando_io);
	signal_mutex(&mutex_diccionario_esperando_io);
	return NULL;
}

void* escuchar_socket_io(void* arg){
	t_dispositivo_io* dispositivo = (t_dispositivo_io*) arg;
	while(1){
		int codigo = recibir_operacion(dispositivo->socket);
		t_buffer* paquete = recibir_paquete(dispositivo->socket);
		log_debug(logger_kernel, "Recibi codigo: %d de dispositivo: %s", codigo, dispositivo->nombre);
		switch (codigo){
			case FINALIZACION_IO:
				int pid = recibir_int_del_buffer(paquete);
				wait_mutex(&mutex_queue_susp_ready);
				wait_mutex(&mutex_queue_ready);
				wait_mutex(&mutex_queue_block);
				wait_mutex(&mutex_lista_cpus);
				wait_mutex(&mutex_lista_dispositivos_io);
				wait_mutex(&mutex_pcb);
				wait_mutex(&mutex_diccionario_esperando_io);
				wait_mutex(&mutex_diccionario_ejecutando_io);

				//log_debug(logger_kernel, "Cantidad de procesos en IO (%s, %d): %d",dispositivo->nombre, dispositivo->id, queue_size(cola));
				tiempo_en_io* proceso = buscar_proceso_en_io(dispositivo->nombre, pid); // necesitamos el pid del proceso para poder buscarlo en la lista
				
				if(proceso == NULL){
					log_debug(logger_kernel, "No se encontro el proceso en io pid <%d>", pid);
				} else {
					// poner un mutex que verifique io de cada proceso?
					if(proceso->pcb->estado == SUSP_BLOCKED){
						cambiar_estado(proceso->pcb, SUSP_READY);
						list_add(queue_susp_ready, proceso->pcb);
						//log_debug(logger_kernel, "Cantidad de procesos en susp ready: %d", list_size(queue_susp_ready));
						signal_sem(&proceso_suspendido_ready);
						int valor = -1;
						sem_getvalue(&proceso_suspendido_ready, &valor);
						log_debug(logger_kernel, "Signal semaforo proceso susp ready: %d", valor);
						signal_sem(&nuevo_proceso);
						signal_sem(&nuevo_proceso_suspendido_ready);
						signal_sem(&espacio_memoria);
						signal_sem(&largo_plazo);
					}
					else{
						poner_en_ready(proceso->pcb, false);

						//if(strcmp(algoritmo_corto_plazo, "SRT") != 0){
        				signal_sem(&planificacion_principal);
    					//}
					}
					log_info(logger_kernel, "## %d finalizó IO", proceso->pcb->pid);
					
					comprobar_cola_bloqueados(dispositivo);
					
					
					free(proceso);
				}

				
				signal_mutex(&mutex_diccionario_ejecutando_io);
				signal_mutex(&mutex_diccionario_esperando_io);
				signal_mutex(&mutex_pcb);
				signal_mutex(&mutex_queue_ready);
				signal_mutex(&mutex_lista_cpus);
				signal_mutex(&mutex_queue_susp_ready);
				signal_mutex(&mutex_lista_dispositivos_io);
				signal_mutex(&mutex_queue_block);

				break;
			case -1:
				log_debug(logger_kernel, "[Dispositivo %s IO Desconectado]", dispositivo->nombre);
				wait_mutex(&mutex_queue_block);
				wait_mutex(&mutex_procesos_ejecutando);
				wait_mutex(&mutex_lista_dispositivos_io);
				wait_mutex(&mutex_pcb);
				list_remove_element(lista_dispositivos_io, dispositivo);
				t_dispositivo_io* io_existente = buscar_io(dispositivo->nombre);
				if(io_existente == 	NULL){
					queue_clean_and_destroy_elements(obtener_esperando_io(dispositivo->nombre), finalizar_proceso_io);
				}
				t_list* lista = obtener_ejecutando_io(dispositivo->nombre);
				for(int i=0; i<list_size(lista);i++){
					tiempo_en_io* actual = list_get(lista, i);
					if(actual->dispositivo->id == dispositivo->id){
						finalizar_proceso_io(actual);
					}
				}
				signal_mutex(&mutex_queue_block);
				signal_mutex(&mutex_procesos_ejecutando);
				signal_mutex(&mutex_lista_dispositivos_io);
				signal_mutex(&mutex_pcb);
				free(dispositivo);
				pthread_exit(NULL);
				break;
			default:
				log_warning(logger_kernel, "Operación desconocida de IO");
				break;
		}
		eliminar_buffer(paquete);
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

	//wait_mutex(&mutex_queue_new);
	list_add(queue_new, pcb_nuevo);
	//signal_mutex(&mutex_queue_new);
	signal_sem(&nuevo_proceso);
	signal_sem(&largo_plazo);
}

void finalizar_proceso(t_pcb* proceso){

	if(paquete_memoria_pid(proceso, FINALIZAR_PROCESO)){
		cambiar_estado(proceso, EXIT);

		log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);
		
		log_metricas_estado(proceso);
		free(proceso);
		signal_sem(&espacio_memoria);
		signal_sem(&largo_plazo);
		//signal_sem(&planificador_largo_plazo);
		/* log_debug(logger_kernel, "Signal del planificador_largo_plazo (%ld) por finalizar proceso pid <%d>", 
			planificador_largo_plazo.__align, proceso->pid); */
		//log_debug(logger_kernel, "HAY ESPACIO FINALIZACION EN MEMORIA");
	}
	else{
		log_error(logger_kernel, "Error en memoria al finalizar proceso PID: <%d>", proceso->pid);
	}
}

void finalizar_proceso_io(void* arg){
	tiempo_en_io* proceso_io = (tiempo_en_io*) arg;
	finalizar_proceso(proceso_io->pcb);
	free(proceso_io);
}

void finalizar_unidad_ejecucion(void* arg){
	t_unidad_ejecucion* unidad = arg;
	finalizar_proceso(unidad->proceso);
	free(unidad);
}

void matar_proceso(void* arg){
	t_pcb* proceso = arg;
	finalizar_proceso(proceso);
}