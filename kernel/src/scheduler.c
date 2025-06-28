#include "scheduler.h"

char* algoritmo_corto_plazo;
char* algoritmo_largo_plazo;

void *planificar_corto_plazo_FIFO(void* arg){
    log_info(logger_kernel, "Entre a Corto Plazo FIFO");
	while(1){
        wait_sem(&proceso_ready);
        if(!list_is_empty(queue_ready)){
            log_info(logger_kernel, "Busco una CPU libre");
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            if(hay_cpu_libre(&cpu_encargada)){
                wait_mutex(&mutex_queue_ready);
                t_pcb* proceso = list_get(queue_ready, 0);
                signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada, cpu_encargada->socket_dispatch);
                pthread_t* esperar_devolucion = malloc(sizeof(pthread_t));
                pthread_create(esperar_devolucion, NULL, (void*) esperar_dispatch, (void*) cpu_encargada);
                free(cpu_encargada);
            }
        }
	}

}

bool hay_cpu_libre(t_cpu** cpu_encargada){
    t_cpu* cpu_libre = buscar_cpu_libre(lista_cpus);
    *cpu_encargada = cpu_libre;

    return cpu_libre != NULL;
}

void esperar_dispatch(void* arg){
    t_cpu* cpu_encargada = (t_cpu*) arg;
    t_buffer* paquete = recibir_paquete(cpu_encargada->socket_dispatch);

    uint32_t pid = recibir_uint32_del_buffer(paquete);
    t_pcb* proceso = buscar_proceso_pid(pid);
    uint32_t pc = recibir_uint32_del_buffer(paquete);
    for(int i=0; i<4; i++){
        proceso->registros[i] = recibir_uint32_del_buffer(paquete);
    };
    int motivo = recibir_int_del_buffer(paquete);

    proceso->program_counter = pc;
    
    switch (motivo)
    {
    case FINALIZADO:
        cambiarEstado(proceso, EXIT);

        wait_mutex(&mutex_queue_exit);
        queue_push(queue_exit, proceso);
        signal_mutex(&mutex_queue_exit);

        sacar_proceso_ejecucion(proceso);

        log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "EXIT");

        log_info(logger_kernel, "## %d - Finaliza el proceso", proceso->pid);

        log_metricas_estado(proceso);

        paquete_memoria_pid(proceso, FINALIZAR_PROCESO);

        free(proceso);

        break;
    case CAUSA_IO:

        int io_id = recibir_int_del_buffer(paquete);
        int io_tiempo = recibir_int_del_buffer(paquete);
        t_dispositivo_io* dispositivo = buscar_io(io_id);
        if(dispositivo == NULL){
            log_error(logger_kernel, "No se encontro la IO: %d", io_id);
            break;
        }

        if(queue_is_empty(obtener_cola_io(io_id))){
            enviar_proceso_a_io(proceso, io_id, io_tiempo);
        }

        tiempo_en_io proceso_bloqueado;
        proceso_bloqueado.pcb = proceso;
        proceso_bloqueado.tiempo = io_tiempo;

        sacar_proceso_ejecucion(proceso);

        wait_mutex(&mutex_queue_block);
        queue_push(obtener_cola_io(io_id), &proceso_bloqueado);
        signal_mutex(&mutex_queue_block);

        cambiarEstado(proceso, BLOCKED);

        free(paquete);
        break;

    case INIT_PROC:
        int tamanio_proceso = recibir_int_del_buffer(paquete);
        char* archivo = recibir_string_del_buffer(paquete);
        char* nombre = malloc(sizeof(char));
        sprintf(nombre, "%d",tamanio_proceso);

        log_info(logger_kernel, "## (<%d>) - Solicito syscall: <%s>", proceso->pid, "INIT_PROC");

        crear_proceso(archivo, nombre);
        free(nombre);
        break;
    case MEMORY_DUMP:
        paquete_memoria_pid(proceso, MEMORY_DUMP);
        // bloquear proceso?
        break;
    default:
        log_error(logger_kernel, "Motivo de desalojo desconocido");
        break;
    }
    free(paquete);
}

