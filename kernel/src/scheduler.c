#include "scheduler.h"

char* algoritmo_corto_plazo;
char* algoritmo_largo_plazo;
int estimacion_inicial;
double estimador_alfa;

void *planificar_corto_plazo(void* arg){
	while(1){
        wait_sem(&proceso_ready);
        // wait de cpu libre?
        wait_mutex(&mutex_queue_ready);
        if(!queue_is_empty(queue_ready)){
            t_pcb* proceso = queue_pop(queue_ready);
            signal_mutex(&mutex_queue_ready);
            t_cpu* cpu_encargada = malloc(sizeof(t_cpu));
            poner_en_ejecucion(proceso, &cpu_encargada);
            cambiarEstado(proceso, EXEC);
            /* 
            crear hilo que quede a la espera de recibir dicho PID después de la ejecución? 
            */
           pthread_t* esperar_devolucion = malloc(sizeof(pthread_t));
           pthread_create(esperar_devolucion, NULL, (void*) esperar_dispatch, (void*) cpu_encargada);
        }
        else{
            signal_mutex(&mutex_queue_ready);
        }
	}

}

/* void esperar_devolucion_proceso(void* arg){
    pthread_t* dispatch = malloc(sizeof(pthread_t));
    pthread_create(dispatch, NULL, (void*) esperar_dispatch, arg);
} */

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
    int io_id = recibir_int_del_buffer(paquete);
    int io_tiempo = recibir_int_del_buffer(paquete);

    proceso->program_counter = pc;
    
    switch (motivo)
    {
    case FINALIZADO:
        cambiarEstado(proceso, EXIT);

        wait_mutex(&mutex_queue_exit);
        queue_push(queue_exit, proceso);
        signal_mutex(&mutex_queue_exit);

        signal_sem(&espacio_memoria);
        break;
    case CAUSA_IO:

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

        wait_mutex(&mutex_queue_block);
        queue_push(obtener_cola_io(io_id), &proceso_bloqueado);
        signal_mutex(&mutex_queue_block);

        cambiarEstado(proceso, BLOCKED);

        free(paquete);
        break;

    case SIGNAL:
        // ver que hacer con este motivo que envia cpu
        break;
    
    default:
        log_error(logger_kernel, "Motivo de desalojo desconocido");
        break;
    }
    
    /* 
    
    CORREGIR sizeof(int) 
    
    */

    free(paquete);
}

void esperar_interrupt(void* arg){
    t_cpu* cpu_encargada = (t_cpu*) arg;
    t_buffer* buffer = malloc(sizeof(t_buffer)); 
    recv(cpu_encargada->socket_interrupt, buffer, sizeof(int), 0);
    
    /* 
    
    CORREGIR sizeof(int) 
    
    */

    int* pid_proceso = malloc(sizeof(int));
    memcpy(buffer->stream, (void*) pid_proceso, sizeof(int));
    int* tiempo_proceso = malloc(sizeof(int));
    memcpy(buffer->stream, (void*) tiempo_proceso, sizeof(int));
    int* id_io_a_usar = malloc(sizeof(int));
    memcpy(buffer->stream, (void*) id_io_a_usar, sizeof(int));
    int id_io = *id_io_a_usar;

    t_dispositivo_io* dispositivo = buscar_io(id_io);
    if(dispositivo == NULL){
        log_error(logger_kernel, "No se encontro la IO: %d", id_io);
    }
    t_paquete* paquete = crear_paquete(PETICION_IO);
    agregar_a_paquete(paquete, pid_proceso, sizeof(int));
    agregar_a_paquete(paquete, tiempo_proceso,sizeof(int));
    enviar_paquete(paquete, dispositivo->socket);
    eliminar_paquete(paquete);
    free(buffer);
    free(pid_proceso);
    free(tiempo_proceso);
    free(id_io_a_usar);
}

t_dispositivo_io* buscar_io(int id_io){
    for(int i = 0; i<list_size(lista_dispositivos_io); i++){
        t_dispositivo_io* actual = (t_dispositivo_io*) list_get(lista_dispositivos_io, i);
        if(actual->id == id_io){
            return actual;
        }
    }
    return NULL;
}


void *planificar_largo_plazo_FIFO(void* arg){
	while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_new);
		if(!queue_is_empty(queue_new)){
			t_pcb* proceso = queue_peek(queue_new);
            signal_mutex(&mutex_queue_new);
			if(espacio_en_memoria(proceso)){

                wait_mutex(&mutex_queue_new);
				queue_pop(queue_new);
                signal_mutex(&mutex_queue_new);

                poner_en_ready(proceso);

                signal_sem(&proceso_ready);
			}
            else{
                signal_sem(&nuevo_proceso); // el proceso nuevo sigue en NEW, hago signal de vuelta
                wait_sem(&espacio_memoria); // espero que algun proceso finalice 

                // HACER SIGNAL EN FINALIZACION
            }
		}
		else{
            signal_mutex(&mutex_queue_new); // libero recurso del mutex
		}
	}

}

void *planificar_largo_plazo_PMCP(void* arg){
    pthread_t hilo_comprobar_espacio;
    pthread_create(&hilo_comprobar_espacio, NULL, (void*) comprobar_espacio_memoria, NULL);
    pthread_detach(hilo_comprobar_espacio);

    while(1){
        wait_sem(&nuevo_proceso);
        wait_mutex(&mutex_queue_new);
        t_pcb *proceso = list_get(queue_new_PMCP, 0);
        signal_mutex(&mutex_queue_new);
        if(espacio_en_memoria(proceso)){
            wait_mutex(&mutex_queue_new);
            list_remove_element(queue_new_PMCP, proceso);
            signal_mutex(&mutex_queue_new);

            poner_en_ready(proceso);
            
            signal_sem(&proceso_ready);
        }
        else{
            list_sort(queue_new_PMCP, proceso_es_mas_chico);
        }
    }
}

void *comprobar_espacio_memoria(void* arg){
    while(1){
        wait_sem(&espacio_memoria);

        wait_mutex(&mutex_queue_new);
        list_sort(queue_new_PMCP, proceso_es_mas_chico); // no se si hay q ordenar otra vez, lo puse x las dudas
        t_pcb* proceso = list_get(queue_new_PMCP, 0);
        signal_mutex(&mutex_queue_new);

        if(espacio_en_memoria(proceso)){
            poner_en_ready(proceso);
        }
    }
}

bool espacio_en_memoria(t_pcb* proceso){
    int conexion = crear_conexion_memoria();
    hacer_handshake(logger_kernel, conexion);

    t_paquete* paqueteID = crear_paquete(IDENTIFICACION);

	void* coso_a_enviar = malloc(sizeof(int));
	int codigo = KERNEL;
	memcpy(coso_a_enviar, &codigo, sizeof(int));
    agregar_a_paquete(paqueteID, coso_a_enviar, sizeof(op_code));
    enviar_paquete(paqueteID, conexion);
    eliminar_paquete(paqueteID);

    t_paquete* paqueteInfo = crear_paquete(INICIAR_PROCESO);
    agregar_a_paquete(paqueteInfo, &proceso->pid, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->tamanio_proceso, sizeof(int));
    agregar_a_paquete(paqueteInfo, &proceso->instrucciones, sizeof(char*));
    enviar_paquete(paqueteInfo, conexion);

    eliminar_paquete(paqueteInfo);

    t_buffer* buffer = malloc(sizeof(t_buffer));
    recv(conexion, buffer, sizeof(int), 0);
    int resultado = recibir_int_del_buffer(buffer);
    free(buffer);
    return resultado==OK;
}


void cambiarEstado(t_pcb* proceso, t_estado estado){

    t_metricas_estado_tiempo* metrica_anterior = obtener_ultima_metrica(proceso);
    if(metrica_anterior != NULL){
        metrica_anterior->tiempo_fin = clock(); // o se puede cambiar por time()
        if(metrica_anterior->estado == EXEC && estado == BLOCKED){
            proceso->rafaga_real = calcular_rafaga(proceso->metricas_tiempo);
        }
    }

    proceso->estado = estado;
    proceso->metricas_estado[id_estado(estado)]++;


    t_metricas_estado_tiempo* metrica = malloc(sizeof(t_metricas_estado_tiempo));
    metrica->estado = estado;
    metrica->tiempo_inicio = clock();

    list_add(proceso->metricas_tiempo, metrica);
}