void paquete_memoria_pid(t_pcb* proceso, op_code codigo){
    int conexion = crear_conexion_memoria();
    
    t_paquete* paquete = crear_paquete(codigo);
    agregar_a_paquete(paquete, &(proceso->pid), sizeof(int));
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);
}

void* planificar_corto_plazo_SJF(void* arg){
    while(1){
        wait_sem(&proceso_ready);
        wait_mutex(&mutex_queue_ready);
        if(!list_is_empty(queue_ready)){
            list_sort(queue_ready, shortest_job_first);
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            if(hay_cpu_libre(&cpu_encargada)){
                t_pcb* proceso = list_remove(queue_ready, 0);
                signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada, cpu_encargada->socket_dispatch);
                pthread_t* esperar_devolucion = malloc(sizeof(pthread_t));
                pthread_create(esperar_devolucion, NULL, (void*) esperar_dispatch, (void*) cpu_encargada);
                free(cpu_encargada);
            }
            signal_mutex(&mutex_queue_ready);
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
    }

}

void* planificar_corto_plazo_SJF_desalojo(void* arg){
      while(1){
        wait_sem(&proceso_ready);
        wait_mutex(&mutex_queue_ready);
        if(!list_is_empty(queue_ready)){
            list_sort(queue_ready, shortest_job_first);
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            if(hay_cpu_libre(&cpu_encargada)){
                t_pcb* proceso = list_remove(queue_ready, 0);
                signal_mutex(&mutex_queue_ready);
                poner_en_ejecucion(proceso, cpu_encargada, cpu_encargada->socket_dispatch);
                cambiarEstado(proceso, EXEC);
                pthread_t* esperar_devolucion = malloc(sizeof(pthread_t));
                pthread_create(esperar_devolucion, NULL, (void*) esperar_dispatch, (void*) cpu_encargada);
                free(cpu_encargada);
            }
            /*  
                Hay que ir iterando la lista de procesos en ready para ver si hay procesos en ejecucion
                que tienen una estimacion mas grande que los que estan en ready. Si es asi hay que desalojarlos
            */ 
            int cant_procesos;
            for(int i = 0; i<list_size(queue_ready);i++){
                t_pcb* otro_proceso = list_get(queue_ready, i);
                for(int j = 0; j<list_size(lista_procesos_ejecutando); j++){
                    t_unidad_ejecucion* proceso_ejecutando = list_get(lista_procesos_ejecutando, j);
                    if(shortest_job_first((void*) otro_proceso, (void*) proceso_ejecutando->proceso) && otro_proceso->estado == READY){
                        poner_en_ejecucion(otro_proceso, proceso_ejecutando->cpu, proceso_ejecutando->cpu->socket_interrupt);
                        cant_procesos++;
                        sacar_proceso_ejecucion(proceso_ejecutando->proceso);
                        log_info(logger_kernel, "## (<%d>) - Desalojado por algoritmo SJF/SRT", proceso_ejecutando->proceso->pid);
                        list_add_sorted(queue_ready, proceso_ejecutando, shortest_job_first);
                    }
                }

            }
            for(int i=0; i<cant_procesos ; i++){
                list_remove(queue_ready, i);
            }
            signal_mutex(&mutex_queue_ready);
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
    }
}

void actualizar_estimacion(t_pcb* proceso){
    double tiempo_actual = (double)time(NULL);
    t_metricas_estado_tiempo* metrica_anterior = obtener_ultima_metrica(proceso);
    if(metrica_anterior->tiempo_fin == 0){
        proceso->estimacion_actual-= (tiempo_actual - metrica_anterior->tiempo_inicio);
    }
    else{
        proceso->estimacion_actual-= (tiempo_actual - metrica_anterior->tiempo_fin);
    }
    metrica_anterior->tiempo_fin = tiempo_actual;
}

void log_metricas_estado(t_pcb* proceso){
    long tiempo_new;
    long tiempo_ready;
    long tiempo_exec;
    long tiempo_blocked;
    long tiempo_susp_ready;
    long tiempo_susp_blocked;
    for(int i = 0; i<list_size(proceso->metricas_tiempo);i++){
        t_metricas_estado_tiempo* metrica = list_get(proceso->metricas_tiempo, i);
        long tiempo = calcular_tiempo(metrica);
        switch (metrica->estado)
        {
        case NEW:
            tiempo_new += tiempo;
            break;
        case READY:
            tiempo_ready += tiempo;
            break;
        case EXEC:
            tiempo_exec += tiempo;
            break;
        case BLOCKED:
            tiempo_blocked += tiempo;
            break;
        case SUSP_READY:
            tiempo_susp_ready += tiempo;
            break;
        case SUSP_BLOCKED:
            tiempo_susp_blocked += tiempo;
            break;
        case EXIT:
            break;
        default:
            break;
        }

    }

    log_info(logger_kernel, "## %d - Metricas de estado: NEW %d %ld, READY %d %ld, EXEC %d %ld, BLOCKED %d %ld, SUSP_READY %d %ld, SUSP_BLOCKED %d %ld", 
    proceso->pid, proceso->metricas_estado[0], tiempo_new, proceso->metricas_estado[1], tiempo_ready, proceso->metricas_estado[2], tiempo_exec, 
    proceso->metricas_estado[3], tiempo_blocked, proceso->metricas_estado[4], tiempo_susp_ready, proceso->metricas_estado[5], tiempo_susp_blocked);

}

long calcular_tiempo(t_metricas_estado_tiempo* metrica){
    return metrica->tiempo_fin - metrica->tiempo_inicio;
}


void *planificar_largo_plazo_FIFO(void* arg){
    t_agregacion_ready* agregacion = arg;
	while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_new);
		if(!list_is_empty(queue_new)){
			t_pcb* proceso = list_get(queue_new, 0);
            signal_mutex(&mutex_queue_new);
			if(espacio_en_memoria(proceso)){

                wait_mutex(&mutex_queue_new);
				list_remove_element(queue_new, proceso);
                signal_mutex(&mutex_queue_new);

                // PONER EN READY
                log_debug(logger_kernel, "Cola de ready:");
                mostrar_cola((t_queue**) &agregacion->cola_ready);
                poner_en_ready(proceso);
			}
            else{
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                wait_sem(&espacio_memoria); // espero que algun proceso finalice 
            }
		}
		else{
            signal_mutex(&mutex_queue_new); // libero recurso del mutex
		}
	}

}