t_metricas_estado_tiempo* obtener_ultima_metrica(t_pcb* proceso){
    if(proceso->metricas_tiempo == NULL)
    {
        return NULL;
    }
    t_metricas_estado_tiempo* ultimo_elemento = list_get(proceso->metricas_tiempo, list_size(proceso->metricas_tiempo)-1);
        return ultimo_elemento;
}

int calcular_rafaga(t_list* metricas_tiempo){
    t_metricas_estado_tiempo* ultima_metrica = list_get(metricas_tiempo, sizeof(metricas_tiempo)-1); 
    t_metricas_estado_tiempo* metrica = list_get(metricas_tiempo, sizeof(metricas_tiempo)-3);
    if(metrica->estado == EXEC){
        return ultima_metrica->tiempo_fin - metrica->tiempo_inicio - (ultima_metrica->tiempo_inicio - metrica->tiempo_fin);
    }
    else{
        return ultima_metrica->tiempo_fin - ultima_metrica->tiempo_inicio;
    }
}

void poner_en_ready(t_pcb* proceso){

    cambiarEstado(proceso, READY);
    wait_mutex(&mutex_queue_ready);
	queue_push(queue_ready, proceso);
    signal_mutex(&mutex_queue_ready);
}

bool proceso_es_mas_chico(void* a, void* b){
    t_pcb* proceso_a = (t_pcb*) a;
    t_pcb* proceso_b = (t_pcb*) b;
    
    /* En un futuro sera:

    double estimacion_a = proceso_a->rafaga_real * estimador_alfa + proceso_a->estimacion_anterior * (1-estimador_alfa);
    double estimacion_b = proceso_b->rafaga_real * estimador_alfa + proceso_b->estimacion_anterior * (1-estimador_alfa);
    return estimacion_a <= estimacion_b;

    */

    return proceso_a->tamanio_proceso <= proceso_b->tamanio_proceso;
}
void poner_en_ejecucion(t_pcb* proceso, t_cpu** cpu_encargada){

    t_cpu* cpu_libre = buscar_cpu_libre(lista_cpus);
    // mandar a alguna cpu dependiendo de cual este libre

    t_paquete* paquete = crear_paquete(CONTEXTO_PROCESO);
    agregar_a_paquete(paquete, &proceso->pid, sizeof(int));
    agregar_a_paquete(paquete, &proceso->program_counter, sizeof(int));
    enviar_paquete(paquete, cpu_libre->socket_dispatch);
    eliminar_paquete(paquete);

    list_add(lista_procesos_ejecutando, proceso);

    //
    *cpu_encargada = cpu_libre;

}

t_cpu* buscar_cpu_libre(t_list* lista_cpus){
    for(int i = 0; i<list_size(lista_cpus); i++){
        t_cpu* actual = list_get(lista_cpus, i);
        if(actual->esta_libre){
            return actual;
        }
    }
    log_error(logger_kernel, "No se hay CPUs libres");
    return NULL;
}

t_pcb* buscar_proceso_pid(uint32_t pid){
    for(int i=0; i<list_size(lista_procesos_ejecutando); i++){
        t_pcb* actual = list_get(lista_procesos_ejecutando, i);
        if(actual->pid == (uint32_t)pid){
            list_remove(lista_procesos_ejecutando, i);
            return actual;
        }
    }
    return NULL;
}

void enviar_proceso_a_io(t_pcb* proceso, int io_id, int io_tiempo){

    t_dispositivo_io* dispositivo = buscar_io(io_id);
    
    t_paquete* paquete = crear_paquete(PETICION_IO);
    agregar_a_paquete(paquete, &(proceso->pid), sizeof(int));
    agregar_a_paquete(paquete, &io_tiempo,sizeof(int));
    enviar_paquete(paquete, dispositivo->socket);

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
    queue_push(queue_ready, &(proceso->pcb));
    signal_mutex(&mutex_queue_ready);

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

t_queue* obtener_cola_io(int io_id){
    return list_get(queue_block, io_id);
}