void *planificar_largo_plazo_PMCP(void* arg){
    t_agregacion_ready* agregacion = arg;
    pthread_t hilo_comprobar_espacio;
    pthread_create(&hilo_comprobar_espacio, NULL, (void*) comprobar_espacio_memoria, arg);
    pthread_detach(hilo_comprobar_espacio);

    while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_new);
        list_sort(queue_new, proceso_es_mas_chico);
        t_pcb *proceso = list_get(queue_new, 0);
        signal_mutex(&mutex_queue_new);
        if(espacio_en_memoria(proceso)){
            wait_mutex(&mutex_queue_new);
            list_remove_element(queue_new, proceso);
            signal_mutex(&mutex_queue_new);

            // PONER EN READY
            log_info(logger_kernel, "Cola de ready:");
            mostrar_cola((t_queue**) &agregacion->cola_ready);
            poner_en_ready(proceso);
        }
    }
}

void *comprobar_espacio_memoria(void* arg){
    while(1){
        wait_sem(&espacio_memoria);

        wait_mutex(&mutex_queue_new);
        list_sort(queue_new, proceso_es_mas_chico); // no se si hay q ordenar otra vez, lo puse x las dudas
        t_pcb* proceso = list_get(queue_new, 0);
        signal_mutex(&mutex_queue_new);

        if(espacio_en_memoria(proceso)){

            wait_mutex(&mutex_queue_new);
            list_remove_element(queue_new, proceso);
            signal_mutex(&mutex_queue_new);

            poner_en_ready(proceso);
        }
    }
}

bool espacio_en_memoria(t_pcb* proceso){
   /*  int conexion = crear_conexion_memoria();

    t_paquete* paqueteInfo = crear_paquete(INICIAR_PROCESO);
    agregar_a_paquete(paqueteInfo, &proceso->pid, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->tamanio_proceso, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->instrucciones, sizeof(char*));
    enviar_paquete(paqueteInfo, conexion);

    eliminar_paquete(paqueteInfo);

    int resultado;
    recv(conexion, &resultado, sizeof(int), 0);
    return resultado==OK; */
    return 1;
}

void poner_en_ready(t_pcb* proceso){

    cambiarEstado(proceso, READY);
    wait_mutex(&mutex_queue_ready);
	list_add(queue_ready, proceso);
    signal_mutex(&mutex_queue_ready);
    signal_sem(&proceso_ready);
}

bool proceso_es_mas_chico(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    

    return proceso_a->tamanio_proceso <= proceso_b->tamanio_proceso;
}

bool shortest_job_first(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    
    return proceso_a->estimacion_actual < proceso_b->estimacion_actual;
}
void poner_en_ejecucion(t_pcb* proceso, t_cpu* cpu_encargada, int socket){

    // t_cpu* cpu_libre = buscar_cpu_libre(lista_cpus);
    // mandar a alguna cpu dependiendo de cual este libre

    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    agregar_a_paquete(paquete, &proceso->program_counter, sizeof(int));
    // enviar_paquete(paquete, cpu_libre->socket_dispatch);
    enviar_paquete(paquete, socket);
    eliminar_paquete(paquete);

    t_unidad_ejecucion* nueva = malloc(sizeof(t_unidad_ejecucion));
    nueva->cpu = cpu_encargada;
    nueva->proceso = proceso;
    list_add(lista_procesos_ejecutando, nueva);
    cambiarEstado(proceso, EXEC);

    
    // *cpu_encargada = cpu_libre;

}

void enviar_proceso_a_io(t_pcb* proceso, int io_id, int io_tiempo){

    t_dispositivo_io* dispositivo = buscar_io(io_id);
    
    t_paquete* paquete = crear_paquete(PETICION_IO);
    agregar_a_paquete(paquete, &(proceso->pid), sizeof(int));
    agregar_a_paquete(paquete, &io_tiempo,sizeof(int));
    enviar_paquete(paquete, dispositivo->socket);

    log_info(logger_kernel, "## %d - Bloqueado por IO: %s", proceso->pid, dispositivo->nombre);

    int *args = malloc(sizeof(int));
	*args = io_id;

    pthread_t hilo_vuelta_io;
    pthread_create(&hilo_vuelta_io, NULL, (void*) vuelta_proceso_io, args); // creo hilo para esperar la vuelta de la io
    pthread_detach(hilo_vuelta_io);
}

void vuelta_proceso_io(void* args){
    int *id_io = (int*) args;
    t_dispositivo_io* io = buscar_io(*id_io);


    t_buffer* buffer = malloc(sizeof(t_buffer));
    int *pid = malloc(sizeof(int));
    recv(io->socket, buffer, sizeof(int), 0);
    memcpy(pid, buffer->stream, sizeof(int));

    tiempo_en_io *proceso = queue_pop(obtener_cola_io(io->id));

    cambiarEstado(proceso->pcb, READY);
    wait_mutex(&mutex_queue_ready);
    list_add(queue_ready, proceso->pcb);
    signal_mutex(&mutex_queue_ready);

    log_info(logger_kernel, "## %d finalizó IO y pasa a READY", *pid);

    comprobar_cola_bloqueados(io->id);

    free(pid);
    free(buffer);
}

void comprobar_cola_bloqueados(int io_id){

    t_queue* cola_io = obtener_cola_io(io_id);

    if(!queue_is_empty(cola_io)){
        tiempo_en_io *proceso = queue_peek(cola_io);

        enviar_proceso_a_io(proceso->pcb, io_id, proceso->tiempo);
    }
}
